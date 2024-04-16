#pragma once 

#include "stdafx.h"  

#include "BimModelInput.h"

namespace XBSJ {

	class SceneOutputConfig;
	//对应了一个场景的输入 全部读入内存
	class ModelInput
	{
	public:
		ModelInput(json & config,  shared_ptr<ModelInputReader> & reader, int index);
		~ModelInput();

		bool load();

		//模型坐标转全球坐标
		osg::Vec3d  toGloble(osg::Vec3d  v);
		bool  configSrs(json & config);
		bool  setSrs(string crs, string coords,string angles);
		bool  setSrs(string crs, vector<double> mapCoords);
		static bool initGdal();
		static bool destroyGdal();
	public:

		shared_ptr<BimScene> scene;

		//输入文件
		string inputfile = "";

		//输入文件所在目录
		string inputfolder = "";

		//是否强制设置为双面
		bool forceDoubleSide = false;

		//调整纹理是否缩放的太厉害
		double textureScaleFactor = 1.3;
		//是否自定义shader
		bool   customShader = true;
		//是否禁用光照
		bool   nolight = false;
		//纹理误差系数
		double textureGeometricErrorFactor = 16;
		//分组优先级
		string splitPriority = "space"; // space 表示空间优先  material 表示材质优先

		//分组大小阈值 200KB左右
		unsigned int splitMaxDataSize = 200000;

		//分组颗粒度   
		//mesh 表示格网粒度，场景完全打散 
		//node 表示对象粒度，确保一个子场景最多含一个node
		//none 表示对场景不分割
		string splitUnit = "mesh";

		//颜色倍率
		double colorRatio = 1;
 
		//是否保存文佳佳匿名
		bool  saveFileName = true;
		//输入文件的名称
		string fileName = "";

		//汉字编码
		bool   encodeGBK = false;

		//ifc引擎
		string  ifcengine = "ifcengine";

		//是否精简三角网  none  不精简， before 合并前 ， after 合并后
		string simplifyMesh = "before";

		osg::Matrixd   bakeMatrix = osg::Matrixd::identity();

		//输入数据的投影 以及偏移位置
		string  srs = "ENU:39.90691,116.39123";
		osg::Vec3d  srsorigin = osg::Vec3d(0, 0, 0);

		//数据到wgs84的变换
		void * transform4326 = nullptr;
		bool enutrans = false;

		osg::Matrixd enuMatrix;

		list<shared_ptr<ModelTexture>> textureCaches;

		//void  sceneSpheres(SceneOutputConfig * config, list<json> & nodespheres);
		static  bool GenInputs(list<shared_ptr<ModelInput>> &inputs, json & config);
		static  bool GenInputs(list<shared_ptr<ModelInput>> &inputs, json & config, json & ids);
		static string ExeFolder;

		void fillSceneTree(json & j);
	protected:
		json cinput; 
  
		static bool InitPlugins();
 
		shared_ptr<ModelInputReader> reader;
		int readerIndex = -1;
	 
 
		map<string, osg::BoundingBoxd> elementsBox;
		void updateSphere(json & j);
		
	};

}
