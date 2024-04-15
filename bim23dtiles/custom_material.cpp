//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <cstdio>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using nlohmann::json;

#include "custom_material.h"

namespace tinygltf2 {
	bool ParseJsonAsValue(tinygltf::Value *ret, const json &o);
	bool ParseStringProperty(
		std::string *ret, std::string *err, const json &o,
		const std::string &property, bool required,
		const std::string &parent_node = std::string());

	bool ParseMaterial(tinygltf::Material *material, std::string *err, const json &o);
}

extern int XBSJ_VERTEX_SHADER;
extern int XBSJ_FRAGMENT_SHADER;
extern std::string flatVS;
extern std::string flatFS;
extern std::string normalVS;
extern std::string normalFS;

json getShaderJson(int shaderBufferViewID, int shaderType);
json getFlatJsonTechnique(int programid);
json getFlatJsonMaterial(int techniqueID, int textureID);
json getNormalJsonTechnique(int programid);
json getNormalJsonMaterial(int techniqueID, int textureID);

// δʹ�ã�δ��֤������������ vtxf 20180107
int addTexture(tinygltf::Model& model, unsigned char* imageData, int imageDataSize, std::string imageType, bool repeat) {
	// 1 add BufferView
	// �ٶ�image����buffer2��
	if (model.buffers.size() < 3) {
		model.buffers.resize(3);
	}

	auto & bufferData = model.buffers[2].data;
	const int originSize = bufferData.size();
	bufferData.resize(originSize + imageDataSize);
	memcpy(&bufferData[originSize], imageData, imageDataSize);

	tinygltf::BufferView bv;
	bv.buffer = 2;
	bv.byteLength = imageDataSize;
	bv.byteOffset = originSize;
	model.bufferViews.push_back(bv);
	const int bvid = model.bufferViews.size() - 1;

	// 2 add Image
	std::string mimeType = "";
	// TODO
	if (imageType == "jpg")
	{
		mimeType = "image/jpeg";
	}
	else if (imageType == "png")
	{
		mimeType = "image/png";
	}
	else if (imageType == "crn")
	{
		mimeType = "image/crn";
	}

	tinygltf::Image image;
	image.bufferView = bvid;
	image.mimeType = mimeType;

	//"images": {
	//	"Image0001": {
	//		"name": "Image0001",
	//			"uri" : "Image0001.png",
	//			"extras" : {
	//			"compressedImage3DTiles": {
	//				"crunch": {
	//					"uri": "Image0001-crunch.crn"
	//				}
	//			}
	//		}
	//	}
	//},

	tinygltf::Value::Object crunch;
	crunch["uri"] = tinygltf::Value("Image0001-crunch.crn"); // TODO

	tinygltf::Value::Object compressedImage3DTiles;
	compressedImage3DTiles["crunch"] = tinygltf::Value(crunch);

	// TODO ���ݲ�����)
	//image.extras = tinygltf::Value(compressedImage3DTiles);

	model.images.push_back(std::move(image));
	const int imageid = model.images.size() - 1;

	// 3 add Sampler
	tinygltf::Sampler sampler;
	//����crn����ѹ�����Ѿ�����mipmap�������������������� LINEAR_MIPMAP_LINEAR
	sampler.minFilter = imageType == "crn" ? TINYGLTF_TEXTURE_FILTER_LINEAR : TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
	sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	sampler.wrapS = repeat ? TINYGLTF_TEXTURE_WRAP_REPEAT : TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
	sampler.wrapT = repeat ? TINYGLTF_TEXTURE_WRAP_REPEAT : TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
	model.samplers.push_back(std::move(sampler));
	const int sampleid = model.samplers.size() - 1;

	// 4 add Texture
	// tinygltf�ж�û��format��internalFormat��target��type��Щ���ԣ������õĶ���Ĭ��ֵ�ɡ���
	//"textures": {
	//	"texture_Image0001": {
	//		"format": 6408,
	//			"internalFormat" : 6408,
	//			"sampler" : "sampler_0",
	//			"source" : "Image0001",
	//			"target" : 3553,
	//			"type" : 5121
	//	}
	//}

	tinygltf::Texture texture;
	texture.sampler = sampleid;
	texture.source = imageid;

	model.textures.push_back(std::move(texture));
	const int texid = model.textures.size() - 1;
	return texid;
}

int addShaderBufferView(tinygltf::Model& model, const std::string & shaderString) {
	const int shaderStringLength = shaderString.length();

	// �ٶ�shader����buffer1��
	//if (model.buffers.size() <= 1) {
	//	model.buffers.push_back(tinygltf::Buffer());
	//}

	auto & bufferData = model.buffers[0].data;
	const int originSize = bufferData.size();
	bufferData.resize(originSize + shaderStringLength);
	memcpy(&bufferData[originSize], &shaderString[0], shaderStringLength);

	tinygltf::BufferView bv;
	bv.buffer = 0;
	bv.byteLength = shaderStringLength;
	bv.byteOffset = originSize;
	model.bufferViews.push_back(bv);

	return model.bufferViews.size() - 1;
}

bool addMaterial(tinygltf::Model& model, const json& jsonMaterial, std::string* err) {
	tinygltf::Material material;
	tinygltf2::ParseStringProperty(&material.name, err, jsonMaterial, "name", false);
	if (!tinygltf2::ParseMaterial(&material, err, jsonMaterial)) {
		return false;
	}

	model.materials.push_back(material);

	int materialid = model.materials.size() - 1;

	return materialid;
}

bool add_KHR_techniques_webgl(tinygltf::Model& model) {
	auto & er = model.extensionsRequired;
	if (er.end() == std::find(er.begin(), er.end(), "KHR_techniques_webgl")) {
		er.push_back("KHR_techniques_webgl");
	}

	auto & eu = model.extensionsUsed;
	if (eu.end() == std::find(eu.begin(), eu.end(), "KHR_techniques_webgl")) {
		eu.push_back("KHR_techniques_webgl");
	}

	if (model.extensions.find("KHR_techniques_webgl") == model.extensions.end())
	{
		model.extensions.insert(std::make_pair("KHR_techniques_webgl", tinygltf::Value(tinygltf::Value::Object())));
	}

	return true;
}

int addShader(tinygltf::Model& model, json jsonShader) {
	tinygltf::Value jsonShaderValue;
	tinygltf2::ParseJsonAsValue(&jsonShaderValue, jsonShader);

	// vtxf Ϊ�˰�value������ȥ��������Сʱ����
	auto & KHR_techniques_webgl = model.extensions["KHR_techniques_webgl"];
	auto & KHR_techniques_webglObject = KHR_techniques_webgl.Get<tinygltf::Value::Object>();
	if (!KHR_techniques_webgl.Has("shaders")) {
		KHR_techniques_webglObject.insert(std::make_pair("shaders", tinygltf::Value(tinygltf::Value::Array())));
	}

	auto & shadersArray = KHR_techniques_webglObject["shaders"].Get<tinygltf::Value::Array>();
	shadersArray.push_back(jsonShaderValue);

	return shadersArray.size() - 1;
}

int addProgram(tinygltf::Model& model, int vsid, int fsid) {
	int programid = -1;

	//json jsonProgram = R"({
	//	"attributes": [
	//		"a_position",
	//		"a_texcoord0"
	//	],
	//	"fragmentShader": 0,
	//	"vertexShader": 1
	//})"_json;

	json jsonProgram = R"({
		"fragmentShader": 0,
		"vertexShader": 1
	})"_json;

	jsonProgram["vertexShader"] = vsid;
	jsonProgram["fragmentShader"] = fsid;

	tinygltf::Value jsonProgramValue;
	tinygltf2::ParseJsonAsValue(&jsonProgramValue, jsonProgram);

	// vtxf Ϊ�˰�jsonTechniqueValue������ȥ��������Сʱ����
	auto & KHR_techniques_webgl = model.extensions["KHR_techniques_webgl"];
	auto & KHR_techniques_webglObject = KHR_techniques_webgl.Get<tinygltf::Value::Object>();
	if (!KHR_techniques_webgl.Has("programs")) {
		KHR_techniques_webglObject.insert(std::make_pair("programs", tinygltf::Value(tinygltf::Value::Array())));
	}
	auto & programsArray = KHR_techniques_webglObject["programs"].Get<tinygltf::Value::Array>();
	programsArray.push_back(jsonProgramValue);

	programid = programsArray.size() - 1;

	return programid;
}

int addTechnique(tinygltf::Model & model, const json & jsonTechnique) {
	tinygltf::Value jsonTechniqueValue;
	tinygltf2::ParseJsonAsValue(&jsonTechniqueValue, jsonTechnique);

	// vtxf Ϊ�˰�jsonTechniqueValue������ȥ��������Сʱ����
	auto & KHR_techniques_webgl = model.extensions["KHR_techniques_webgl"];
	auto & KHR_techniques_webglObject = KHR_techniques_webgl.Get<tinygltf::Value::Object>();
	if (!KHR_techniques_webgl.Has("techniques")) {
		KHR_techniques_webglObject.insert(std::make_pair("techniques", tinygltf::Value(tinygltf::Value::Array())));
	}
	auto & techniquesArray = KHR_techniques_webglObject["techniques"].Get<tinygltf::Value::Array>();
	techniquesArray.push_back(jsonTechniqueValue);

	return techniquesArray.size() - 1;
}

int addCustomTechnique(tinygltf::Model & model,bool normal ) {
	add_KHR_techniques_webgl(model);

	int vsbvid = addShaderBufferView(model, normal ? normalVS : flatVS);
	int fsbvid = addShaderBufferView(model, normal ? normalFS : flatFS);

	int vsid = -1;
	json jsonVS = getShaderJson(vsbvid, XBSJ_VERTEX_SHADER);
	vsid = addShader(model, jsonVS);

	int fsid = -1;
	json jsonFS = getShaderJson(fsbvid, XBSJ_FRAGMENT_SHADER);
	fsid = addShader(model, jsonFS);

	int programid = -1;
	programid = addProgram(model, vsid, fsid);

	int techniqueID = -1;
	json jsonTechnique;
	
	if (normal)
		jsonTechnique = getNormalJsonTechnique(programid);
	else
		jsonTechnique = getFlatJsonTechnique(programid);

	 
	 techniqueID = addTechnique(model, jsonTechnique);
  
	//"enum" : [3042, 2884, 2929, 32823, 32926, 3089],
	//"gltf_enumNames" : ["BLEND", "CULL_FACE", "DEPTH_TEST", "POLYGON_OFFSET_FILL", "SAMPLE_ALPHA_TO_COVERAGE", "SCISSOR_TEST"]
	/*
	if (blend)
	{
		auto stateEnables = json::array();
		stateEnables.push_back(3042);
		//if (nmp.cull_face) stateEnables.push_back(2884);
		//if (nmp.depth_test) stateEnables.push_back(2929);
		jsonTechnique["states"]["enable"] = stateEnables;
	}*/

	return techniqueID;
}

int addCustomMaterial(tinygltf::Model & model,bool normal, int textureID, int techniqueID,bool doublesided,bool blend,
	double * d, double *a, double* s,std::string * err)
{
	 json jsonMaterial;
	if (normal)
		jsonMaterial = getNormalJsonMaterial(techniqueID, textureID);
	else
		jsonMaterial = getFlatJsonMaterial(techniqueID, textureID);
	 
	if (blend) 
		jsonMaterial["alphaMode"] = "BLEND";

	if (doublesided)
		jsonMaterial["doubleSided"] = true;


	if (normal) {

		if (d)
		{
			jsonMaterial["extensions"]["KHR_techniques_webgl"]["values"]["u_diffuse"] = { d[0], d[1], d[2], d[3] };
		}

		if (a) {
			jsonMaterial["extensions"]["KHR_techniques_webgl"]["values"]["u_ambient"] = { a[0], a[1], a[2], 1.0 };
			
		}

		if (s) {
			jsonMaterial["extensions"]["KHR_techniques_webgl"]["values"]["u_specular"] = { s[0], s[1], s[2], 1.0 };
		}
	}
 
	return addMaterial(model, jsonMaterial, err);
}
