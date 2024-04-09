#pragma once

//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <cstdio>
#include <fstream>
#include <iostream>

// 1 draco压缩用
bool dracoGltf(tinygltf::Model& model, const std::vector<std::string>& excludeAttributes = std::vector<std::string>());




struct NormalMaterialParamters
{
	float u_diffuse[4];
	float u_ambient[3];
	float u_specular[3];
	//"enum" : [3042, 2884, 2929, 32823, 32926, 3089],
	//"gltf_enumNames" : ["BLEND", "CULL_FACE", "DEPTH_TEST", "POLYGON_OFFSET_FILL", "SAMPLE_ALPHA_TO_COVERAGE", "SCISSOR_TEST"]
	bool blend = false;
	bool cull_face=true;
	bool depth_test = true;
};


// 2 自定义shader相关函数
int addCustomTechnique(tinygltf::Model & model, bool normal);
 
int addCustomMaterial(tinygltf::Model & model, bool normal, int textureID, int techniqueID, bool doublesided, bool blend,
	double * d, double *a, double* s, std::string * err);