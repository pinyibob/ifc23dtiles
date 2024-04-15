#include "SceneOutputConfig.h"

#include <ogr_api.h>
#include <ogr_srs_api.h>
#include <ogrsf_frmts.h>

#include "util.h"
#include "Cesium.h"
#include "OsgUtil.h"

#include "SubScenePacker.h"
#include "SceneOutput.h"
#include "ModelInput.h"
#include "SubScene.h"
#include "TilesStorage.h"


namespace XBSJ {

	bool parseFromStr(OGRSpatialReference & ref, char * str) {

		auto  err = ref.SetFromUserInput(str);
		if (!err)
			return true;

		err = ref.importFromWkt(&str);
		if (!err)
			return true;

		err = ref.importFromProj4(str);
		if (!err)
			return true;

		return false;

	}
	bool SceneOutputConfig::config(json & config) {


		auto coutput = config["output"];
		if (coutput.is_null()) {
			LOG(ERROR) << "output config failed";
			return false;
		}
 
		//构造存储器
		storage = TilesStorage::createStorage(coutput, [](string info, TilesStorage::LogLevel level, string file, long line) {

			//google::LogMessage(file.c_str(), (int)line, (int)level).stream() << info;

		});
		if (!storage) {

			LOG(ERROR) << "init storage failed";
			return false;
		}
 
 
		//误差比例
		auto cgeometricErrorFactor = config["geometricErrorFactor"];
		if (cgeometricErrorFactor.is_number()) {
			auto v = cgeometricErrorFactor.get<double>();
			if (v > 0) {
				geometricErrorFactor = v;
			}
		}

		//最大几何误差比例
		auto cboxRadius2GeometricError = config["boxRadius2GeometricError"];
		if (cboxRadius2GeometricError.is_number()) {
			auto v = cboxRadius2GeometricError.get<double>();
			if (v > 0.01 && v< 100) {
				boxRadius2GeometricError = v;
			}
			if (v == 0.0) {
				boxRadius2GeometricError = 0.0;
			}
		}

		
		//最大几何误差比例
		auto cminGeometricError = config["minGeometricError"];
		if (cminGeometricError.is_number()) {
			auto v = cminGeometricError.get<double>();
			if (v > 0.000001) {
				minGeometricError = v;
			}
		}

		//几何误差递减系数
		auto cnextGeometricErrorFactor = config["nextGeometricErrorFactor"];
		if (cnextGeometricErrorFactor.is_number()) {
			auto v = cnextGeometricErrorFactor.get<double>();
			if (v > 0 && v< 0.6) {
				nextGeometricErrorFactor = v;
			}
		}
		


		//最大数据量
		auto ctileMaxDataSize = config["tileMaxDataSize"];
		if (ctileMaxDataSize.is_number_unsigned()) {
			auto v = ctileMaxDataSize.get<unsigned int>();
			if (v >= 2000) {
				tileMaxDataSize = v;
			}
		}
	 
	 
		//几何面精简系数
		auto csimplifyFactor = config["simplifyFactor"];
		if (csimplifyFactor.is_number()) {
			auto v = csimplifyFactor.get<double>();
			if (v > 0) {
				simplifyFactor = v;
			}
		}

		//draco 三角网压缩
		auto cdracoCompression = config["dracoCompression"];
		if (cdracoCompression.is_boolean()) {
			dracoCompression = cdracoCompression.get<bool>();
		}
		//crn  纹理压缩
		auto ccrnCompression = config["crnCompression"];
		if (ccrnCompression.is_boolean()) {
			crnCompression = ccrnCompression.get<bool>();
		}
		

		return true;
	}
 

	bool SceneOutputConfig::process(shared_ptr<ModelInput> input, size_t idx) {

 
		//1, 场景分割
		shared_ptr<SubSceneSplitor> splitor = make_shared<SubSceneSplitor>();
		{
			ProgressHelper pinput("split input", 1, 0.2);
			if (!splitor->split(input.get())) {
				LOG(WARNING) << "splitor input failed";
				return false;
			}
		}
		

		//2，遍历所有子场景，分别去处理

		//下面这个root仅仅是为了组织多个目录
		shared_ptr<SceneOutput>  root = make_shared<SceneOutput>(this, nullptr, idx);

		ProgressHelper pprocess("process input", splitor->subscenes.size(), 0.8);
		int step = 0;
		for (auto &subscene : splitor->subscenes) {
			step++;
			if (subscene->maxGeometricError < minGeometricError)
				continue;

			//构造一个子场景输出
			shared_ptr<SceneOutput>  sceneout = make_shared<SceneOutput>(this, root.get(), step-1);

			//处理子场景
			json sproot;
			if (!sceneout->process(sproot, subscene)) {
				LOG(ERROR) << "sceneout failed";
				continue;
			}

			//子场景结果暂存
			shared_ptr<PostProcessTile> pt = make_shared<PostProcessTile>();
			pt->tilejson = move(sproot);
			pt->box = move(sceneout->globleBox());
			ptiles.push_back(pt);

 			//走下进度
			pprocess.skip(step);
		}
		

	 
		//获得这个input的场景树
		input->fillSceneTree(sceneTree);

		//保存文件属性
		if (hasFileParams.find(input->fileName) == hasFileParams.end()) {
			json ppp;
			ppp["file"] = input->fileName;

			list<json> values;
			for (auto & p : input->scene->params) {
				values.push_back(p.to_json());
			}
			ppp["params"] = values;
			fileParams.push_back(ppp);

			hasFileParams.insert(input->fileName);
		}
 

		return true;
	}
	//后处理
	bool SceneOutputConfig::postProcess() {

		//1, 后处理
		PostProcess  pp(this);
		json root = pp.process(ptiles);
		if (root.is_null()) {
			return false;
		}
		osg::BoundingBoxd rootbox = pp.box;

	 
		//上述的位置已经是全球位置了，无需再设置transform了
		//root["transform"] = globalMatrix.isIdentity() ? XbsjOsgUtil::tianAnMen() : XbsjOsgUtil::toJson(globalMatrix);
		root["refine"] = "REPLACE";

	 

		//保存tileset
		if (!writeTileset("tileset.json", root, fileParams)) {
			LOG(ERROR)<< "保存tileset失败";
			return false;
		}

		//保存场景树		
		storage->saveSceneTree(sceneTree);

		return true;
	}
 
	bool SceneOutputConfig::writeTileset(string file, json & root, json properties ) {

		json tileset;
		tileset["asset"] = { { "version","1.0" },
		{ "generatetool","etop@https://www.szetop.cn/" } ,
		{ "gltfUpAxis","Z" } };
		tileset["root"] = root;

		if (!properties.is_null())
			tileset["properties"] = properties;
		auto rooterror = root["geometricError"].get<double>();
 
		return storage->saveJson(file, tileset);
 
	}
	json SceneOutputConfig::linkJson(string filename, json & child) {
		json fakechild;
		fakechild["geometricError"] = child["geometricError"];
		fakechild["refine"] = child["refine"];
		fakechild["boundingVolume"] = child["boundingVolume"];
		fakechild["content"] = { { "url",filename } };

		//注意这里，fakechild 保存transform，但是原始child 设置我单位矩阵
		auto srctrans = child["transform"];
		fakechild["transform"] = srctrans;

		//注意设置他为单位矩阵
		child.erase("transform");

		//如果保存失败，那么这里记录一下错误
		if (!writeTileset(filename, child)) {

			LOG(ERROR) << "保存tileset" << filename << "失败";
			child["transform"] = srctrans;
			return move(child);
		}

		return move(fakechild);
	}

}