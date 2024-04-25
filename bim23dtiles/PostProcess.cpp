#include "PostProcess.h"
#include "OsgUtil.h" 
#include "util.h"
#include "SceneOutputConfig.h"

namespace XBSJ {
	PostProcess::PostProcess(SceneOutputConfig *cfg)
	{
		config = cfg;
	}


	PostProcess::~PostProcess()
	{
	}

	json PostProcess::process(PostTiles &  tiles)
	{
		json j;

		//计算总范围
		for (auto& t : tiles) {
			box.expandBy(t->box);
		}

		if (!box.valid())
			return move(j);

		// 构造root的enu转换
		auto center = box.center();
		//构造此位置的enu
		globleBoxCenterENU = XbsjOsgUtil::eastNorthUpMatrix(center);
		//计算enu的逆矩阵
		globleBoxCenterENUInv = osg::Matrix::inverse(globleBoxCenterENU);

		root = make_shared<PostProcessTreeNode>(box, this);

		//构造八叉树
		for (auto &t : tiles) {
			root->add(t);
		}

		//处理八叉树
		j = root->process(); 

		//修正root的transform和box
		j["transform"] = XbsjOsgUtil::toJson(globleBoxCenterENU);

		//更新场景树
		return move(j);
	}

	PostProcessTreeNode::PostProcessTreeNode(osg::BoundingBoxd b, PostProcess * pro) {
		boxInit = b;
		index = 0;
		center = boxInit.center();
		postprocess = pro;
		 
		tilesize = boxInit.radius();
	}

	PostProcessTreeNode::PostProcessTreeNode(osg::BoundingBoxd b, PostProcessTreeNode * p, int idx) {

		boxInit = b;
		index = idx;
		center = boxInit.center();
		tilesize = boxInit.radius();
		parent = p;
		postprocess = p->postprocess;
	}
	void PostProcessTreeNode::add(shared_ptr<PostProcessTile> & tile) {

		//本node的实际包围盒范围
		boxContent.expandBy(tile->box);

		//判定，如果这个tile 的半径 已经 大于本快半径的 1/2 表示就放这个Node里
		auto twidth = tile->box.radius();
		
		auto halfsize = tilesize * 0.5;
		if (twidth  >= halfsize) {
			tiles.push_back(tile);
			return;
		}

		//判断落在哪个子块内 就放在哪个子块内
		auto tc = tile->box.center();

		int idx = tc.x() <= center.x() ? 0 : 1;
		int idy = tc.y() <= center.y() ? 0 : 1;
		int idz = tc.z() <= center.z() ? 0 : 1;

		int id = idx * 4 + idy * 2 + idz;
		auto & child = children[id];
		if (!child) {
			osg::BoundingBoxd cbox(
				center.x() + (idx - 1)* halfsize,
				center.y() + (idy - 1)* halfsize,
				center.z() + (idz - 1)* halfsize,
				center.x() + idx * halfsize,
				center.y() + idy * halfsize,
				center.z() + idz * halfsize);
			child = make_shared<PostProcessTreeNode>(cbox, this, id);
		}
		child->add(tile);
 
	}
 

	json  PostProcessTreeNode::process() {

		

		//先处理四个子的

		list<json> jchildren;
		for (int i = 0; i < 8; i++) {
			auto child = children[i];
			if (!child)
				continue;

			auto c = child->process();
			if (c.is_null())
				continue;

			jchildren.push_back(c);
			
		}
		
		
		//如果包含实际tile
		for (auto & t : tiles) {
			if (!t->tilejson.is_null()) {

				auto j = t->tilejson;
				//这里需要修正t->tilejson  因为transform 改了
				auto ct = j["transform"];
				if (!ct.is_null()) {
					auto trans = XbsjOsgUtil::matFromJson(ct);
					trans = trans * postprocess->globleBoxCenterENUInv;
					j["transform"] = XbsjOsgUtil::toJson(trans);
				}
				auto je = j["geometricError"];

				jchildren.push_back(j);
			}
		}
		json j;
		auto url = getPath("/");

		LOG(INFO)<<"post process:"<<url;

		auto box = XbsjOsgUtil::tranformBox(postprocess->globleBoxCenterENUInv, boxContent);
		j["boundingVolume"] = { { "box",XbsjOsgUtil::toJson(box) } };
	  

		double geoError = boxContent.radius()* postprocess->config->boxRadius2GeometricError;

		j["geometricError"] = jchildren.empty() ? 0 : geoError;
		j["refine"] = "REPLACE";
		j["children"] = jchildren;

		int count =  PostProcess::childrenCount(j);
		//如果子节点过多，那么把此节点保存，并且生成一个 引用
		if (count > 100) {

			auto filename = "lab_b_" + getPath("_") + ".json";

			j = postprocess->config->linkJson(filename, j);
		}
		
		//j["content"] = { { "url" , filename } };

		return move(j);
	}

	string PostProcessTreeNode::getPath(string iden) {

		auto p = parent;
		string path = str(index);
		while (p) {
			path = str(p->index) + iden + path;
			p = p->parent;
		};

		return move(path);
	}
 

	int PostProcess::childrenCount(json & n) {
		int count = 1;
		auto children = n["children"];
		if (!children.is_array())
			return count;
		for (auto& c : children) {
			count += childrenCount(c);
		}
		return count;
	}

	 

}