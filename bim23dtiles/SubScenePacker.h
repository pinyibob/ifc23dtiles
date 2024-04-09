#pragma once
#include "stdafx.h"
#include <osg/Vec3d>
#include <osg/Matrix>
#include <osg/BoundingBox>
#include "TexturePacker.h"

namespace XBSJ {

	class SubScene;
	struct SubSceneMesh;
	class ModelMaterial; 
	class SceneOutputConfig;
	class ModelInput;

	//精简后的mesh
	class SimpledMesh {
	public:
		struct Vertex {
			osg::Vec3f v;
			size_t idx = 0; //在原mesh中的序号
		};
		struct Face {
			unsigned int i0;
			unsigned int i1;
			unsigned int i2;
		};
		//精简后顶点
		vector<Vertex> vertexs;
		//三角面
		vector<Face> faces;
		//引用的未精简mesh
		SubSceneMesh * srcmesh;
		size_t getDataSize();
		//应用纹理的最大缩放
		double maxTextureScale = 0; 
	};
	//精简后子场景
	class SimpledSubScene {

	public:
		list<shared_ptr<SimpledMesh>> meshs;
		size_t getDataSize();
	};


	//打包后的mesh
	struct PackedMesh {
		//下面这几个应该和顶点相同
		vector<osg::Vec3f> position;
		vector<osg::Vec3f> normals;
		vector<osg::Vec2f> texcoord;
		vector<unsigned int> color;
		//对象id
		vector<unsigned short> batchid;

		//索引
		vector<unsigned int> indices;

		//最小最大值
		osg::Vec3f minPos;
		osg::Vec3f maxPos;

		//打包后的材质
		int   packedMaterial = -1;

		//是否需要下一级
		double   minGeometricError = 0;

		//合并后数据量
		int    dataseize = 0;

		bool hasNormal() {
			return !normals.empty();
		}
		bool hasTexcoord() {
			return !texcoord.empty();
		}
		bool hasColor() {
			return !color.empty();
		}
	};

	

	//子场景  打包  精简 处理器
	class SubScenePacker {

	public:
		SubScenePacker(shared_ptr<SubScene> s);
		shared_ptr<SubScene> subscene;

		//精简后的数据
		shared_ptr<SimpledSubScene> simpled;


		struct UsedMaterial {

			ModelMaterial * material = nullptr;
			double  textureScale = 0;
			bool  uvBetween01 = true;
			//合并后的纹理序号
			int   packedTexture = -1;
			//合并后的纹理范围
			Rect<float>  packedRect;
			//合并后的材质序号
			int   packedMaterial = -1;

			//纹理里是否有alpha
			bool hasalpha = false;
 
		};

		//打包后的材质
		list<shared_ptr<PackedMaterial>>  packedMaterials;
		//打包后的纹理
		list<shared_ptr<TexturePacker>>   packedTextures;
		UsedMaterial getMeshMaterial(shared_ptr<SimpledMesh> mesh);


		list<UsedMaterial> usedMaterial;


		//收集的文件名称
		vector<string>  fileNames;
		//收集的对象名称
		vector<string>  elementNames;
		vector<string>  elementIds;
		vector<int>     elementSIds;

	 

		struct OtherProps {
			int paramindex;
			list<json> values;
		};
		//其他属性，int为属性序号
		list<shared_ptr<OtherProps>>  elementProps;

	

		list<shared_ptr<PackedMesh>>  packedMeshs;

	    

		ModelInput * input = nullptr;
	protected:
		
		//精简一个子场景
		shared_ptr<SimpledSubScene> copySrc(shared_ptr<SubScene> scene, double geometric, SceneOutputConfig * config);
		//精简一个mesh
		shared_ptr<SimpledMesh> copySrc(shared_ptr<SubScene> scene, SubSceneMesh * mesh, double geometric, SceneOutputConfig * config);

		//是否有块因为过小被剔除了
		bool unVisibleMesh = false;

		//是否有块被精简了
		bool someSimpled = false;



		void  addUsedMaterial(shared_ptr<SimpledMesh> mesh);
		//对材质和纹理进行打包
		void   materialPack();


		//对mesh 进行打包
		void  meshPack();
		bool  meshPack(shared_ptr<SimpledMesh> mesh,int featid);


		//对打包后的三角网精简
		bool meshSimplify(shared_ptr<PackedMesh> mesh,double geometricError);

		//对打包前的三角网精简
		bool meshSimplify(shared_ptr<SimpledMesh> mesh, double geometricError);

		//坐标变换 原始模型坐标 转为 保存到 b3dm中的坐标
		osg::Vec3f   transformVertex(osg::Vec3d v);


		
		// 获得element的序号
		int  getElementFeatureId(BimElement * element);

		map<BimElement *, int> element2feature;

		


	public:
		bool simply(double geometric, SceneOutputConfig * config);
		//计算精简后数据量总量
		size_t getDataSize();
	 
 

 
		//全球坐标包围盒
		osg::BoundingBoxd globleBox;

		//本块内包围盒（模型坐标）
		osg::BoundingBoxd srcBox;

		//本块内容包围盒(b3dm坐标)
		osg::BoundingBoxd contentBox;

		//中心enu偏移矩阵 模型坐标乘以 此矩阵得到 全球坐标
		osg::Matrix       globleBoxCenterENU;
		//逆矩阵   全球坐标 乘以此 矩阵 得到 模型坐标
		osg::Matrix       globleBoxCenterENUInv;


		static void test();


		static int vcgSimply(void * vcgm, double geometric);
		static int vcgSimply(void * vcgm, int facecount);
		static int vcgFaceCount(void * vcgm);
		static void vcgMeshFrom(void * vcgm,shared_ptr<SimpledMesh> mesh);
		static void vcgMeshFrom(void * vcgm, shared_ptr<PackedMesh> mesh);
	};
 
}
