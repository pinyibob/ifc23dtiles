#include "SubScenePacker.h"
#include "SubScene.h"
#include "VcgUtil.h"
#include "ModelInput.h"
#include "BintreeSplitor.h"
#include "TexturePacker.h"
#include "util.h"
#include "SceneOutputConfig.h"
#include "OsgUtil.h"
namespace XBSJ {

	SubScenePacker::SubScenePacker(shared_ptr<SubScene> s) {
		subscene = s;
		for (auto & mesh : subscene->meshes) {
			srcBox.expandBy(mesh.box);
		}

		input = s->input;
	}
	void  SubScenePacker::materialPack() {

		//2, 遍历所有材质的纹理，进行合并
		for (auto & k : usedMaterial) {

			auto ptex = k.material->diffuseTexture;
			//没有纹理跳过
			if (!ptex)
				continue;

			//scale = 0 表示没有引用到
			double scale = k.textureScale;
			if (scale == 0)
				continue;

			/*
			if (ptex && ptex->path.find("JN10-28.jpg") != string::npos) {
				LOG(INFO) << "debug";
			}*/

			//如果纹理坐标在0~1 范围内 并且不是 repeat类型的，才可以组合打包
			if (k.uvBetween01  /*&& ptex->mode_s != aiTextureMapMode_Wrap  && ptex->mode_t != aiTextureMapMode_Wrap */) {
				//判断是否有合适位置，如果没有，那么新建打包
				int idx = -1;
				for (auto &pt : packedTextures) {
					idx++;
					auto rect = pt->packTexture(ptex, scale);
					if (rect.w == 0 || rect.h == 0) {
						continue;
					}
					k.packedTexture = idx;
					k.packedRect = rect;
					k.hasalpha = pt->hasalpha;
					break;
				}
				//如果已经打包 跳出
				if (k.packedTexture >= 0) {
					continue;
				}

				//新建一个打包 打包本文件
				shared_ptr<TexturePacker> packer = make_shared<TexturePacker>();
				k.packedRect = packer->packTexture(ptex, scale);
				k.packedTexture = packedTextures.size();
				k.hasalpha = packer->hasalpha;
				packedTextures.push_back(packer);
			}
			//其他情况，直接新建打包	
			else {
				shared_ptr<TexturePacker> packer = make_shared<TexturePacker>(ptex, scale);
				k.packedRect.w = k.packedRect.h = 1;
				k.packedTexture = packedTextures.size();
				k.hasalpha = packer->hasalpha;
				//这里强制设置为repteat
				packer->mode_s = ModelTextureMode_Wrap;
				packer->mode_t = ModelTextureMode_Wrap;
				packedTextures.push_back(packer);
			}


		}

		LOG(INFO) << "packedTextures:" << packedTextures.size();
		//2, 遍历所有材质，进行合并
		for (auto & k : usedMaterial) {

			auto & other = k.material;

			//寻找参数相同的材质
			int idx = -1;
			for (auto &pt : packedMaterials) {
				idx++;
				//打包后的纹理不同，认为不同
				if (pt->packedTexture != k.packedTexture)
					continue;
				if (pt->doubleSide != other->doubleSide)
					continue;
				if (pt->shinniness != other->shinniness)
					continue;
				if (pt->diffuse != other->diffuse)
					continue;
				if (pt->ambient != other->ambient)
					continue;
				if (pt->specular != other->specular)
					continue;
				if (pt->emissive != other->emissive)
					continue;
				if (pt->customShader != other->customShader)
					continue;
				if (pt->nolight != other->nolight)
					continue;

				k.packedMaterial = idx;
				break;
			}
			//已经找到
			if (k.packedMaterial >= 0) {
				continue;
			}

			//新建一个 并加入列表
			shared_ptr<PackedMaterial> pm = make_shared<PackedMaterial>();
			pm->packedTexture = k.packedTexture;
			pm->hasalpha = k.hasalpha;

			pm->doubleSide = other->doubleSide;
			pm->shinniness = other->shinniness;
			pm->diffuse = other->diffuse;
			pm->ambient = other->ambient;
			pm->emissive = other->emissive;
			pm->specular = other->specular;
			pm->customShader = other->customShader;
			pm->nolight = other->nolight;
			pm->metallicFactor = other->metallicFactor;
			pm->roughnessFactor = other->roughnessFactor;


			k.packedMaterial = packedMaterials.size();
			packedMaterials.push_back(pm);

		}

		LOG(INFO) << "packedMaterials:" << usedMaterial.size() << "/" << packedMaterials.size();
	}
	SubScenePacker::UsedMaterial SubScenePacker::getMeshMaterial(shared_ptr<SimpledMesh> mesh) {

		for (auto &u : usedMaterial) {
			if (u.material == mesh->srcmesh->material)
				return u;
		}
		LOG(ERROR) << "empty usedmaterial ";
		return SubScenePacker::UsedMaterial();
	}
	void  SubScenePacker::addUsedMaterial(shared_ptr<SimpledMesh> mesh) {


		//遍历
		for (auto & u : usedMaterial) {
			if (u.material == mesh->srcmesh->material) {
				u.textureScale = fmax(u.textureScale, mesh->maxTextureScale);
				u.uvBetween01 &= mesh->srcmesh->uvBetween01;
				return;
			}
		}
		UsedMaterial um;
		um.material = mesh->srcmesh->material;
		um.textureScale = mesh->maxTextureScale;
		um.uvBetween01 = mesh->srcmesh->uvBetween01;
		usedMaterial.push_back(um);
		return;
	}


	bool SubScenePacker::simply(double geometric, SceneOutputConfig * config) {

		//1， 获取所有包围盒
		auto center = srcBox.center();

		LOG(INFO) << "box center:" << center.x() << "," << center.y() << "," << center.z() << " radius:" << srcBox.radius();
		//使用input 把这个模型坐标 变换到全球坐标
		center = input->toGloble(center);
		//构造此位置的enu
		globleBoxCenterENU = XbsjOsgUtil::eastNorthUpMatrix(center);
		//计算enu的逆矩阵
		globleBoxCenterENUInv = osg::Matrix::inverse(globleBoxCenterENU);

		//2,遍历子场景拷贝原始三角网，做大小筛选、纹理缩放比率计算
		simpled = copySrc(subscene, geometric, config);
		if (!simpled || simpled->meshs.empty())
			return true;

		///3, 如果打包前精简
		if (input->simplifyMesh == "before") {
			for (auto & m : simpled->meshs) {
				 someSimpled |= meshSimplify(m, geometric*geometric* config->simplifyFactor);
			}
		}

		//4,对材质进行打包
		materialPack();
		//5,对mesh打包
		meshPack();
		//6, 如果在打包前没有精简，那么这里精简
		if (input->simplifyMesh == "after") {
			for (auto & m : packedMeshs) {
				someSimpled |= meshSimplify(m, geometric*geometric* config->simplifyFactor);
			}
		}

		//7, 这里需要对mesh再进行处理，去除为空的mesh，统计各个材质的引用计数，对于引用计数为0的材质也删除
		map<unsigned int, bool> materialRef;

		for (auto iter = packedMeshs.begin(); iter != packedMeshs.end();) {
			auto & mesh = *iter;
			if (mesh->position.empty() || mesh->indices.empty()) {
				iter = packedMeshs.erase(iter);

				LOG(INFO) << "empty packed mesh";
			}
			else {
				//表示这个材质用到了
				materialRef[mesh->packedMaterial] = 1;

				iter++;
			}
		}
		//8, 然后重新调整精简后的mesh材质引用 
		//一个原始序号到最终删减后序号映射
		map<unsigned int, unsigned int> afterDelete;
		unsigned int idx = 0;
		for (auto iter = packedMaterials.begin(); iter != packedMaterials.end(); idx++) {
			//如果材质没有被任何mesh引用，那么需要删除
			if (materialRef.find(idx) == materialRef.end()) {
				iter = packedMaterials.erase(iter);
				LOG(INFO) << "empty ref material";
			}
			else {
				iter++;
				afterDelete[idx] = afterDelete.size();
			}
		}
		//9, 重新调整 mesh中的材质引用
		for (auto p : packedMeshs) {
			auto smat = p->packedMaterial;
			if (afterDelete.find(smat) == afterDelete.end()) {
				LOG(WARNING) << "error material idx";
			}
			else {
				p->packedMaterial = afterDelete[smat];
			}
		}

		return unVisibleMesh || someSimpled;
	}

	shared_ptr<SimpledSubScene>  SubScenePacker::copySrc(shared_ptr<SubScene> scene, double geometric, SceneOutputConfig * config) {


		//遍历所有mesh去精简
		shared_ptr<SimpledSubScene> ret = make_shared<SimpledSubScene>();
		for (auto & m : scene->meshes) {
			auto sm = copySrc(scene, &m, geometric, config);
			if (sm) {
				ret->meshs.push_back(sm);

				addUsedMaterial(sm);
			}
		}

		if (ret->meshs.empty())
			return nullptr;

		return move(ret);
	}

	shared_ptr<SimpledMesh> SubScenePacker::copySrc(shared_ptr<SubScene> scene, SubSceneMesh * smesh, double geometric, SceneOutputConfig * config) {

	
		//1, 如果要求的误差大于本块的包围半径，那么返回null

		if (geometric > smesh->box.radius())
		{
			unVisibleMesh = true;
			auto mesh = smesh->mesh;
			//也需要计算实际包围盒
			for (auto & v : mesh->vertexes) {
				auto p = transformVertex(v);
			}
		 
			return nullptr;
		}

		//2,拷贝顶点和三角面数据
		shared_ptr<SimpledMesh> sm = make_shared<SimpledMesh>();
		sm->srcmesh = smesh;

		auto mesh = smesh->mesh;
		sm->vertexs.reserve(mesh->vertexes.size());
		//遍历增加所有顶点
		for (unsigned int i = 0; i < mesh->vertexes.size(); i++) {
			auto v = mesh->vertexes[i];
 
			auto p = transformVertex(v);
 
			SimpledMesh::Vertex vv;
			vv.v = p;
			vv.idx = i;
			sm->vertexs.push_back(vv);
		}

		//填充所有三角面
		//添加索引
		for (unsigned int i = 0; i < mesh->indices.size() / 3; i++) {
		 
			SimpledMesh::Face sf;
			sf.i0 = mesh->indices[3 * i + 0];
			sf.i1 = mesh->indices[3 * i + 1];
			sf.i2 = mesh->indices[3 * i + 2];
			sm->faces.push_back(sf);
		}




		//3, 如果材质没有纹理，那么返回
		auto mat = sm->srcmesh->material;
		if (!mat->diffuseTexture || !mesh->HasTextureCoords()) {

			return move(sm);
		}

		//4，如果误差小于本mesh几何误差，那么直接缩放设置为1
		if (geometric <= smesh->geometricError) {
			sm->maxTextureScale = 1;
			return move(sm);
		}


		auto texture = mat->diffuseTexture;

		//5,判断纹理的缩放比率  
		sm->maxTextureScale = 0;

		for (auto & f : sm->faces) {
			auto s0 = sm->vertexs[f.i0];
			auto s1 = sm->vertexs[f.i1];
			auto s2 = sm->vertexs[f.i2];

			osg::BoundingBoxd b;
			b.expandBy(s0.v); b.expandBy(s1.v); b.expandBy(s2.v);

			//取得纹理坐标
			auto t0 = mesh->textcoords[f.i0];
			auto t1 = mesh->textcoords[f.i1];
			auto t2 = mesh->textcoords[f.i2];


			t0.x() *= texture->width; t0.y() *= texture->height;
			t1.x() *= texture->width; t1.y() *= texture->height;
			t2.x() *= texture->width; t2.y() *= texture->height;

			//计算纹理的像素坐标
			osg::BoundingBoxd tb;
			tb.expandBy(osg::Vec3d(t0.x(), t0.y(), 0)); tb.expandBy(osg::Vec3d(t1.x(), t1.y(), 0)); tb.expandBy(osg::Vec3d(t2.x(), t2.y(), 0));






			//我们知道他贴了多大像素范围
			double pixelSize = tb.radius();

			if (pixelSize <= 0)
				continue;

			// double gm = 4 * b.radius() / pixelSize;
			// ps = 4 * b.radius() / geometricError;
			// scale = ps / pixelSize;

			sm->maxTextureScale = fmax(sm->maxTextureScale, scene->input->textureGeometricErrorFactor * b.radius() / geometric / fmax(texture->width, texture->height));
		}

		sm->maxTextureScale = fmin(1, sm->maxTextureScale);

 
		someSimpled |=  sm->maxTextureScale < 0.8 ;
		


		return move(sm);
		
	}


	size_t SubScenePacker::getDataSize() {
		if (!simpled)
			return 0;
		return simpled->getDataSize();
	}

	size_t SimpledSubScene::getDataSize() {
		size_t size = 0;
		for (auto s : meshs) {

			size += s->getDataSize();

		}

		return size;
	}

	size_t SimpledMesh::getDataSize() {

		
		auto mesh = srcmesh->mesh;

		auto texture = srcmesh->material->diffuseTexture;


		size_t geometricDatasize = 0;
		geometricDatasize = vertexs.size() * sizeof(osg::Vec3f);
		if (mesh->HasNormals())
			geometricDatasize += vertexs.size() * sizeof(osg::Vec3f);
		if (texture && mesh->HasTextureCoords())
			geometricDatasize += vertexs.size() * sizeof(osg::Vec2f);
 
		geometricDatasize += vertexs.size() * sizeof(unsigned short) * 3;

		return geometricDatasize;
	
		
		return 0;
	}


	int  SubScenePacker::getElementFeatureId(BimElement * element) {
		auto f = element2feature.find(element);
		if (f != element2feature.end())
			return f->second;

 
		//名称，id
		elementNames.push_back(element->name);
		elementIds.push_back(element->id);
		elementSIds.push_back(element->sid);
 
		//文件名
		if (input->saveFileName)
			fileNames.push_back(input->fileName);

		//其他属性
		for (auto & p : element->propertes) {

			//寻找当前此属性的列表，如果不存在，那么添加
			bool finded = false;
			for (auto & op : elementProps) {
				if (op->paramindex == p.paramIndex) {
					op->values.push_back(p.value);
					finded = true;
					break;
				}
			}

			//如果没找到对应字段，那么添加所有
			if (!finded) {
				shared_ptr<OtherProps> op = make_shared<OtherProps>();
				op->paramindex = p.paramIndex;
				//先添加前面缺失的
				while (op->values.size() < elementNames.size() - 1) {
					op->values.push_back(json());
				}
				op->values.push_back(p.value);
				elementProps.push_back(op);
			}
		}

		//到了这部，可能有些字段的属性个数不够 那么补全
		for (auto & op : elementProps) {
			while (op->values.size() < elementNames.size()) {
				op->values.push_back(json());
			}
		}

		int idx = elementIds.size() - 1;

		element2feature[element] = idx;


		return idx;
	}

	void  SubScenePacker::meshPack() {

		if (!simpled)
			return;

		
		//遍历所有简化后的场景去处理
		for (auto &m : simpled->meshs) {
		 

			//获取序号
			int featureId = getElementFeatureId(m->srcmesh->element);


			//打包几何体
			meshPack(m, featureId);
		}

		
		LOG(INFO) << "packed mesh:" << simpled->meshs.size() << "/" << packedMeshs.size();
	}




	bool SubScenePacker::meshPack(shared_ptr<SimpledMesh> mesh, int featid) {

		
		 

		auto usedmaterial = getMeshMaterial(mesh);

		//寻找能合并的 
		shared_ptr<PackedMesh>  currentMesh;
		for (auto & p : packedMeshs) {

			//如果打包后的材质不相同，那么不能打包
			if (p->packedMaterial != usedmaterial.packedMaterial)
				continue;
			//无光照，不判定法向量
			if (!usedmaterial.material->nolight)
			{
				if (p->hasNormal() != mesh->srcmesh->mesh->HasNormals())
					continue;
			}

			//如果材质有纹理，那么还需要判定是否有纹理坐标
			if (usedmaterial.material->diffuseTexture) {
				if (p->hasTexcoord() != mesh->srcmesh->mesh->HasTextureCoords())
					continue;
			}

			currentMesh = p;
			break;
		}

		if (!currentMesh) {
			currentMesh = make_shared<PackedMesh>();
			packedMeshs.push_back(currentMesh);
			currentMesh->minGeometricError = mesh->srcmesh->geometricError;
			currentMesh->packedMaterial = usedmaterial.packedMaterial;
		}
		else {
			currentMesh->minGeometricError = fmin(currentMesh->minGeometricError, mesh->srcmesh->geometricError);
		}
		currentMesh->dataseize += mesh->srcmesh->geometricDatasize;
		//处理mesh的顶点

		int voffset = currentMesh->position.size();
		//保存顶点信息
		for (auto & v : mesh->vertexs) {
			auto p = v.v;
			currentMesh->position.push_back(p);

			//处理顶点
			if (currentMesh->position.size() == 1) {
				currentMesh->minPos = currentMesh->maxPos = p;
			}
			else {
				auto &minPos = currentMesh->minPos;
				auto &maxPos = currentMesh->maxPos;

				minPos.x() = fmin(minPos.x(), p.x());
				minPos.y() = fmin(minPos.y(), p.y());
				minPos.z() = fmin(minPos.z(), p.z());

				maxPos.x() = fmax(maxPos.x(), p.x());
				maxPos.y() = fmax(maxPos.y(), p.y());
				maxPos.z() = fmax(maxPos.z(), p.z());
			}

			//处理batchid
			if (featid >= 0)
				currentMesh->batchid.push_back(featid);

			//无光照，不处理法向量
			if (!usedmaterial.material->nolight)
			{
				//处理法向量
				if (mesh->srcmesh->mesh->HasNormals()) {
					auto n = mesh->srcmesh->mesh->normals[v.idx];
					currentMesh->normals.push_back(osg::Vec3f(n.x(), -n.z(), n.y()));
				}
			}
			if (usedmaterial.material->diffuseTexture && mesh->srcmesh->mesh->HasTextureCoords()) {
				auto t = mesh->srcmesh->mesh->textcoords[v.idx];

				//这里根据材质的texture参数调整
				t.x() = usedmaterial.packedRect.x + t.x() *  usedmaterial.packedRect.w;
				t.y() = usedmaterial.packedRect.y + t.y() *  usedmaterial.packedRect.h;
				currentMesh->texcoord.push_back(t);

			}
			else {
				//LOG(WARNING) << "not copy texturecoord";
			}
		
		}
		if ((currentMesh->texcoord.size() != currentMesh->position.size()) &&
			!currentMesh->texcoord.empty()) {

			LOG(WARNING) << "currentMesh->texcoord.size() != currentMesh->position.size()";
		}
		//保存面信息
		for (auto & f : mesh->faces) {
			currentMesh->indices.push_back(f.i0 + voffset);
			currentMesh->indices.push_back(f.i1 + voffset);
			currentMesh->indices.push_back(f.i2 + voffset);
		}
		
		return true;
	}

 


	unsigned int addUniqueIndex2(list<unsigned int> & indexs, unsigned int id) {

		unsigned int cur = 0;
		for (auto i : indexs) {
			if (i == id)
				return cur;
			cur++;
		}

		indexs.push_back(id);
		return indexs.size() - 1;
	}

	void SubScenePacker::vcgMeshFrom(void * vcgm, shared_ptr<SimpledMesh> mesh) {

		 
		//运行到这里，需要用vcg来精简
		MyMesh &vcgmesh = *(MyMesh*)vcgm;
		//增加顶点
		for (auto & p : mesh->vertexs) {
			MyMesh::VertexIterator vi = vcg::tri::Allocator<MyMesh>::AddVertices(vcgmesh, 1);
			vi->P() = MyMesh::CoordType(p.v.x(), p.v.y(), p.v.z());

			if (mesh->srcmesh->mesh->HasNormals()) {
				auto & n = mesh->srcmesh->mesh->normals[p.idx];
				//注意这里交换
				vi->N() = MyMesh::CoordType(n.x(), n.y(),n.z());
			}
		}
		//增加索引  
		for (size_t i = 0; i < mesh->faces.size(); i++) {
			auto &f = mesh->faces[i];
			if (f.i0 == f.i1 || f.i0 == f.i2 || f.i1 == f.i2)
			{
				LOG(WARNING) << "index equals";
				continue;
			}
				
			vcg::tri::Allocator<MyMesh>::AddFace(vcgmesh, f.i0, f.i1, f.i2);
		}

		//增加一个逐点的用户筛选属性 用来存储索引
		MyMesh::PerVertexAttributeHandle<unsigned int> index_data = vcg::tri::Allocator<MyMesh>::GetPerVertexAttribute<unsigned int>(vcgmesh, std::string("index"));
		size_t srcidx = 0;
		for (auto vi = vcgmesh.vert.begin(); vi != vcgmesh.vert.end(); ++vi, ++srcidx) {
			index_data[vi] = srcidx;
		}
	 
	}
	bool SubScenePacker::meshSimplify(shared_ptr<SimpledMesh> mesh, double geometricError) {

		//这里判断几何误差，如果足够小，那么没必要精简了
		if (geometricError < mesh->srcmesh->geometricError)
			return false;
		//这里写一个简单的 ，如果已经是100个三角形了，不再精简
		if (mesh->faces.size() <= 100)
			return false;

		//运行到这里，需要用vcg来精简
		MyMesh vcgmesh;
		 
		vcgMeshFrom(&vcgmesh, mesh);

	 
#ifdef _DEBUG
		//vcg::tri::io::ExporterOBJ<MyMesh>::Save(vcgmesh, "c:\\gisdata\\bim\\ditie\\14.obj", vcg::tri::io::Mask::IOM_VERTCOORD | vcg::tri::io::Mask::IOM_FACEINDEX);
#endif

		// 先按通常的方式精简
		int facecount  = vcgSimply(&vcgmesh, geometricError);

		//这里是没有办法了，加一个检测，如果结果三角形为1，一般意味着平面精简，我们设置个最下三角形个数为2
		if (facecount < 2) {
			vcgmesh.Clear();

			vcgMeshFrom(&vcgmesh, mesh);

			vcgSimply(&vcgmesh, 2);
		}

 
		list<unsigned int> usedVertex;

		//清空原始索引
		mesh->faces.clear();

		for (auto fi = vcgmesh.face.begin(); fi != vcgmesh.face.end(); ++fi)
		{
			auto & face = *fi;
			//如果面被删除，或者不是三角形，那么跳过
			if (face.IsD() || face.VN() != 3)
				continue;

			XBSJ::SimpledMesh::Face  sface;

			//添加每个三角面的索引
			for (int k = 0; k < face.VN(); k++)
			{
				//注意看这里的写法 vcg::tri::Index(mesh, (*fi).V(k)) 获取这个面这个点在mesh中的总索引
				//通过 VertexId 得到 在精简后的序号
				auto idx = vcg::tri::Index(vcgmesh, face.V(k));

				//添加到列表中
				idx = addUniqueIndex2(usedVertex, idx);

				if (k == 0)
					sface.i0 = idx;
				else if (k == 1)
					sface.i1 = idx;
				else
					sface.i2 = idx;


			}
			mesh->faces.push_back(sface);
		}

		size_t srccount = mesh->vertexs.size();

		auto vertexs = move(mesh->vertexs);
 

		MyMesh::PerVertexAttributeHandle<unsigned int> index_data = vcg::tri::Allocator<MyMesh>::GetPerVertexAttribute<unsigned int>(vcgmesh, std::string("index"));

		//重新构造顶点
		for (auto i : usedVertex) {
			auto & vert = vcgmesh.vert[i];

			auto srcidx = index_data[vert];
			if (srcidx >= vertexs.size()) {
				LOG(WARNING) << "error srcidx";
				continue;
			}
			mesh->vertexs.push_back(vertexs[srcidx]);
		}

 
		LOG(INFO) << " simply before packer: " << vcgmesh.face.size() << "/" << mesh->faces.size() << " with geoerror:" << geometricError;

		return srccount > mesh->vertexs.size();
	}


	void SubScenePacker::vcgMeshFrom(void * vcgm, shared_ptr<PackedMesh> mesh) {
		MyMesh & vcgmesh = *(MyMesh*)vcgm;
		//增加顶点
		size_t cur = 0;
		for (auto & p : mesh->position) {
			MyMesh::VertexIterator vi = vcg::tri::Allocator<MyMesh>::AddVertices(vcgmesh, 1);
			vi->P() = MyMesh::CoordType(p.x(), p.y(), p.z());
			if (mesh->hasNormal()) {
				auto & n = mesh->normals[cur];
				vi->N() = MyMesh::CoordType(n.x(), n.y(), n.z());
			}
			cur++;
		}
		//增加索引  
		for (size_t i = 0; i < mesh->indices.size(); i += 3) {
			auto i0 = mesh->indices[i];
			auto i1 = mesh->indices[i + 1];
			auto i2 = mesh->indices[i + 2];
			if (!((i0 != i1) && (i1 != i2) && (i0 != i2))) {
				LOG(WARNING) << "index equals";
				continue;
			}
			vcg::tri::Allocator<MyMesh>::AddFace(vcgmesh, i0, i1, i2);
		}

		//增加一个逐点的用户筛选属性 用来存储索引
		MyMesh::PerVertexAttributeHandle<unsigned int> index_data = vcg::tri::Allocator<MyMesh>::GetPerVertexAttribute<unsigned int>(vcgmesh, std::string("index"));
		size_t srcidx = 0;
		for (auto vi = vcgmesh.vert.begin(); vi != vcgmesh.vert.end(); ++vi, ++srcidx) {
			index_data[vi] = srcidx;
		}

	}
	bool SubScenePacker::meshSimplify(shared_ptr<PackedMesh>  mesh, double geometricError) {


		//如果误差已经小于 mesh的最小误差 那么无需精简
		if (geometricError < mesh->minGeometricError)
			return false;

		//如果已经是两个面了，无需精简
		if (mesh->indices.size() <= 6)
			return false;


		//运行到这里，需要用vcg来精简
		MyMesh vcgmesh;

		vcgMeshFrom(&vcgmesh, mesh);


#ifdef _DEBUG
		vcg::tri::io::ExporterOBJ<MyMesh>::Save(vcgmesh, "c:\\gisdata\\bim\\ditie\\14.obj", vcg::tri::io::Mask::IOM_VERTCOORD | vcg::tri::io::Mask::IOM_FACEINDEX);
#endif

		// 先按通常的方式精简
		int facecount = vcgSimply(&vcgmesh, geometricError);

		//这里是没有办法了，加一个检测，如果结果三角形为1，一般意味着平面精简，我们设置个最下三角形个数为2
		if (facecount < 2) {
			vcgmesh.Clear();

			vcgMeshFrom(&vcgmesh, mesh);

			vcgSimply(&vcgmesh, 2);
		}
 
		list<unsigned int> usedVertex;

		//清空原始索引
		mesh->indices.clear();
		for (auto fi = vcgmesh.face.begin(); fi != vcgmesh.face.end(); ++fi)
		{
			auto & face = *fi;
			//如果面被删除，或者不是三角形，那么跳过
			if (face.IsD() || face.VN() != 3)
				continue;

			//添加每个三角面的索引
			for (int k = 0; k < face.VN(); k++)
			{
				//注意看这里的写法 vcg::tri::Index(mesh, (*fi).V(k)) 获取这个面这个点在mesh中的总索引
				//通过 VertexId 得到 在精简后的序号
				auto idx = vcg::tri::Index(vcgmesh, face.V(k));
				//添加到列表中
				idx = addUniqueIndex2(usedVertex, idx);
				mesh->indices.push_back(idx);
			}
		}

		size_t srccount = mesh->position.size();

		auto position = move(mesh->position);
		auto normals = move(mesh->normals);
		auto texcoord = move(mesh->texcoord);
		auto color = move(mesh->color);
		auto batchid = move(mesh->batchid);

		MyMesh::PerVertexAttributeHandle<unsigned int> index_data = vcg::tri::Allocator<MyMesh>::GetPerVertexAttribute<unsigned int>(vcgmesh, std::string("index"));

		//重新构造顶点
		for (auto i : usedVertex) {
			auto & vert = vcgmesh.vert[i];

			auto srcidx = index_data[vert];
			if (srcidx >= position.size()) {
				LOG(WARNING) << "error srcidx";
				continue;
			}

			mesh->position.push_back(position[srcidx]);

			if (!batchid.empty())
				mesh->batchid.push_back(batchid[srcidx]);
			if (!normals.empty())
				mesh->normals.push_back(normals[srcidx]);
			if (!texcoord.empty())
				mesh->texcoord.push_back(texcoord[srcidx]);
			if (!color.empty())
				mesh->color.push_back(color[srcidx]);
		}


		LOG(INFO) << " simply after packer: " << vcgmesh.face.size() << "/" << mesh->indices.size()/3 << " with geoerror:" << geometricError;

		return true;
	}


	osg::Vec3f   SubScenePacker::transformVertex(osg::Vec3d ss) {

		//先用input 地理变换到 全球世界坐标
		ss = input->toGloble(ss);

		globleBox.expandBy(ss);

		//变换到模型坐标
		ss = ss * globleBoxCenterENUInv;

		contentBox.expandBy(ss);

		return osg::Vec3f(ss.x(), ss.y(), ss.z());
	}

	void SubScenePacker::test() {


		MyMesh vcgmesh;

		int flag = vcg::tri::io::Mask::IOM_VERTCOORD | vcg::tri::io::Mask::IOM_FACEINDEX;
		vcg::tri::io::ImporterOBJ<MyMesh>::Open(vcgmesh, "c:\\gisdata\\bim\\ditie\\14.obj", flag);


		int facecount = vcgSimply(&vcgmesh, 0.2);
		 
		if (facecount < 2) {
			vcgmesh.Clear();

			 
			vcg::tri::io::ImporterOBJ<MyMesh>::Open(vcgmesh, "c:\\gisdata\\bim\\ditie\\14.obj", flag);

			vcgSimply(&vcgmesh, 2);
		}


		list<unsigned int> usedVertex;


		for (auto fi = vcgmesh.face.begin(); fi != vcgmesh.face.end(); ++fi)
		{
			auto & face = *fi;
			//如果面被删除，或者不是三角形，那么跳过
			if (face.IsD() || face.VN() != 3)
				continue;

			XBSJ::SimpledMesh::Face  sface;

			//添加每个三角面的索引
			for (int k = 0; k < face.VN(); k++)
			{
				//注意看这里的写法 vcg::tri::Index(mesh, (*fi).V(k)) 获取这个面这个点在mesh中的总索引
				//通过 VertexId 得到 在精简后的序号
				auto idx = vcg::tri::Index(vcgmesh, face.V(k));

				//添加到列表中
				idx = addUniqueIndex2(usedVertex, idx);

				if (k == 0)
					sface.i0 = idx;
				else if (k == 1)
					sface.i1 = idx;
				else
					sface.i2 = idx;
			}
		}

		vcg::tri::io::ExporterOBJ<MyMesh>::Save(vcgmesh, "c:\\gisdata\\bim\\ditie\\142.obj", vcg::tri::io::Mask::IOM_VERTCOORD | vcg::tri::io::Mask::IOM_FACEINDEX);

		LOG(INFO) << usedVertex.size();
	}

	int SubScenePacker::vcgFaceCount(void * vcgm) {

		MyMesh &vcgmesh = *(MyMesh*)vcgm;

		int count = 0;
		//计算剩余的三角形个数
		for (auto fi = vcgmesh.face.begin(); fi != vcgmesh.face.end(); ++fi)
		{
			auto & face = *fi;
			//如果面被删除，或者不是三角形，那么跳过
			if (face.IsD() || face.VN() != 3)
				continue;

			count++;
		}

		return count;
	}
	int SubScenePacker::vcgSimply(void * vcgm, double geometric) {


		MyMesh &vcgmesh = *(MyMesh*)vcgm;

		//更新拓扑关系
		vcg::tri::UpdateTopology<MyMesh>::VertexFace(vcgmesh);


		vcg::math::Quadric<double> QZero;
		QZero.SetZero();
		QuadricTemp TD(vcgmesh.vert, QZero);
		QHelper::TDp() = &TD;

		vcg::tri::TriEdgeCollapseQuadricParameter pp;
		//pp.PreserveBoundary = true;  //不保证边界，如果保证边界，可能会有一堆零碎三角面
		//pp.BoundaryWeight = 0.5;
		pp.PreserveTopology = true;  //保证拓扑关系   设置这个之后，精简的数量太少了 但是又出了很多异常，所以这个值不能设置为true
		pp.QualityCheck = true;  //质量检查
		pp.NormalCheck = true;  //法向量检测  如果法向量差异太大，不精简点
		pp.NormalThrRad = M_PI / 2; //值越大，实际内部计算的阈值越小，省去的点越少
		pp.ScaleFactor = 1;
		pp.ScaleIndependent = false;
		pp.UseArea = false;
		pp.OptimalPlacement = false;
		//pp.QualityThr = 1.0;

		vcg::LocalOptimization<MyMesh> deciSession(vcgmesh, &pp);
		deciSession.Init<MyTriEdgeCollapse>();

		//因为vcg的误差含义为到面的距离平方，所以传入参数需要两个相乘
		deciSession.SetTargetMetric(geometric);

		//这个是按目标三角网的点个数控制，在这里不适用
		//deciSession.SetTargetSimplices(2);
		//这个函数超级慢
		deciSession.DoOptimization();
		deciSession.Finalize<MyTriEdgeCollapse>();


		return vcgFaceCount(vcgm);

	}
	int SubScenePacker::vcgSimply(void * vcgm, int facecount) {

		MyMesh &vcgmesh = *(MyMesh*)vcgm;

		//更新拓扑关系
		vcg::tri::UpdateTopology<MyMesh>::VertexFace(vcgmesh);


		vcg::math::Quadric<double> QZero;
		QZero.SetZero();
		QuadricTemp TD(vcgmesh.vert, QZero);
		QHelper::TDp() = &TD;

		vcg::tri::TriEdgeCollapseQuadricParameter pp;
		//pp.PreserveBoundary = true;  //不保证边界，如果保证边界，可能会有一堆零碎三角面
		//pp.BoundaryWeight = 0.5;
		pp.PreserveTopology = false;  //保证拓扑关系   设置这个之后，精简的数量太少了 但是又出了很多异常，所以这个值不能设置为true
		pp.QualityCheck = true;  //质量检查
		pp.NormalCheck = true;  //法向量检测  如果法向量差异太大，不精简点
		pp.NormalThrRad = M_PI / 2; //值越大，实际内部计算的阈值越小，省去的点越少
		pp.ScaleFactor = 1;
		pp.ScaleIndependent = false;
		pp.UseArea = false;
		pp.OptimalPlacement = false;
		//pp.QualityThr = 1.0;

		vcg::LocalOptimization<MyMesh> deciSession(vcgmesh, &pp);
		deciSession.Init<MyTriEdgeCollapse>();

		//这个是按目标三角网的点个数控制，在这里不适用
		deciSession.SetTargetSimplices(facecount);
		//这个函数超级慢
		deciSession.DoOptimization();
		deciSession.Finalize<MyTriEdgeCollapse>();


		return vcgFaceCount(vcgm);
	}
}

