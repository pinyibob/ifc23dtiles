//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include "json.hpp"
using nlohmann::json;

#undef max
#undef min
#undef ERROR
#include "draco/compression/encode.h"


// excludeAttributes?????????????????????? // vtxf 20190107
// ?????shader????????????????????attribute????draco?????????????????????
// ??????Cesium??bug????????????????????????excludeAttributes
bool dracoGltf(tinygltf::Model& model, const std::vector<std::string>& excludeAttributes = std::vector<std::string>()) {
	tinygltf::Buffer dracoBuffer;
	dracoBuffer.uri = "0.bin";
	dracoBuffer.data.reserve(2 * 1024 * 1024); // ??????

	std::vector<tinygltf::BufferView> dracoBufferViews;



	// Add faces to Draco mesh.
	for (size_t i = 0; i < model.meshes.size(); i++) {
		auto & mesh = model.meshes[i];
		for (size_t k = 0; k < mesh.primitives.size(); k++) {

			std::unique_ptr<draco::Mesh> dracoMesh(new draco::Mesh());

			auto & primitive = model.meshes[i].primitives[k];

			if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
				continue;
			}

			{
				auto & accessor = model.accessors[primitive.indices];
				accessor.count;
				accessor.componentType;
				accessor.type; // TINYGLTF_TYPE_SCALAR
				accessor.byteOffset;
				auto & bufferView = model.bufferViews[accessor.bufferView];
				bufferView.byteOffset;
				auto & buffer = model.buffers[bufferView.buffer];

				const int start = bufferView.byteOffset + accessor.byteOffset;

				const int numTriangles = accessor.count / 3;
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					unsigned short* primitiveIndices = (unsigned short*)&(buffer.data[start]);

					for (draco::FaceIndex fi(0); fi < numTriangles; ++fi) {
						draco::Mesh::Face face;
						face[0] = primitiveIndices[fi.value() * 3];
						face[1] = primitiveIndices[fi.value() * 3 + 1];
						face[2] = primitiveIndices[fi.value() * 3 + 2];
						dracoMesh->SetFace(fi, face);
					}
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					unsigned int* primitiveIndices = (unsigned int*)&(buffer.data[start]);

					for (draco::FaceIndex fi(0); fi < numTriangles; ++fi) {
						draco::Mesh::Face face;
						face[0] = primitiveIndices[fi.value() * 3];
						face[1] = primitiveIndices[fi.value() * 3 + 1];
						face[2] = primitiveIndices[fi.value() * 3 + 2];
						dracoMesh->SetFace(fi, face);
					}
				}
			}

			int NORMAL_att_id = -1;
			int POSITION_att_id = -1;
			int TEXCOORD_0_att_id = -1;
			int COLOR_att_id = -1;

			tinygltf::Value::Object attributes;

			// Add attributes to Draco mesh.
			for (const auto& entry : primitive.attributes) {
				// First create Accessor without data.
				std::string semantic = entry.first;

				const auto & e = excludeAttributes;
				if (e.end() != std::find(e.begin(), e.end(), semantic))
				{
					continue;
				}

				auto & accessor = model.accessors[entry.second];
				accessor.count;
				accessor.componentType;
				accessor.type; // TINYGLTF_TYPE_SCALAR
				accessor.byteOffset;
				auto & bufferView = model.bufferViews[accessor.bufferView];
				bufferView.byteOffset;
				auto & buffer = model.buffers[bufferView.buffer];

				int componentCount = 1;
				if (accessor.type == TINYGLTF_TYPE_SCALAR) {
					componentCount = 1;
				}
				else if (accessor.type == TINYGLTF_TYPE_VEC2) {
					componentCount = 2;
				}
				else if (accessor.type == TINYGLTF_TYPE_VEC3) {
					componentCount = 3;
				}
				else if (accessor.type == TINYGLTF_TYPE_VEC4) {
					componentCount = 4;
				}
				else {
					assert(0);
				}

				//const int vertexCount = accessor.count / componentCount;
				const int vertexCount = accessor.count;

				const int start = bufferView.byteOffset + accessor.byteOffset;

				if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
				{
					continue;
				}

				const float * attributeData = (const float*)&buffer.data[start];

				// Create attributes for Draco mesh.
				draco::GeometryAttribute::Type att_type = draco::GeometryAttribute::GENERIC;
				if (semantic == "POSITION")
					att_type = draco::GeometryAttribute::POSITION;
				else if (semantic == "NORMAL")
					att_type = draco::GeometryAttribute::NORMAL;
				else if (semantic.find("TEXCOORD") == 0)
					att_type = draco::GeometryAttribute::TEX_COORD;
				else if (semantic.find("COLOR") == 0)
					att_type = draco::GeometryAttribute::COLOR;

				draco::PointAttribute att;
				//if (att_type != draco::GeometryAttribute::GENERIC)
					att.Init(att_type, componentCount, draco::DT_FLOAT32, /* normalized */ false, /* stride */ sizeof(float) * componentCount);
				//else 
				//	att.Init(att_type, NULL, componentCount, draco::DT_UINT16, /* normalized */ false, /* stride */ 2 * componentCount, /* byte_offset */ 0);
				int att_id = dracoMesh->AddAttribute(att, /* identity_mapping */ true, vertexCount);
				draco::PointAttribute *att_ptr = dracoMesh->attribute(att_id);
				// Unique id of attribute is set to attribute id initially.
				// To note that the attribute id is not necessary to be the same as unique id after compressing the mesh, but the unqiue id will not change.
				//dracoExtension->attributeToId[semantic] = att_id;

				if (semantic == "POSITION")
					POSITION_att_id = att_id;
				else if (semantic == "NORMAL")
					NORMAL_att_id = att_id;
				else if (semantic.find("TEXCOORD") == 0)
					TEXCOORD_0_att_id = att_id;
				else if (semantic.find("COLOR") == 0)
					COLOR_att_id = att_id;

				for (draco::PointIndex i(0); i < vertexCount; ++i) {
					std::vector<float> vertex_data(componentCount);
					//if (att_type != draco::GeometryAttribute::GENERIC)
						memcpy(&vertex_data[0], &attributeData[i.value() * componentCount], sizeof(float) * componentCount);
					//else 
					//	memcpy(&vertex_data[0], &attributeData[i.value() * componentCount], 2 * componentCount);

					att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &vertex_data[0]);
				}

				attributes[semantic] = tinygltf::Value(att_id);
			}

			// Compress the mesh
			// Setup encoder options.
			draco::Encoder encoder;
			const int posQuantizationBits = 14;
			const int texcoordsQuantizationBits = 10;
			const int normalsQuantizationBits = 10;
			const int colorQuantizationBits = 8;
			// Used for compressing joint indices and joint weights.
			const int genericQuantizationBits = 16;

			encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, posQuantizationBits);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, texcoordsQuantizationBits);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, normalsQuantizationBits);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, colorQuantizationBits);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, genericQuantizationBits);

			draco::EncoderBuffer buffer;
			const draco::Status status = encoder.EncodeMeshToBuffer(*dracoMesh, &buffer);
			if (!status.ok()) {
				std::cerr << "Error: Encode mesh.\n";
				return false;
			}

			// Add compressed data to bufferview
			const int bufferSize = buffer.size();
			//unsigned char* allocatedData = (unsigned char*)malloc(buffer.size());
			//std::memcpy(allocatedData, buffer.data(), buffer.size());

			size_t originSize = dracoBuffer.data.size();
			size_t newSize = originSize + bufferSize;
			dracoBuffer.data.resize(newSize);
			std::memcpy((unsigned char*)&dracoBuffer.data[originSize], buffer.data(), bufferSize);

			const int bufferViewIndex = dracoBufferViews.size();
			tinygltf::BufferView bufferView;
			bufferView.buffer = 0;
			bufferView.byteOffset = originSize;
			bufferView.byteLength = bufferSize;
			// bufferView.target = 
			dracoBufferViews.push_back(bufferView);

			dracoMesh.reset();

			{
				auto & accessor = model.accessors[primitive.indices];
				accessor.bufferView = -1;
				accessor.byteOffset = 0;
			}

			for (const auto& entry : primitive.attributes) {
				auto & accessor = model.accessors[entry.second];
				accessor.bufferView = -1;
				accessor.byteOffset = 0;
			}

			//tinygltf::Value::Object attributes;
			//if (POSITION_att_id != -1)
			//	attributes["POSITION"] = tinygltf::Value(POSITION_att_id);
			//if (NORMAL_att_id != -1)
			//	attributes["NORMAL"] = tinygltf::Value(NORMAL_att_id);
			//if (TEXCOORD_0_att_id != -1)
			//	attributes["TEXCOORD_0"] = tinygltf::Value(TEXCOORD_0_att_id);

			//attributes["WEIGHTS_0"] = tinygltf::Value(5);
			//attributes["JOINTS_0"] = tinygltf::Value(5);

			tinygltf::Value::Object value_object;
			value_object["bufferView"] = tinygltf::Value(bufferViewIndex);
			value_object["attributes"] = tinygltf::Value(attributes);

			primitive.extensions.insert(std::make_pair("KHR_draco_mesh_compression", tinygltf::Value(value_object)));
			//primitive.extras = tinygltf::Value(attributes);
		}
	}

	//model.extensionsUsed.push_back("KHR_draco_mesh_compression");
	//model.extensionsRequired.push_back("KHR_draco_mesh_compression");

	auto & er = model.extensionsRequired;
	if (er.end() == std::find(er.begin(), er.end(), "KHR_draco_mesh_compression")) {
		er.push_back("KHR_draco_mesh_compression");
	}

	auto & eu = model.extensionsUsed;
	if (eu.end() == std::find(eu.begin(), eu.end(), "KHR_draco_mesh_compression")) {
		eu.push_back("KHR_draco_mesh_compression");
	}

	// ?????????§Ö???????????buffer0????
	//model.buffers.clear();
	//model.bufferViews.clear(); // TODO ????????bufferViews?????
	model.buffers[0] = std::move(dracoBuffer);

	//model.bufferViews = std::move(dracoBufferViews);

	{
		std::vector<tinygltf::BufferView> bufferViews = std::move(dracoBufferViews);

		int s = model.bufferViews.size();
		for (int i = 0; i < s; ++i) {
			const tinygltf::BufferView & bv = model.bufferViews[i];
			if (bv.buffer != 0)
			{
				bufferViews.push_back(std::move(bv));
			}
		}

		model.bufferViews = std::move(bufferViews);
	}

	return true;
}
