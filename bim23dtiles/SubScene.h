#pragma once
#include "stdafx.h"

#include "ModelInput.h"
#include <osg/BoundingBox>
#include <set>
using namespace std;

namespace  XBSJ
{  

	//对原始场景的引用
	struct SubSceneMesh { 

		//仅仅是 三角网
		double triangleError = 0;

		//三角网的最大几何误差
		double maxTriangleError = 0;

		//包含 三角网 和  贴图的几何误差
		double geometricError = 0;
		//用到的材质
		ModelMaterial * material = nullptr;
		//几何数据量
		size_t  geometricDatasize = 0;

		//包围盒
		osg::BoundingBoxd box;

		bool uvBetween01 = true;

		BimElement * element = nullptr;
		ModelMesh * mesh = nullptr;
	};

	//子场景
	class SubScene
	{
	public:
		SubScene(ModelInput * input);
		~SubScene();


	public:
		
		//所有mesh
		list<SubSceneMesh> meshes;
		//子场景包围盒
		osg::BoundingBoxd boundingBox;
		//子场景数据量
		size_t       dataSize = 0;
		//使用到的材质
		set<ModelMaterial*> usedMaterial;
		//最小几何误差
		double            minGeometricError = 0;

		//最大几何误差
		double            maxGeometricError = 0;

		ModelInput * input = nullptr;
	public:
		bool  canSplit(); 
		bool  addMesh(BimElement * element, ModelMesh * m);
		void update();

		//按材质分割为两部分
		bool splitWithMaterial(shared_ptr<SubScene> & c0, shared_ptr<SubScene> & c1);
		//按空间分割为两部分
		bool splitWithSpace(list<shared_ptr<SubScene>> &children);
		 
	
		bool split(list<shared_ptr<SubScene>> &children);
 
		
	
	};

	//子场景分割器
	
	class SubSceneSplitor
	{
	public:
		 
		bool split(ModelInput * input);

		list<shared_ptr<SubScene>> subscenes;


	protected:
		ModelInput * input = nullptr;

		//检测分块是否结束
		bool  finished();
 
	 
		//一个node所有mesh放到 subscene
		bool meshSubScene(BimElement * node,  shared_ptr<SubScene> &subscene);

		//一个node所有mesh放到 subscene ，并且它的子也放入
		void meshSubSceneWithChildren(BimElement * node, shared_ptr<SubScene> &subscene);

 
		void objectSubScene(BimElement * node,  list<shared_ptr<SubScene>> &subscenes);

		// 树状分组
		void treesplit(shared_ptr<SubScene> scene);
 
	    
	};


}
