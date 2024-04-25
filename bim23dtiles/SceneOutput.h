#pragma once
#include "stdafx.h"

#include <osg/BoundingBox>
#include <osg/Matrix>
namespace XBSJ {

	
	class SubScene;
	class SceneOutputConfig;
	class SubScenePacker;
 
	//场景输出
	class SceneOutput
	{
	public:
		SceneOutput(SceneOutputConfig *config, SceneOutput *parent,size_t idx);
		~SceneOutput();

		bool process(json & content, shared_ptr<SubScene> scene, double geometricError = 0);

		//当前输出的子场景
		shared_ptr<SubScene> subscene;
		//当前块的子场景分组
		shared_ptr<SubScenePacker> packer;

		//全球坐标包围盒
		osg::BoundingBoxd globleBox();
	protected:
		bool process(json & content);

		SceneOutputConfig* config = nullptr;

		//当前块的几何误差
		double geometricError = 0;
	
		//获取url
		string getPath(string iden);
		string getOutfile();
		string getUrl();

		//同级别中序号
		size_t  index = 0;
		//父块
		SceneOutput *parent = nullptr;
		//当前的偏移矩阵 偏移到世界坐标 
		osg::Matrix  transform = osg::Matrix::identity();
		osg::Matrix  transformInverse = osg::Matrix::identity();

		//输出b3dm
		bool save2glb(ostream & stm);
		bool save2b3dm(ostream & stm);
		json save2b3dm();

		bool splitSubScene(list<shared_ptr<SubScene>>&scenes);

	};
}

