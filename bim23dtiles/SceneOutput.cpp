#include "SceneOutput.h"
#include "util.h"
#include "SubScenePacker.h"
#include "SubScene.h"

#include "B3dm.h"
#include "OsgUtil.h"
#include "ModelInput.h"
#include "SceneOutputConfig.h"
#include "Scene2Gltf.h"
#include "TilesStorage.h"

namespace XBSJ {
	 
	SceneOutput::SceneOutput(SceneOutputConfig *cfg, SceneOutput * p, size_t idx):
		 parent(p),config(cfg),index(idx)
	{

	}


	SceneOutput::~SceneOutput()
	{

	}

	osg::BoundingBoxd SceneOutput::globleBox() {
		if (!packer)
			return osg::BoundingBoxd();

		return packer->globleBox;
	}

	bool SceneOutput::process(json& content, shared_ptr<SubScene> scene, double gerror) {

		subscene = scene;
		packer = make_shared<SubScenePacker>(scene);

		//set geometricError along with src box size 
		if (gerror == 0)
			geometricError = packer->srcBox.radius() * config->boxRadius2GeometricError;
		else
			geometricError = gerror;

		//geometricError = 0;

		return process(content);
	}

	bool SceneOutput::process(json& content) {

		auto url = getUrl();
		ProgressHelper pglobal("process :" + url, 1);

		//1, simplify
		bool simpled = false;
		{
			ProgressHelper psimple("simply :" + url, 1, 0.1);
			simpled = packer->simply(geometricError, config);
		}
 
		//2, save one b3dm file
		{
			ProgressHelper psave("save :" + url, 1, 0.1);
			content = save2b3dm();
		}
	 
		if (!simpled)
		{
			content["geometricError"] = 0;
			return true;
		}		

		// cal next lod data
		double nextGeometricError = geometricError * config->nextGeometricErrorFactor;

		// function exit condition
		if (nextGeometricError < config->minGeometricError) {
			LOG(WARNING) << "nextGeometricError < config->minGeometricError";
			content["geometricError"] = 0;
			return true;
		}
		 
		//5, child lod construct
		list<shared_ptr<SubScene>> children;
		if (!splitSubScene(children))
		{
			children.push_back(subscene);
		}
		else {
			LOG(INFO) << "splitSubScene:"<<children.size();
		}
	
		list<json> jsonchildren;
		size_t idx = 0;
		ProgressHelper psave("children :" + url, children.size(), 0.8);

		for (auto& child : children) {
			SceneOutput so(config, this, idx);
			json j;
			if (so.process(j, child, nextGeometricError)) {
				jsonchildren.push_back(j);
			}
			idx++;
			psave.skip(idx);
		}
		content["children"] = jsonchildren;
		int count = PostProcess::childrenCount(content);

		//
		if (count > 100) {
			auto filename = "lab_a_" + getPath("_");
			replace(filename, ".b3dm", ".json");
			content = config->linkJson(filename, content);
		}
	 
		return true;
	}

	bool SceneOutput::save2glb(ostream & stm) {

		struct Header {
			char magic[4] = { 'g','l','T','F' };
			uint32_t version = 1; //��ʾ�汾1
			uint32_t length = 0;
		};
		const uint32_t ChunkJSON = 0x4E4F534A;
		const uint32_t ChunkBIN = 0x004E4942;

		struct Chunk {
			uint32_t chunkLength = 0;
			uint32_t chunkType = 0;
		};


		//gltf
		stringstream gltfbufferStream;
		//json gltfjson;
		string gltfcontent;

		Scene2Gltf gltf(config);
		if (!gltf.write(packer, gltfbufferStream, gltfcontent)) {

			LOG(ERROR) << "exportGltf failed";
			return false;
		}

		//4, construct b3dm
		//string gltfcontent = gltfjson.dump();
		while (gltfcontent.size() % 8 != 0) {
			gltfcontent.push_back(' ');
		}

		string gltfbuffer = gltfbufferStream.str();

		Header header;
		header.length = sizeof(Header) + gltfcontent.size() + gltfbuffer.size() + 2 * sizeof(Chunk);
		header.version = 2;

		//header
		stm.write((char*)&header, sizeof(Header));
		//chunk0  gltf json
		Chunk chunkjson;
		chunkjson.chunkLength = gltfcontent.size();
		chunkjson.chunkType = ChunkJSON;
		stm.write((char*)&chunkjson, sizeof(Chunk));
		//content
		stm.write((char*)gltfcontent.data(), gltfcontent.size());

		//chunk1 gltf buffer
		Chunk chunkbin;
		chunkbin.chunkLength = gltfbuffer.size();
		chunkbin.chunkType = ChunkBIN;
		stm.write((char*)&chunkbin, sizeof(Chunk));

		//body
		stm.write((char*)gltfbuffer.data(), gltfbuffer.size());

		return  true;
	}
	
	//输出当前场景到b3dm
	bool SceneOutput::save2b3dm(ostream& stm) {

		//construct glb content
		stringstream glbstm;
		if (!save2glb(glbstm)) {
			LOG(ERROR) << "save2glb  failed";
			return false;
		}

		//construct b3dm
		B3dm b3dm;
		if (!packer->elementNames.empty()) {
			shared_ptr<B3dm::BatchData> bd = make_shared<B3dm::BatchData>();
			bd->name = "name";
			bd->jsonData = packer->elementNames;
			b3dm.batchDatas.push_back(bd);
			b3dm.BATCH_LENGTH = packer->elementNames.size();
		}
		//ID
		if (!packer->elementIds.empty()) {
			shared_ptr<B3dm::BatchData> bd = make_shared<B3dm::BatchData>();
			bd->name = "id";
			bd->jsonData = packer->elementIds;
			b3dm.batchDatas.push_back(bd);
			b3dm.BATCH_LENGTH = packer->elementIds.size();
		}
		//s ID
		if (!packer->elementSIds.empty()) {
			shared_ptr<B3dm::BatchData> bd = make_shared<B3dm::BatchData>();
			bd->name = "sid";
			bd->binData.componentType = "INT";
			bd->binData.type = "SCALAR";
			bd->binData.data.resize(sizeof(int) * packer->elementSIds.size());
			//数据拷贝
			memcpy(&bd->binData.data[0], &packer->elementSIds[0], bd->binData.data.size());

			b3dm.batchDatas.push_back(bd);
			//b3dm.BATCH_LENGTH = packer->elementIds.size();
		}

		//文件名
		if (!packer->fileNames.empty()) {
			shared_ptr<B3dm::BatchData> bd = make_shared<B3dm::BatchData>();
			bd->name = "file";
			bd->jsonData = packer->fileNames;
			b3dm.batchDatas.push_back(bd);
			//b3dm.BATCH_LENGTH = packer->fileNames.size();
		}
		//其他属性
		for (auto & op : packer->elementProps) {
			shared_ptr<B3dm::BatchData> bd = make_shared<B3dm::BatchData>();
 
			//获取属性定义
			auto param = packer->input->scene->params[op->paramindex];
			bd->name = param.name;
			//bd->name = str(op->paramindex);

			//根据其中的类型判定应该存储什么类型
			
			//bool 类型的
			if (param.type == "YesNo") {
				//二进制，byte 存储
				vector<BYTE> data;
				for (auto &p : op->values) {
					if (p.is_number_integer()) {
						data.push_back(p.get<int>());
					}
					else {
						data.push_back(0);
					}
				}
 				// 添加字段
				bd->binData.componentType = "BYTE";
				bd->binData.type = "SCALAR";
				bd->binData.data.resize(sizeof(BYTE) * data.size());
				//数据拷贝
				memcpy(&bd->binData.data[0], &data[0], bd->binData.data.size());
				b3dm.batchDatas.push_back(bd);
			}
			//面积，体积  长度
			else if (param.type == "Area"|| param.type == "Volume" || param.type == "Length") {
				//二进制，float 存储
				vector<float> data;
				for (auto &p : op->values) {
					if (p.is_number()) {
						data.push_back(p.get<float>());
					}
					else {
						data.push_back(0);
					}
				}
				//添加字段
				bd->binData.componentType = "FLOAT";
				bd->binData.type = "SCALAR";
				bd->binData.data.resize(sizeof(float) * data.size());
				//数据拷贝
				memcpy(&bd->binData.data[0], &data[0], bd->binData.data.size());
				b3dm.batchDatas.push_back(bd);
			}
			else if (param.type == "Text" || param.type == "string") {
				//添加字符串字段
				bd->jsonData = op->values;
				b3dm.batchDatas.push_back(bd);
			}
			else if (param.type == "int") {
				//二进制，float 存储
				vector<int> data;
				for (auto &p : op->values) {
					if (p.is_number()) {
						data.push_back(p.get<int>());
					}
					else {
						data.push_back(0);
					}
				}
				// 添加字段
				bd->binData.componentType = "INT";
				bd->binData.type = "SCALAR";
				bd->binData.data.resize(sizeof(int) * data.size());
				//数据拷贝
				memcpy(&bd->binData.data[0], &data[0], bd->binData.data.size());
				b3dm.batchDatas.push_back(bd);
			}
			else if (param.type == "double") {
				//二进制，byte 存储
				vector<double> data;
				for (auto &p : op->values) {
					if (p.is_number()) {
						data.push_back(p.get<double>());
					}
					else {
						data.push_back(0);
					}
				}
				// 添加字段
				bd->binData.componentType = "DOUBLE";
				bd->binData.type = "SCALAR";
				bd->binData.data.resize(sizeof(double) * data.size());
				//数据拷贝
				memcpy(&bd->binData.data[0], &data[0], bd->binData.data.size());
				b3dm.batchDatas.push_back(bd);
			}
			else if (param.type == "bool") {
				//添加字符串字段
				vector<BYTE> data;
				for (auto &p : op->values) {
					if (p.is_number_integer()) {
						data.push_back(p.get<bool>()?1:0);
					}
					else {
						data.push_back(0);
					}
				}
				// 数据拷贝
				bd->binData.componentType = "BYTE";
				bd->binData.type = "SCALAR";
				bd->binData.data.resize(sizeof(BYTE) * data.size());
				//���ݿ���
				memcpy(&bd->binData.data[0], &data[0], bd->binData.data.size());
				b3dm.batchDatas.push_back(bd);
			}
			else if (param.group == "PG_PHASING") {
				//添加字符串字段
				bd->jsonData = op->values;
				b3dm.batchDatas.push_back(bd);
			}

		}
		 
		try {
			auto istr = glbstm.str();
			if (!b3dm.save(stm, istr)) {

				LOG(ERROR) << "b3dm.save  failed";
				return false;
			}
		}
		catch (std::exception & ex) {
			LOG(ERROR) << "b3dm.save  failed:" <<ex.what();
			return false;
		}
		
		return true;
	}

	//对子场景再次进行分割
	bool SceneOutput::splitSubScene(list<shared_ptr<SubScene>>&scenes) {
		
		if (!subscene->canSplit())
			return false;

		// 处理后的数据量很小，那么没必要分割了
		if(packer->getDataSize() < config->tileMaxDataSize)
			return  false;

		return subscene->split(scenes);
	}
 
	json SceneOutput::save2b3dm() {

		json ret;
		ret["geometricError"] = geometricError;
		ret["refine"] = "REPLACE";

		//场景 包围盒  获得是 b3dm后的  需要转到 全球世界坐标
		osg::BoundingBoxd box = packer->contentBox;
		//如果box 过小，那么扩展一下
		double minV = 0.005;
		if (box.xMin() == box.xMax()) {
			box.xMin() -= minV;
			box.xMax() += minV;
		}
		if (box.yMin() == box.yMax()) {
			box.yMin() -= minV;
			box.yMax() += minV;
		}
		if (box.zMin() == box.zMax()) {
			box.zMin() -= minV;
			box.zMax() += minV;
		} 
		
		//4,本块的矩阵  b3dm里的坐标 * 此矩阵 = 全球世界坐标
		transform = packer->globleBoxCenterENU;
		transformInverse = packer->globleBoxCenterENUInv;

		ret["boundingVolume"] = { { "box", XbsjOsgUtil::toJson(box) } };

		//计算transform
		auto trans = transform;
		if (parent) {
			// 块内坐标 * transform = 块内坐标 * trans * parent->transform
			trans = transform * parent->transformInverse;
		}

		if(!trans.isIdentity())
			ret["transform"] = XbsjOsgUtil::toJson(trans);

		//如果group完全被精简了
		if (!packer->simpled) {

			LOG(WARNING) << "scene group empty";
			return move(ret);
		}
		string file = getOutfile();
		//存储数据
		stringstream buffer;
		bool r = save2b3dm(buffer);
		if (!r) {
			LOG(ERROR) << "save2b3dm failed " << file;
			return move(ret);
		}

		auto istr = buffer.str();
		if (config->storage->saveFile(file, istr))
		{
			LOG(INFO) << "saved :" << file << " geometricError:" << geometricError;
		}
 
		ret["content"] = { { "url",getUrl() } };

		return move(ret);
	}

	string SceneOutput::getPath(string iden) {
		string name = str(index) + ".b3dm";
		auto p = this->parent;
		while (p) {
			name = str(p->index) + iden + name;
			p = p->parent;
		}
		return move(name);
	}

	string SceneOutput::getOutfile() {
#if WIN32
		return getPath("\\");
#else
		return getPath("/");
#endif
	}

	string SceneOutput::getUrl() {
		return getPath("/");
	}

}