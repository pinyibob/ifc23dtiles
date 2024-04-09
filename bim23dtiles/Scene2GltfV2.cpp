#include "Scene2Gltf.h"
#include "SubScenePacker.h"

#include "GltfCustomShader.h"
#include "util.h"
#include "SceneOutputConfig.h"
//#include <gl/GL.h>
namespace XBSJ {

	
	LabColor fromModelColor(ModelColor& ac)
	{
		return LabColor(ac.r, ac.g, ac.b, ac.a);
	}

	float * newColor(vector<LabColor*> & colors, ModelColor & c) {
		LabColor * nc = new LabColor();
		nc->r = c.r; nc->g = c.g; nc->b = c.b; nc->a = c.a;
		colors.push_back(nc);

		return (float*)nc;
	}
 

	void addAttributesToDracoMesh(GLTF::Primitive * primitive, shared_ptr<PackedMesh> &mesh, vector< GLTF::Object * > & waitRelease) {

		//
		GLTF::DracoExtension* dracoExtension = new GLTF::DracoExtension(); waitRelease.push_back(dracoExtension);

		primitive->extensions["KHR_draco_mesh_compression"] = (GLTF::Extension*)dracoExtension;

		std::unique_ptr<draco::Mesh> dracoMesh(new draco::Mesh());
		const int numTriangles = mesh->indices.size() / 3;
		dracoMesh->SetNumFaces(numTriangles);
		for (draco::FaceIndex i(0); i < numTriangles; ++i) {
			draco::Mesh::Face face;
			face[0] = mesh->indices[i.value() * 3];
			face[1] = mesh->indices[i.value() * 3 + 1];
			face[2] = mesh->indices[i.value() * 3 + 2];
			dracoMesh->SetFace(i, face);
		}


		//顶点
		{
			draco::PointAttribute att;
			const int componentCount = GLTF::Accessor::getNumberOfComponents(GLTF::Accessor::Type::VEC3);
			att.Init(draco::GeometryAttribute::POSITION,
				NULL,
				componentCount,
				draco::DT_FLOAT32,
				/* normalized */ false,
				/* stride */ sizeof(float) * componentCount,
				/* byte_offset */ 0);

			size_t vertexCount = mesh->position.size();
			int att_id = dracoMesh->AddAttribute(att, /* identity_mapping */ true, vertexCount);

			draco::PointAttribute *att_ptr = dracoMesh->attribute(att_id);
			// Unique id of attribute is set to attribute id initially.
			// To note that the attribute id is not necessary to be the same as unique id after compressing the mesh, but the unqiue id will not change.
			dracoExtension->attributeToId["POSITION"] = att_id;

			for (draco::PointIndex i(0); i < vertexCount; ++i) {
				std::vector<float> vertex_data(componentCount);
				memcpy(&vertex_data[0], &mesh->position[i.value()], sizeof(float) * componentCount);
				att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &vertex_data[0]);
			}
		}
		//法向量
		if (mesh->normals.size() > 0) {
			draco::PointAttribute att;
			const int componentCount = GLTF::Accessor::getNumberOfComponents(GLTF::Accessor::Type::VEC3);
			att.Init(draco::GeometryAttribute::NORMAL,
				NULL,
				componentCount,
				draco::DT_FLOAT32,
				/* normalized */ false,
				/* stride */ sizeof(float) * componentCount,
				/* byte_offset */ 0);

			size_t vertexCount = mesh->normals.size();
			int att_id = dracoMesh->AddAttribute(att, /* identity_mapping */ true, vertexCount);

			draco::PointAttribute *att_ptr = dracoMesh->attribute(att_id);
			// Unique id of attribute is set to attribute id initially.
			// To note that the attribute id is not necessary to be the same as unique id after compressing the mesh, but the unqiue id will not change.
			dracoExtension->attributeToId["NORMAL"] = att_id;

			for (draco::PointIndex i(0); i < vertexCount; ++i) {
				std::vector<float> vertex_data(componentCount);
				memcpy(&vertex_data[0], &mesh->normals[i.value()], sizeof(float) * componentCount);
				att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &vertex_data[0]);
			}
		}
		//纹理坐标
		if (mesh->texcoord.size() > 0) {
			draco::PointAttribute att;
			const int componentCount = GLTF::Accessor::getNumberOfComponents(GLTF::Accessor::Type::VEC2);
			att.Init(draco::GeometryAttribute::TEX_COORD,
				NULL,
				componentCount,
				draco::DT_FLOAT32,
				/* normalized */ false,
				/* stride */ sizeof(float) * componentCount,
				/* byte_offset */ 0);

			size_t vertexCount = mesh->texcoord.size();
			int att_id = dracoMesh->AddAttribute(att, /* identity_mapping */ true, vertexCount);

			draco::PointAttribute *att_ptr = dracoMesh->attribute(att_id);
			// Unique id of attribute is set to attribute id initially.
			// To note that the attribute id is not necessary to be the same as unique id after compressing the mesh, but the unqiue id will not change.
			dracoExtension->attributeToId["TEXCOORD_0"] = att_id;

			for (draco::PointIndex i(0); i < vertexCount; ++i) {
				std::vector<float> vertex_data(componentCount);
				memcpy(&vertex_data[0], &mesh->texcoord[i.value()], sizeof(float) * componentCount);
				att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &vertex_data[0]);
			}
		}


		//batchid
		if (mesh->batchid.size() > 0) {
			draco::PointAttribute att;
			const int componentCount = GLTF::Accessor::getNumberOfComponents(GLTF::Accessor::Type::SCALAR);
			att.Init(draco::GeometryAttribute::GENERIC,
				NULL,
				componentCount,
				draco::DT_UINT16,
				/* normalized */ false,
				/* stride */ sizeof(unsigned short) * componentCount,
				/* byte_offset */ 0);

			size_t vertexCount = mesh->batchid.size();
			int att_id = dracoMesh->AddAttribute(att, /* identity_mapping */ true, vertexCount);

			draco::PointAttribute *att_ptr = dracoMesh->attribute(att_id);
			// Unique id of attribute is set to attribute id initially.
			// To note that the attribute id is not necessary to be the same as unique id after compressing the mesh, but the unqiue id will not change.
			dracoExtension->attributeToId["_BATCHID"] = att_id;

			for (draco::PointIndex i(0); i < vertexCount; ++i) {
				std::vector<unsigned short> vertex_data(componentCount);
				memcpy(&vertex_data[0], &mesh->batchid[i.value()], sizeof(unsigned short) * componentCount);
				att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &vertex_data[0]);
			}
		}


		dracoExtension->dracoMesh = std::move(dracoMesh);

	}

	bool Scene2Gltf::writeV2(shared_ptr<SubScenePacker> group, stringstream & sss, string & jsoncontent) {

		GLTF::Asset gltfAsset;

		GLTF::Asset::Metadata meta;
		meta.generator = "cesiumlab";

		gltfAsset.metadata = &meta;


		GLTF::Options options;
		options.dracoCompression = config->dracoCompression;
		options.binary = true;

		vector< GLTF::Object * > waitrelease;
		vector<LabColor * > waitreleaseColor;
		//1,  填充模型信息
		auto scene = gltfAsset.getDefaultScene();


		GLTF::Node * node = new GLTF::Node(); waitrelease.push_back(node);

		//node->name = "";


		//纹理信息
		vector<GLTF::Texture * > textures;
		for (auto & tex : group->packedTextures) {
			auto * texture = new GLTF::Texture(); waitrelease.push_back(texture);
			auto * sampler = new GLTF::Sampler(); waitrelease.push_back(sampler);
			//对于crn纹理压缩，已经生成mipmap，不能在这里再设置了
			sampler->minFilter = config->crnCompression ?  GLTF::Constants::WebGL::LINEAR : GLTF::Constants::WebGL::LINEAR_MIPMAP_LINEAR;
			sampler->magFilter = GLTF::Constants::WebGL::LINEAR;
			sampler->wrapS = tex->mode_s == 0 ? GLTF::Constants::WebGL::REPEAT : GLTF::Constants::WebGL::CLAMP_TO_EDGE;
			sampler->wrapT = tex->mode_t == 0 ? GLTF::Constants::WebGL::REPEAT : GLTF::Constants::WebGL::CLAMP_TO_EDGE;
			texture->sampler = sampler;

			unsigned int bpp = 0;
			tex->updateImageData(config->crnCompression);

			if (!tex->imageData.empty()) {
 
				auto * gltfimage = new GLTF::Image("", (unsigned char *)tex->imageData.data(),tex->imageData.size(),tex->crn ? "crn":""); waitrelease.push_back(gltfimage);
		 
				texture->source = gltfimage;
			}
			textures.push_back(texture);
		}
		//材质信息
		GLTF::Program * customProgram = nullptr;

		vector<GLTF::Material*> materials;
		for (auto &mat : group->packedMaterials) {
			//自定义shader，且有纹理
			// if (mat->customShader && mat->packedTexture >= 0) {
			if (mat->customShader) {

				LabMaterial * material = new LabMaterial; waitrelease.push_back(material);

				material->doubleSided = mat->doubleSide;
				if (mat->hasalpha || mat->diffuse.a < 1) {
					material->alphaMode = "BLEND";
				}

				if (!customProgram)
					customProgram = LabFactory::newProgram(waitrelease);

				material->technique = LabFactory::newTecnique(waitrelease);
				material->technique->program = customProgram;

				//填充values
				if (mat->packedTexture >= 0) {
					material->values->diffuseTexture = textures[mat->packedTexture];
					material->textureId = mat->packedTexture;

					//这里判定一下，如果颜色全黑，那么变为默认
					if (mat->diffuse.r == 0 && mat->diffuse.r == 0 && mat->diffuse.r == 0) {
						mat->diffuse.r = 1.0;
						mat->diffuse.g = 1.0;
						mat->diffuse.b = 1.0;
						mat->diffuse.a = 1.0;
					}
					
				}
				else 
				{
					material->textureId = -1;
				}
				
				material->diffuse = fromModelColor(mat->diffuse);
				material->ambient = fromModelColor(mat->ambient);
				material->specular = fromModelColor(mat->specular);
				material->emissive = fromModelColor(mat->emissive);
				material->shinniness = mat->shinniness;
				material->nolight = mat->nolight;
 
				materials.push_back(material);
			}
			else {
				//构件pbr材质
				GLTF::MaterialPBR * material = new GLTF::MaterialPBR(); waitrelease.push_back(material);
				material->values->diffuse = newColor(waitreleaseColor, mat->diffuse);

				auto * metal = new GLTF::MaterialPBR::MetallicRoughness(); waitrelease.push_back(metal);
				material->metallicRoughness = metal;
				metal->baseColorFactor = material->values->diffuse;

				//是否需要以后把这两个暴漏出来
				metal->metallicFactor = 0.3;
				metal->roughnessFactor = 0.7;

				if (mat->packedTexture >= 0) {
					auto * pbrtexture = new GLTF::MaterialPBR::Texture(); waitrelease.push_back(pbrtexture);
					pbrtexture->texture = textures[mat->packedTexture];
					pbrtexture->texCoord = 0;
					metal->baseColorTexture = pbrtexture;
				}
				material->doubleSided = mat->doubleSide;
				if (mat->hasalpha || mat->diffuse.a < 1) {
					material->alphaMode = "BLEND";
				}
				materials.push_back(material);

			}
		}

		//三角网信息
		GLTF::Mesh * mesh = new GLTF::Mesh(); waitrelease.push_back(mesh);
		node->mesh = mesh;

		for (auto &p : group->packedMeshs) {
			if (p->position.empty() || p->indices.empty()) {
				LOG(WARNING) << "empty packed mesh";
				continue;
			}
			// 这里很恶心，但是没办法，就这么简单干吧
			//node->name = p->name;

			GLTF::Primitive * primitive = new GLTF::Primitive(); waitrelease.push_back(primitive);
			primitive->mode = GLTF::Primitive::TRIANGLES;

			//primitive->name = p->name;

			primitive->attributes["POSITION"] = new GLTF::Accessor(GLTF::Accessor::Type::VEC3,
				GLTF::Constants::WebGL::FLOAT,
				(unsigned char *)&p->position[0],
				p->position.size(),
				GLTF::Constants::WebGL::ARRAY_BUFFER); waitrelease.push_back(primitive->attributes["POSITION"]);

			if (p->batchid.size() > 0) {
				primitive->attributes["_BATCHID"] = new GLTF::Accessor(GLTF::Accessor::Type::SCALAR,
					GLTF::Constants::WebGL::UNSIGNED_SHORT,
					(unsigned char *)&p->batchid[0],
					p->batchid.size(),
					GLTF::Constants::WebGL::ARRAY_BUFFER); waitrelease.push_back(primitive->attributes["_BATCHID"]);

			}

			if (p->normals.size() > 0) {
				primitive->attributes["NORMAL"] = new GLTF::Accessor(GLTF::Accessor::Type::VEC3,
					GLTF::Constants::WebGL::FLOAT,
					(unsigned char *)&p->normals[0],
					p->normals.size(),
					GLTF::Constants::WebGL::ARRAY_BUFFER); waitrelease.push_back(primitive->attributes["NORMAL"]);
			}

			//处理颜色
			if (p->color.size() > 0)
			{
				primitive->attributes["COLOR"] = new GLTF::Accessor(GLTF::Accessor::Type::SCALAR,
					GLTF::Constants::WebGL::UNSIGNED_INT,
					(unsigned char *)&p->color[0],
					p->color.size(),
					GLTF::Constants::WebGL::ARRAY_BUFFER); waitrelease.push_back(primitive->attributes["COLOR"]);
			}

			//处理纹理
			if (p->texcoord.size() > 0)
			{
				primitive->attributes["TEXCOORD_0"] = new GLTF::Accessor(GLTF::Accessor::Type::VEC2,
					GLTF::Constants::WebGL::FLOAT,
					(unsigned char *)&p->texcoord[0],
					p->texcoord.size(),
					GLTF::Constants::WebGL::ARRAY_BUFFER); waitrelease.push_back(primitive->attributes["TEXCOORD_0"]);
			}
			else {
				LOG(WARNING) << "empty texture coord";
			}
			//索引
			GLTF::Accessor* indices = NULL;
			if (p->indices.size() < 65536) {
				// We can fit this in an UNSIGNED_SHORT
				std::vector<unsigned short> unsignedShortIndices(p->indices.begin(), p->indices.end());
				indices = new GLTF::Accessor(GLTF::Accessor::Type::SCALAR, GLTF::Constants::WebGL::UNSIGNED_SHORT, (unsigned char*)&unsignedShortIndices[0], unsignedShortIndices.size(), GLTF::Constants::WebGL::ELEMENT_ARRAY_BUFFER);
			}
			else {
				// Leave as UNSIGNED_INT
				indices = new GLTF::Accessor(GLTF::Accessor::Type::SCALAR, GLTF::Constants::WebGL::UNSIGNED_INT, (unsigned char*)&p->indices[0], p->indices.size(), GLTF::Constants::WebGL::ELEMENT_ARRAY_BUFFER);
			}
			primitive->indices = indices; waitrelease.push_back(indices);


			primitive->material = materials[p->packedMaterial];


			if (config->dracoCompression) {

				addAttributesToDracoMesh(primitive, p, waitrelease);
			}

			mesh->primitives.push_back(primitive);

		}


		scene->nodes.push_back(node);




		//2，生成gltf的 buffer部分

		//执行压缩
		if (config->dracoCompression) {
			gltfAsset.removeUncompressedBufferViews();
			gltfAsset.compressPrimitives(&options);
		}

		GLTF::Buffer* buffer = gltfAsset.packAccessors();

		//遍历所有图片存入
		std::vector<GLTF::Image*> images = gltfAsset.getAllImages();

		size_t imageBufferLength = 0;
		for (GLTF::Image* image : images) {
			imageBufferLength += image->byteLength;
		}
		unsigned char* bufferData = buffer->data;
		bufferData = (unsigned char*)realloc(bufferData, buffer->byteLength + imageBufferLength);
		size_t byteOffset = buffer->byteLength;

		for (GLTF::Image* image : images) {
			GLTF::BufferView* bufferView = new GLTF::BufferView(byteOffset, image->byteLength, buffer);
			image->bufferView = bufferView;
			std::memcpy(bufferData + byteOffset, image->data, image->byteLength);
			byteOffset += image->byteLength;
		}
		buffer->data = bufferData;
		buffer->byteLength += imageBufferLength;

		sss.write((char *)bufferData, buffer->byteLength);


		//3， 生成gltf 的json部分
		rapidjson::StringBuffer stringBuf;
		rapidjson::Writer<rapidjson::StringBuffer> jsonWriter = rapidjson::Writer<rapidjson::StringBuffer>(stringBuf);
		jsonWriter.StartObject();

		gltfAsset.writeJSON(&jsonWriter, &options);
		jsonWriter.EndObject();

		jsoncontent = stringBuf.GetString();


		//晕，这么搓，这个库不能自动释放东西 还需要自己释放
 
		for (auto * p : waitrelease) {
			delete p;
		}
		waitrelease.clear();

		for (auto * c : waitreleaseColor) {
			delete c;
		}
		waitreleaseColor.clear();

		return true;
	}
}
