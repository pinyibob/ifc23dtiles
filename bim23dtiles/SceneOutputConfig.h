#pragma once
#include "stdafx.h"

#include <osg/BoundingBox>
#include <osg/Matrix>
#include <set>
#include "PostProcess.h"


namespace XBSJ {

	 
	class SubSceneSplitor;
	class ModelInput;

	class TilesStorage;
 
	//输出配置
	class SceneOutputConfig {

	public:
		bool config(json & config);

		//处理一个输入
		bool process(shared_ptr<ModelInput> input,size_t idx);
		//后处理
		bool postProcess();
		//输出目录
		//string outputDir = "";
		
		//递减系数
		double nextGeometricErrorFactor = 0.25;

		//最小几何误差
		double minGeometricError = 0.01;
 
		//三角面精简系数
		double  simplifyFactor = 0.2;

		//包围球半径 到 几何误差的比例系数
		double boxRadius2GeometricError = 1;

		//子块分割阈值
		unsigned int tileMaxDataSize = 20000;

		//是否进行draco压缩
		bool  dracoCompression = false;

		//是否进行crn压缩
		bool  crnCompression = false;
  


		shared_ptr<TilesStorage>  storage = nullptr;

		//保存一个外链的json文件
		json linkJson(string filename, json & child);
		//单独保存一个tileset.json
		bool writeTileset(string file, json & root, json properties = json());
	protected:

		 
		//几何误差比例
		double geometricErrorFactor = 1;
  
 
		PostTiles  ptiles;
 
		 
		json   fileParams;
		std::set<std::string> hasFileParams;
		

		json sceneTree;
	};
 
}

