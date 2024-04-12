// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� BIMMODELINPUT_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// BIMMODELINPUT_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�

#pragma once

#include "config.h"
#include <osg/Vec3d>
#include <osg/Vec3f>
#include <osg/Matrixd>
#include <osg/BoundingBox>
#include <nlohmann/json.hpp>
using namespace nlohmann; 

namespace XBSJ
{
	using namespace std;
	enum BIMMODELINPUT_API ModelTextureMode {
		ModelTextureMode_Wrap,
		ModelTextureMode_Clamp
	};
	//��������
	class BIMMODELINPUT_API ModelTexture {

	public:
		ModelTexture();
		~ModelTexture();
 
	public:
		string id; // ����id
		string path; //����·��
		int    width = 0; //��������
		int    height = 0;//�����߶�
		ModelTextureMode mode_s = ModelTextureMode_Clamp; //������������
		ModelTextureMode mode_t = ModelTextureMode_Clamp; //������������
		unsigned int datasize = 0;

		string imageData;  //�����Ķ���������

		//ԭʼ����ͼ�� ʹ��freeimage����
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

	//���ʶ���
	class BIMMODELINPUT_API ModelMaterial {
	public:
		ModelColor diffuse = ModelColor(0.7, 0.7, 0.7);
		ModelColor ambient = ModelColor(0.5, 0.5, 0.5);
		ModelColor specular = ModelColor(0, 0, 0);
		ModelColor emissive = ModelColor(0, 0, 0);
		bool       doubleSide = false;


		float     shinniness = 0;
		//�Ƿ��Զ���shader
		bool   customShader = false;
		//�Ƿ���ù���
		bool   nolight = false;

		std::shared_ptr<ModelTexture> diffuseTexture = nullptr;

		bool equals(std::shared_ptr<ModelMaterial> other);
		float metallicFactor = 0.1;
		float roughnessFactor = 0.7;

	};



	//������
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

	//ģ���ڵĶ�����
	class BIMMODELINPUT_API  BimElement {

	public:
		BimElement() {}

	public:

		//�ӽڵ�
		//list<std::shared_ptr<BimElement>> children;

		//�������б�
		list<std::shared_ptr<ModelMesh>> meshes;

		//bim ����
		list<BimPropety> propertes;

		//�ṹ����
		vector<ConstructPropety> constructProps;


		string  name = "";

		//id����ͳһ�������ַ�����ʽ
		string  id = "";
		//���id�������ԭʼbim ������� ����id
		int     sid = 0;

		osg::BoundingBoxd caculBox();

	};


	//��������
	class BIMMODELINPUT_API BimScene {
	public:
		BimScene() {

		}
	public:
		list<std::shared_ptr<BimElement>> elements;
		list< std::shared_ptr<ModelMaterial>> materials;
		list <std::shared_ptr<ModelTexture>> textures;

		//����
		vector<BimParam> params;

		//�ṹ����
		vector<ConstructParam> constructParams;

		std::shared_ptr<ModelMaterial> getMaterial(std::shared_ptr<ModelMaterial> & material);
		std::shared_ptr<ModelTexture>  getTexture(std::shared_ptr<ModelTexture> & texture);


		int getParamIndex(string name, string type = "string");

		//ͶӰ����
		string crs = "";
		//ifcplus����������ͽǶȾ��洢��������
		vector<double> mapCoords;

		//��������
		string coords = "";

		//�Ƕ�
		string angles = "";

		
	};


	//��ȡ������
	class BIMMODELINPUT_API ModelInputReader {
	public:
		//ʹ�ò�����ʼ��
		virtual bool init(json & param) = 0;
		virtual bool init(json & param, json & ids) = 0;

		//�����Ҫ���ζ�ȡ
		virtual int  getNumScene() = 0;

		//��ȡ�ڼ���
		virtual std::shared_ptr<BimScene>  loadScene(int i) = 0;


		//��ȡ�����ĳ�����
		virtual bool fillSceneTree(json & sceneTree) {
				
			return false;
		};
	};

	//�������
	class BIMMODELINPUT_API   ModelInputPlugin {

	public:
		//���������
		string name;
		virtual bool supportFormat(string & ext) = 0;

		virtual std::shared_ptr<ModelInputReader>  createReader() = 0;

	public:
		static list<std::shared_ptr<ModelInputPlugin>>  plugins;
	};

	//���ȱ��渨����
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


 
 