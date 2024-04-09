// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 BIMMODELINPUT_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// BIMMODELINPUT_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#pragma once

#include "config.h"
#include <osg/Vec3d>
#include <osg/Vec3f>
#include <osg/Matrixd>
#include <osg/BoundingBox>
#include <json.hpp>
using namespace nlohmann; 

namespace XBSJ
{
	using namespace std;
	enum BIMMODELINPUT_API ModelTextureMode {
		ModelTextureMode_Wrap,
		ModelTextureMode_Clamp
	};
	//纹理定义
	class BIMMODELINPUT_API ModelTexture {

	public:
		ModelTexture();
		~ModelTexture();
 
	public:
		string id; // 纹理id
		string path; //纹理路径
		int    width = 0; //纹理宽度
		int    height = 0;//纹理高度
		ModelTextureMode mode_s = ModelTextureMode_Clamp; //横向纹理贴法
		ModelTextureMode mode_t = ModelTextureMode_Clamp; //纵向纹理贴法
		unsigned int datasize = 0;

		string imageData;  //纹理的二进制数据

		//原始数据图像 使用freeimage处理
		void * freeimage = nullptr;


		bool   loadImage();

		bool equals(std::shared_ptr<ModelTexture> other);


		static string tga2png(string &path);

		static string tocrn(string & imagedata);

		static string towebp(string & imagedata);
 
		 
	};


	struct BIMMODELINPUT_API  ModelColor {
		double r = 1;
		double g = 1;
		double b = 1;
		double a = 1;

		ModelColor() {

		}
		ModelColor(double _r, double _g, double _b) {
			r = _r;
			g = _g;
			b = _b;
		}

		bool operator == (ModelColor & c) {

			return c.r == r && c.g == g && c.b == b && c.a == a;
		}
		bool operator != (ModelColor & c) {

			return c.r != r || c.g != g || c.b != b || c.a != a;
		}
	};

	//材质定义
	class BIMMODELINPUT_API ModelMaterial {
	public:
		ModelColor diffuse = ModelColor(0.7, 0.7, 0.7);
		ModelColor ambient = ModelColor(0.5, 0.5, 0.5);
		ModelColor specular = ModelColor(0, 0, 0);
		ModelColor emissive = ModelColor(0, 0, 0);
		bool       doubleSide = false;


		float     shinniness = 0;
		//是否自定义shader
		bool   customShader = false;
		//是否禁用光照
		bool   nolight = false;

		std::shared_ptr<ModelTexture> diffuseTexture = nullptr;

		bool equals(std::shared_ptr<ModelMaterial> other);
		float metallicFactor = 0.1;
		float roughnessFactor = 0.7;

	};



	//三角网
	class BIMMODELINPUT_API ModelMesh {

	public:
		ModelMesh() {}
	public:
		vector<osg::Vec3d> vertexes;
		vector<osg::Vec3f> normals;
		vector<osg::Vec2f> textcoords;
		vector<int>     indices;

		std::shared_ptr<ModelMaterial>  material;


		bool HasFaces() {
			return indices.size() > 0 && vertexes.size() > 0;
		}
		bool HasNormals() {
			return normals.size() > 0;
		}
		bool HasTextureCoords() {
			return textcoords.size() > 0;
		}

		osg::BoundingBoxd caculBox();
	};

	struct BIMMODELINPUT_API BimParam {
		string id;
		string name;
		string group;
		string type;
		string unittype;

		json to_json() {
			json j;
			j["id"] = id;
			j["name"] = name;
			j["group"] = group;
			j["type"] = type;
			j["unittype"] = unittype;

			return move(j);
		}
	};

	struct BIMMODELINPUT_API BimPropety {
		int  paramIndex;
		json value;
	};

	struct BIMMODELINPUT_API ConstructParam {
		string name;
		string field;
	};

	struct BIMMODELINPUT_API ConstructPropety {
		string name;
		int    id = -1;
	};

	//模型内的对象定义
	class BIMMODELINPUT_API  BimElement {

	public:
		BimElement() {}

	public:

		//子节点
		//list<std::shared_ptr<BimElement>> children;

		//三角网列表
		list<std::shared_ptr<ModelMesh>> meshes;

		//bim 属性
		list<BimPropety> propertes;

		//结构参数
		vector<ConstructPropety> constructProps;


		string  name = "";

		//id进行统一，都是字符串形式
		string  id = "";
		//这个id用来标记原始bim 软件里的 整性id
		int     sid = 0;

		osg::BoundingBoxd caculBox();

	};


	//场景定义
	class BIMMODELINPUT_API BimScene {
	public:
		BimScene() {

		}
	public:
		list<std::shared_ptr<BimElement>> elements;
		list< std::shared_ptr<ModelMaterial>> materials;
		list <std::shared_ptr<ModelTexture>> textures;

		//属性
		vector<BimParam> params;

		//结构参数
		vector<ConstructParam> constructParams;

		std::shared_ptr<ModelMaterial> getMaterial(std::shared_ptr<ModelMaterial> & material);
		std::shared_ptr<ModelTexture>  getTexture(std::shared_ptr<ModelTexture> & texture);


		int getParamIndex(string name, string type = "string");

		//投影参数
		string crs = "";
		//ifcplus将基点坐标和角度均存储在数组中
		vector<double> mapCoords;

		//基点坐标
		string coords = "";

		//角度
		string angles = "";

		
	};


	//读取器定义
	class BIMMODELINPUT_API ModelInputReader {
	public:
		//使用参数初始化
		virtual bool init(json & param) = 0;
		virtual bool init(json & param, json & ids) = 0;

		//获得需要几次读取
		virtual int  getNumScene() = 0;

		//读取第几次
		virtual std::shared_ptr<BimScene>  loadScene(int i) = 0;


		//获取完整的场景树
		virtual bool fillSceneTree(json & sceneTree) {
				
			return false;
		};
	};

	//插件定义
	class BIMMODELINPUT_API   ModelInputPlugin {

	public:
		//插件的名称
		string name;
		virtual bool supportFormat(string & ext) = 0;

		virtual std::shared_ptr<ModelInputReader>  createReader() = 0;

	public:
		static list<std::shared_ptr<ModelInputPlugin>>  plugins;
	};

	//进度报告辅助类
	class BIMMODELINPUT_API ProgressHelper
	{
	public:
		ProgressHelper(string name, double step, double percent = 0);
		~ProgressHelper();

		void skip(double step);


		static void init(string taskserver, string taskname);
		static void finish();
	};
	
}


 
 