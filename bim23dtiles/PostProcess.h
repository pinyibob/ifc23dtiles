#pragma once
#include <osg/Matrix>
#include <osg/BoundingBox>
#include "stdafx.h"

namespace XBSJ {

	class SceneOutputConfig;
	class PostProcessTile {
	public:
		osg::BoundingBoxd box;
		json     tilejson;
	};
	typedef list<shared_ptr<PostProcessTile>> PostTiles;


	class PostProcess;
	class PostProcessTreeNode {

	public:
		PostProcessTreeNode(osg::BoundingBoxd b, PostProcess * process);

		PostProcessTreeNode(osg::BoundingBoxd b, PostProcessTreeNode * parent, int index);

		//从父传递过来的构造包围盒
		osg::BoundingBoxd boxInit;

		//实际的数据范围包围盒
		osg::BoundingBoxd boxContent;

		//父
		PostProcessTreeNode *  parent = nullptr;

		PostProcess * postprocess = nullptr;
		//四个子块
		shared_ptr<PostProcessTreeNode> children[8];

		//data
		PostTiles tiles;

		void add(shared_ptr<PostProcessTile> & tile);

		double tilesize = 0;

		int  index = 0;

		osg::Vec3d center;

		json  process();

		string getPath(string iden);
	};

	class PostProcess
	{
	public:
		PostProcess(SceneOutputConfig * cfg);
		~PostProcess();

	public:
		json process(PostTiles&  tiles);
	 
		osg::BoundingBoxd box;

		SceneOutputConfig * config = nullptr;
	private:
		shared_ptr<PostProcessTreeNode> root;
		
	public:
 
		//3，计算json子节点个数
		static int childrenCount(json & n);

		//中心enu偏移矩阵 模型坐标乘以 此矩阵得到 全球坐标
		osg::Matrix       globleBoxCenterENU;
		//逆矩阵   全球坐标 乘以此 矩阵 得到 模型坐标
		osg::Matrix       globleBoxCenterENUInv;

	
	};
}


