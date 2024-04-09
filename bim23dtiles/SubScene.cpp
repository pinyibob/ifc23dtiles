#include "SubScene.h" 
#include "BintreeSplitor.h" 

namespace  XBSJ
{

	SubScene::SubScene(ModelInput * in) :input(in)
	{

	}


	SubScene::~SubScene()
	{
	}
	 
	bool  SubScene::canSplit() {

		// 如果只有一个mesh，那么不可分割
		if (meshes.size() <= 1)
			return false;
		//如果当前分割是node，如果场景中都属于一个 node 了那么不可分割
		if (input->splitUnit == "node") {
			BimElement * ele = nullptr;
			bool  inoneNode = true;
			for (auto & m : meshes) {
				if (!ele)
					ele = m.element;
				else if (ele != m.element)
				{
					inoneNode = false;
					break;
				}
			}
			if (inoneNode)
				return false;
		}
		
		//如果只有一个材质了，如果分割后的 数据增量  超过了最大阈值，那就 得不偿失了
		// aftersplit = materialsize * 2 + (datasize  - materialsize)
		// aftersplit - dataSize > maxsize 
		// 也就是说 materialsize > maxsize
		if (usedMaterial.size() == 1) {
			auto & mat = *usedMaterial.begin();
			if (mat->diffuseTexture) {
				size_t materialsize = mat->diffuseTexture->datasize;
				if (materialsize >= input->splitMaxDataSize) {
					return false;
				}
			}
		}
		return  true;
	}
	//添加mesh
	bool  SubScene::addMesh(BimElement * element, ModelMesh * mesh) {

		if (!element || !mesh || !mesh->HasFaces())
			return false;
		//计算这块mesh的最小几何误差
		auto mat = mesh->material;

		auto texture = mat->diffuseTexture;

		double geometricError = FLT_MAX;
		double triangleError = FLT_MAX;
		double maxTriangleError = 0;

		osg::BoundingBoxd meshbox;

		bool uvBetween01 = true;
		//遍历所有三角形
		for (size_t i = 0; i < mesh->indices.size() / 3; i++) {
			 

			auto v0 =  mesh->vertexes[mesh->indices[3 * i + 0]];
			auto v1 = mesh->vertexes[mesh->indices[3 * i + 1]];
			auto v2 =  mesh->vertexes[mesh->indices[3 * i + 2]];

			osg::BoundingBoxd b;
			b.expandBy(v0); b.expandBy(v1); b.expandBy(v2);

			meshbox.expandBy(b);

			//如果包围盒为0，表示是一个异常值，那么不处理
			if (b.radius() <= 0)
				continue;
			//直接求两者的最小值
			triangleError = fmin(triangleError, b.radius());
			geometricError = fmin(geometricError, b.radius());
			maxTriangleError = fmax(maxTriangleError, b.radius());

			//如果有纹理 计算纹理的几何误差
			if (!texture || !mesh->HasTextureCoords())
				continue;

			auto t0 = mesh->textcoords[mesh->indices[3 * i + 0]];
			auto t1 = mesh->textcoords[mesh->indices[3 * i + 1]];
			auto t2 = mesh->textcoords[mesh->indices[3 * i + 2]];


			t0.x() *= texture->width; t0.y() *= texture->height;  
			t1.x() *= texture->width; t1.y() *= texture->height;  
			t2.x() *= texture->width; t2.y() *= texture->height;  

			//计算纹理的像素坐标
			osg::BoundingBoxd tb;
			tb.expandBy(osg::Vec3d(t0.x(), t0.y(), 0)); tb.expandBy(osg::Vec3d(t1.x(), t1.y(), 0)); tb.expandBy(osg::Vec3d(t2.x(), t2.y(), 0));



			if (tb.xMax() > texture->width || tb.xMin() < 0)
				uvBetween01 = false;
			if (tb.yMax() > texture->height || tb.yMin() < 0)
				uvBetween01 = false;

			//我们知道他贴了多大像素范围
			double pixelSize = tb.radius();
			if (pixelSize <= 0)
				continue;

			//  我们设定4像素为几何误差映射的米
			double gm = input->textureGeometricErrorFactor * b.radius() / pixelSize;
			geometricError = fmin(geometricError, gm);

		}

		if (geometricError == FLT_MAX)
			return false;

		//添加到列表中
		SubSceneMesh smesh;
		smesh.geometricError = geometricError;
		smesh.triangleError = triangleError;
		smesh.maxTriangleError = maxTriangleError;
		smesh.material = mat.get();
		smesh.mesh = mesh;
		smesh.element = element;
		smesh.box = meshbox;
		smesh.uvBetween01 = uvBetween01;

		smesh.geometricDatasize = mesh->vertexes.size() * sizeof(osg::Vec3f);
		if(mesh->HasNormals())
			smesh.geometricDatasize += mesh->vertexes.size() * sizeof(osg::Vec3f);
		if(texture && mesh->HasTextureCoords())
			smesh.geometricDatasize += mesh->vertexes.size() * sizeof(osg::Vec2f);
 

		smesh.geometricDatasize += mesh->indices.size() * sizeof(unsigned short)  ;

		meshes.push_back(smesh);
 
		//LOG_IF(INFO, meshbox.radius()>1000) << "bigMesh:" << meshbox.radius() << node->mName.C_Str();
		LOG_IF(INFO, meshbox.center().x() < -10000) << "bigMesh:" << meshbox.center().x() << element->name;
		return true;
	}
	//更新数据总量
	void SubScene::update() {

		dataSize = 0;
		usedMaterial.clear();
		boundingBox = osg::BoundingBoxd();
		minGeometricError = FLT_MAX;
		maxGeometricError = 0;

		for (auto & m : meshes) {
			//使用到的纹理
			usedMaterial.insert(m.material);

			//计算几何数据大小
			dataSize += m.geometricDatasize;

			//更新包围盒大小
			boundingBox.expandBy(m.box);

			//更新最小几何误差
			minGeometricError = fmin(m.geometricError, minGeometricError);
			maxGeometricError = fmax(m.maxTriangleError, maxGeometricError);
		}

		//计算材质大小
		for (auto a : usedMaterial) {
			if(a->diffuseTexture)
				dataSize += a->diffuseTexture->datasize;
		}
 
		return ;
	}
	//按材质分割为两部分
	bool SubScene::splitWithMaterial(shared_ptr<SubScene> & child0, shared_ptr<SubScene> & child1) {
		if (meshes.size() <= 1)
			return false;
		if(usedMaterial.size() <= 1)
			return false;

		//构造分类数据
		list<SplitScalar> splitors;
		for (auto  m : usedMaterial) {
			
			SplitScalar ss;
			ss.ref = m;
			ss.value = 0;

			//本身数据量 
			if (m->diffuseTexture)
				ss.value += m->diffuseTexture->datasize;

			//引用该材质的所有mesh的几何数据量
			for (auto & mesh : meshes) {
				if (mesh.material == m)
					ss.value += mesh.geometricDatasize;
			}
			splitors.push_back(ss);
		}
		//分割器分割， 如果失败
		list<SplitScalar> s0, s1;
		BintreeSplitor bs;
		if (!bs.split(splitors, s0, s1)) {

			LOG(ERROR) << "BintreeSplitor datasize   failed";
			return false;
		}

		//分割成功，那么收集子场景
		child0 = make_shared<SubScene>(input);
		child1 = make_shared<SubScene>(input);
		for (auto s : s0) {
			ModelMaterial * mat = (ModelMaterial *)s.ref;
			for (auto & mesh : meshes) {
				if (mesh.material == mat)
				{
					child0->meshes.push_back(mesh);
				}
			}
		}
		for (auto s : s1) {
			ModelMaterial * mat = (ModelMaterial *)s.ref;
			for (auto & mesh : meshes) {
				if (mesh.material == mat)
				{
					child1->meshes.push_back(mesh);
				}
			}
		}
		child0->update();
		child1->update();
		return true;

	}
	//按空间分割为两部分
	bool SubScene::splitWithSpace(list<shared_ptr<SubScene>> &children) {
		if(meshes.size() <= 1)
			return false;


		//这里来一个简单的八叉树分割
		shared_ptr<SubScene> octree[8];
		auto center = this->boundingBox.center();
		auto halfsize = this->boundingBox.radius() * 0.5;

		for (auto & m : meshes) {
			auto tc = m.box.center();

			int idx = tc.x() <= center.x() ? 0 : 1;
			int idy = tc.y() <= center.y() ? 0 : 1;
			int idz = tc.z() <= center.z() ? 0 : 1;

			int id = idx * 4 + idy * 2 + idz;
			auto & child = octree[id];
			if (!child) {
				child = make_shared<SubScene>(input);
			}
			child->meshes.push_back(m);
		}

		for (size_t i = 0; i < 8; i++) {
			auto & s = octree[i];
			if (s) {
				s->update();
				children.push_back(s);
			}
		}

		//这里判定，如果children为1，那么返回失败
		if (children.size() <= 1) {
			children.clear();
			return false;
		}

		return true;
	 
	}

	bool SubScene::split(list<shared_ptr<SubScene>> &children) {
		if (meshes.size() <= 1)
			return false;


		if (input->splitPriority == "material") {
			shared_ptr<SubScene>  child0;
			shared_ptr<SubScene>  child1;
			if (splitWithMaterial(child0, child1))
			{
				children.push_back(child0);
				children.push_back(child1);
				return true;
			}

			//按空间分割
			if (splitWithSpace(children))
				return true;
		}
		else {
			//按空间分割
			if (splitWithSpace(children))
				return true;

			//空间分割失败，那么还是按材质分割
			shared_ptr<SubScene>  child0;
			shared_ptr<SubScene>  child1;
			if (splitWithMaterial(child0, child1))
			{
				children.push_back(child0);
				children.push_back(child1);
				return true;
			}
		}

		return false;
	}

   

	bool SubSceneSplitor::split(ModelInput * input) {
		if (!input)
			return false;

		if (!input->scene )
			return false;

		this->input = input;

		// 如果颗粒度为对象，那么遍历场景 每个对象构造一个subsenes
		if (input->splitUnit == "node") {
			list<shared_ptr<SubScene>> scenes;
			for (auto & child : input->scene->elements) {
				objectSubScene(child.get(), scenes);
			}
			for (auto & s : scenes) {
				treesplit(s);
			}
		}
		//分组为 mesh  或者 none
		else {
			//0, 遍历场景所有mesh，构造超大子场景
			shared_ptr<SubScene> root = make_shared<SubScene>(input);
			for (auto & child : input->scene->elements) {
				meshSubSceneWithChildren(child.get(), root);
			}
			
			root->update();

			//如果颗粒度为 mesh ，那么完全打散
			if (input->splitUnit == "mesh") {
				//对root进行分割
				treesplit(root);
			}
			//分组为none 不分割
			else {
				subscenes.push_back(root);
			}
		}
 
		//统计一下切分结果
		osg::BoundingBoxd allscene;
		for (auto &s : subscenes) {
			LOG(INFO) << "  subscene mesh count:" << s->meshes.size() << " datasize:" << s->dataSize << " error:" << s->minGeometricError << " radius:" << s->boundingBox.radius();
			allscene.expandBy(s->boundingBox);
		}
 
		LOG(INFO) << "split input subscene count:" << subscenes.size()<< " radius:" << allscene.radius();
 
		return !subscenes.empty();
	}

	//判断分块是否结束
	bool  SubSceneSplitor::finished() {

		//每个子场景的大小满足阈值，或者 每个子场景都不可再分割
		for (auto & s : subscenes) {

			//满足大小阈值 跳过
			if (s->dataSize <= input->splitMaxDataSize)
				continue;
			//如果可以分割，那么返回false
			if (s->canSplit())
				return false;
		}

		return true;
	}

	void SubSceneSplitor::objectSubScene(BimElement * node, list<shared_ptr<SubScene>> &sscenes) {
		
		shared_ptr<SubScene> scene = make_shared<SubScene>(input);
		if (meshSubScene(node, scene)) {
			scene->update();
			sscenes.push_back(scene);
		}

		//遍历所有子
	//	for (auto & child: node->children) {
	//		 
	//		objectSubScene(child.get(), sscenes);
	//	}

	}
	

	void SubSceneSplitor::meshSubSceneWithChildren(BimElement * element, shared_ptr<SubScene> &subscene) {
		if (!element)
			return;

		//本node的mesh
		meshSubScene(element, subscene);

		//遍历所有子
	//	for (auto & child: element->children) {
			 
		//	meshSubSceneWithChildren(child.get(), subscene);
	//	}
	}

	bool  SubSceneSplitor::meshSubScene(BimElement * element, shared_ptr<SubScene> &subscene) {
		if (!element)
			return false;

		//本node所有mesh
		for (auto & mesh : element->meshes) {
			 
			subscene->addMesh(element, mesh.get());
		}
		
		return !subscene->meshes.empty();
		
	}
 
	bool compSubScene(shared_ptr<SubScene> & t1, shared_ptr<SubScene> &  t2) {
		return  t1->dataSize > t2->dataSize;
	}

	 
	void SubSceneSplitor::treesplit(shared_ptr<SubScene> scene) {

		if (scene->meshes.empty())
			return;
		//数据量小，直接添加
		if (scene->dataSize <= input->splitMaxDataSize) {
			subscenes.push_back(scene);
			return;
		}
	 
		//如果不可分割，直接添加
		if (!scene->canSplit()) {
			//LOG(INFO) << "!canSplit";
			subscenes.push_back(scene);
			return;
		}

		//分割失败
		list<shared_ptr<SubScene>> children;
		if (!scene->split(children)) {
			//LOG(INFO) << "!split";
			subscenes.push_back(scene);
			return;
		}

		//递归分割
		for (auto & s : children) {
			treesplit(s);
		}
		  
	}
	
}