#define TINYGLTF_IMPLEMENTATION

#include "tiny_gltf.h"


#include <cstdio>
#include <fstream>
#include <iostream>

namespace tinygltf2 {
	bool ParseJsonAsValue(tinygltf::Value *ret, const json &o) {
		return tinygltf::ParseJsonAsValue(ret, o);
	}

	bool ParseStringProperty(
		std::string *ret, std::string *err, const json &o,
		const std::string &property, bool required,
		const std::string &parent_node = std::string()) {
		return tinygltf::ParseStringProperty(ret, err, o, property, required, parent_node);
	}

	bool ParseMaterial(tinygltf::Material *material, std::string *err, const json &o) {
		return tinygltf::ParseMaterial(material, err, o);
	}
}