#pragma once

#include "config.h"
#include <osg/Vec3d>
#include <osg/Vec3f>
#include <osg/Matrixd>
#include <osg/BoundingBox>
#include <nlohmann/json.hpp>
#include <set>
using namespace nlohmann; 

namespace XBSJ
{
	using namespace std;
	enum BIMMODELINPUT_API ModelTextureMode {
		ModelTextureMode_Wrap,
		ModelTextureMode_Clamp
	};
	//?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7
	class BIMMODELINPUT_API ModelTexture {

	public:
		ModelTexture();
		~ModelTexture();
 
	public:
		string id; // ?1?7?1?7?1?7?1?7id
		string path; //?1?7?1?7?1?7?1?7��?1?7?1?7
		int    width = 0; //?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7
		int    height = 0;//?1?7?1?7?1?7?1?7?1?7?1?2?1?7
		ModelTextureMode mode_s = ModelTextureMode_Clamp; //?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7
		ModelTextureMode mode_t = ModelTextureMode_Clamp; //?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7
		unsigned int datasize = 0;

		string imageData;  //?1?7?1?7?1?7?1?7?1?7?0?8?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7?1?7

		//?0?9?0?3?1?7?1?7?1?7?1?7?0?0?1?7?1?7 ?0?0?1?7?1?7freeimage?1?7?1?7?1?7?1?7
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

		ModelColor() {}

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

	class BIMMODELINPUT_API ModelMaterial {
	public:
		ModelColor diffuse = ModelColor(0.7, 0.7, 0.7);
		ModelColor ambient = ModelColor(0.5, 0.5, 0.5);
		ModelColor specular = ModelColor(0, 0, 0);
		ModelColor emissive = ModelColor(0, 0, 0);
		bool       doubleSide = false;

		float shinniness = 0;
		bool customShader = false;
		bool nolight = false;

		std::shared_ptr<ModelTexture> diffuseTexture = nullptr;

		bool equals(std::shared_ptr<ModelMaterial> other);
		float metallicFactor = 0.1;
		float roughnessFactor = 0.7;

	};

	//basic mesh class contains vertex, normal , uv and it's material
	class BIMMODELINPUT_API ModelMesh
	{
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

	//seems part of ifc construct
	class BIMMODELINPUT_API  BimElement
	{
	public:
		BimElement() {}

	public:
		//list<std::shared_ptr<BimElement>> children;

		list<std::shared_ptr<ModelMesh>> meshes;

		//bim element property
		list<BimPropety> propertes;

		vector<ConstructPropety> constructProps;

		string  name = "";

		bool _bShell = false;

		//bim element id
		string  id = "";
		int     sid = 0;

		osg::BoundingBoxd caculBox();

	};

	//similiar element
	class BIMMODELINPUT_API BimScene {
	public:
		BimScene() {}

		std::shared_ptr<BimScene> copy() const;

		list<std::shared_ptr<BimElement>> elements;
		//pbr param
		list<std::shared_ptr<ModelMaterial>> materials;
		//uv texture
		list<std::shared_ptr<ModelTexture>> textures;

		vector<BimParam> params;

		vector<ConstructParam> constructParams;

		std::shared_ptr<ModelMaterial> getMaterial(std::shared_ptr<ModelMaterial> & material);
		std::shared_ptr<ModelTexture>  getTexture(std::shared_ptr<ModelTexture> & texture);

		int getParamIndex(string name, string type = "string");

		string crs = "";
		
		//la lon height rotation
		vector<double> mapCoords;

		string coords = "";

		string angles = "";

	};

	class BIMMODELINPUT_API ModelInputReader {
	public:
		virtual bool init(json & param) = 0;
		virtual bool init(json & param, const std::set<std::uint32_t>& ids) = 0;

		virtual int  getNumScene() = 0;

		virtual std::shared_ptr<BimScene>  loadScene(int i) = 0;

		virtual bool fillSceneTree(json & sceneTree) {
				
			return false;
		};
	};

	class BIMMODELINPUT_API ModelInputPlugin {
	public:
		string name;
		virtual bool supportFormat(string & ext) = 0;

		virtual std::shared_ptr<ModelInputReader>  createReader() = 0;

	public:
		static list<std::shared_ptr<ModelInputPlugin>> plugins;
	};

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
 