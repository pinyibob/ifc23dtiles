#include "Scene2Gltf.h"

//#include <windows.h>
#include <GL/gl.h>
#include "SubScenePacker.h"
#include "SubScene.h"
#include "util.h" 
#include "SceneOutputConfig.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "custom_material.h"

namespace XBSJ {

	Scene2Gltf::Scene2Gltf(SceneOutputConfig * cfg) : config(cfg)
	{
	}

	Scene2Gltf::~Scene2Gltf()
	{
	}

	 
	class Scene2GltfInner {

	public:
		Scene2GltfInner(SubScenePacker * g, stringstream & stm, SceneOutputConfig*cfg) :group(g),
			 gltfbufferStream(stm),config(cfg) {
 
		}

	public:
		json process() {

			tinygltf::Buffer buffer0;
 
			for (auto &p : group->packedMeshs) {
				if(!p->hasTexcoord())
					processMesh(p,buffer0);
			}

			for (auto &p : group->packedMeshs) {
				if (p->hasTexcoord())
					processMesh(p, buffer0);
			}
			
			gltfmodel.buffers.push_back(buffer0);
			//???mesh
			gltfmodel.meshes.push_back(gltfmesh);
			//???node 
			tinygltf::Node node;
			node.mesh = gltfmodel.meshes.size() - 1;
			gltfmodel.nodes.push_back(node);
			//???scene
			tinygltf::Scene gltfscene;
			for (int i = 0; i < gltfmodel.nodes.size(); i++) {
				gltfscene.nodes.push_back(i);
			}
			gltfmodel.scenes.push_back(gltfscene);
			gltfmodel.defaultScene = 0;

			//???draco???
			vector<string> excludes;
			//excludes.push_back("_BATCHID");
			if (config->dracoCompression) {
				dracoGltf(gltfmodel, excludes);
			}
			
		 	//2,????????
			for (auto tex : group->packedTextures) {
				if (!tex->freeimage)
					continue;

				tex->updateImageData(config->crnCompression);
 
				if (tex->imageData.empty())
				{
					LOG(WARNING) << "updateImageData failed";
					continue;
				}
				//gltfimage
				tinygltf::Image timage;
				timage.mimeType = tex->imageType;

				timage.image.resize(tex->imageData.size());
				memcpy(&timage.image[0], tex->imageData.data(), tex->imageData.size());
 
				gltfmodel.images.push_back(timage);
 
				//????sampler
				tinygltf::Sampler sampler;
				sampler.minFilter = config->crnCompression  ? TINYGLTF_TEXTURE_FILTER_LINEAR : TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
				sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
				//gltf???????? 10496
				//sampler.wrapS =  TINYGLTF_TEXTURE_WRAP_REPEAT;
				//sampler.wrapT =  TINYGLTF_TEXTURE_WRAP_REPEAT;

				sampler.wrapS = tex->mode_s == ModelTextureMode_Wrap ? TINYGLTF_TEXTURE_WRAP_REPEAT : TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
				sampler.wrapT = tex->mode_t == ModelTextureMode_Wrap ? TINYGLTF_TEXTURE_WRAP_REPEAT : TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
				gltfmodel.samplers.push_back(sampler);

				//????Texture
				tinygltf::Texture texture;
				texture.sampler = gltfmodel.samplers.size() - 1;
				texture.source = gltfmodel.images.size() - 1;
				gltfmodel.textures.push_back(texture);

			}
			//LOG(INFO) << "save image count:" << group->packedTextures.size() << " size:" << gltfbufferSize - cursz;

			int customTechId = -1;
			//3,???????
			for (auto &mat : group->packedMaterials) {
				 
				//https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md
 
				//?????��????????????????????????
				if (mat->diffuse.r == 0 && mat->diffuse.g == 0 && mat->diffuse.b == 0) {
					mat->diffuse.r = 1.0;
					mat->diffuse.g = 1.0;
					mat->diffuse.b = 1.0;
					mat->diffuse.a = 1.0;
				}

				if (mat->customShader) {
				   if (customTechId < 0) {
					   
					   customTechId = addCustomTechnique(gltfmodel,!mat->nolight);
					  
				   }
				   string err;
				   int materialid = addCustomMaterial(gltfmodel, !mat->nolight,  mat->packedTexture, customTechId,  mat->doubleSide, mat->hasalpha || mat->diffuse.a<1,
					   (double *)&mat->diffuse, (double *)&mat->ambient, (double *)&mat->specular, &err);
				}
				else {
					tinygltf::Material material;
					tinygltf::Parameter baseColor;
					baseColor.number_array = { mat->diffuse.r , mat->diffuse.g , mat->diffuse.b , mat->diffuse.a  };
					material.values["baseColorFactor"] = baseColor;

					if (mat->packedTexture >= 0)
					{
						tinygltf::Parameter texturep;
						texturep.json_double_value["index"] = mat->packedTexture;
						texturep.json_double_value["texCoord"] = 0;
						material.values["baseColorTexture"] = texturep;
					}

					tinygltf::Parameter metallicFactor;
					metallicFactor.number_value = mat->metallicFactor;
					material.values["metallicFactor"] = metallicFactor;

					tinygltf::Parameter roughnessFactor;
					roughnessFactor.number_value = mat->roughnessFactor;
					material.values["roughnessFactor"] = roughnessFactor;


					if (mat->doubleSide) {
						tinygltf::Parameter twoside;
						twoside.bool_value = true;
						material.additionalValues["doubleSided"] = twoside;
					}

					//????????
					if (mat->hasalpha || mat->diffuse.a<1) {
						tinygltf::Parameter alphaMode;
						alphaMode.string_value = "BLEND";
						//twoside.string_value = "MASK";
						material.additionalValues["alphaMode"] = alphaMode;
					}
					gltfmodel.materials.push_back(material);
				}
				
			}

			auto& afterDraco = gltfmodel.buffers[0];
			for (auto &img : gltfmodel.images) {

				writeBuffer(afterDraco,&img.image[0], img.image.size());

				img.bufferView = gltfmodel.bufferViews.size() - 1;

				img.image.clear();
			}

			gltfbufferStream.clear();
			for (auto & b : gltfmodel.buffers) {
				gltfbufferStream.write((char *)&b.data[0], b.data.size());
				 
				b.extras = tinygltf::Value((int)b.data.size());
				b.data.clear();
			}
 
			//??????? 
			gltfmodel.asset.generator = "cesiumlab";
			gltfmodel.asset.version = "2.0";

			tinygltf::TinyGLTF tf;

			json gltfjson;
			tf.WriteGltfSceneToJSON(&gltfmodel, gltfjson);

			removeNull(gltfjson);
			return move(gltfjson);
		}
	protected:
	 
		void removeNull(json &json) {
			for (auto v = json.begin(); v != json.end();) {
				if (v.value().is_null()) {
					v = json.erase(v);
					continue;
				}

				if (v->is_object() || v->is_array()) {
					removeNull(*v);
				}
				v++;
			}
		}
 
		bool processMesh(shared_ptr<PackedMesh> mesh,tinygltf::Buffer & buffer) {

			if (mesh->position.empty() || mesh->indices.empty()) {
				LOG(WARNING) << "empty packed mesh";
				return false;
			}
			tinygltf::Primitive primit;

			//????????
 
			unsigned int vcount = mesh->position.size();
			
			{
				writeBuffer(buffer,&mesh->position[0], vcount * sizeof(osg::Vec3f));
				tinygltf::Accessor accessor;
				accessor.bufferView = gltfmodel.bufferViews.size() - 1;
				accessor.byteOffset = 0;
				accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				accessor.count = vcount;
				accessor.type = TINYGLTF_TYPE_VEC3;
				auto &minPos = mesh->minPos;
				auto &maxPos = mesh->maxPos;
				accessor.minValues.push_back(minPos.x());
				accessor.minValues.push_back(minPos.y());
				accessor.minValues.push_back(minPos.z());
				accessor.maxValues.push_back(maxPos.x());
				accessor.maxValues.push_back(maxPos.y());
				accessor.maxValues.push_back(maxPos.z());

				gltfmodel.accessors.push_back(accessor);
				primit.attributes["POSITION"] = gltfmodel.accessors.size() - 1;
			}

			//????batchid
			if(mesh->batchid.size() > 0)
			{
				vector<float> fbatchid;
				fbatchid.reserve(mesh->batchid.size());
				for (auto b : mesh->batchid) {
					fbatchid.push_back(b);
				}
				writeBuffer(buffer, &fbatchid[0], vcount * sizeof(float));
				tinygltf::Accessor accessor;
				accessor.bufferView = gltfmodel.bufferViews.size() - 1;
				accessor.byteOffset = 0;
				accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				accessor.count = vcount;
				accessor.type = TINYGLTF_TYPE_SCALAR;
			 
				gltfmodel.accessors.push_back(accessor);
				primit.attributes["_BATCHID"] = gltfmodel.accessors.size() - 1;
			}


			//??????????
			if (mesh->normals.size() > 0)
			{
				writeBuffer(buffer, &mesh->normals[0], vcount * sizeof(osg::Vec3f));
				tinygltf::Accessor accessor;
				accessor.bufferView = gltfmodel.bufferViews.size() - 1;
				accessor.byteOffset = 0;
				accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				accessor.count = vcount;
				accessor.type = TINYGLTF_TYPE_VEC3;
				gltfmodel.accessors.push_back(accessor);
				primit.attributes["NORMAL"] = gltfmodel.accessors.size() - 1;
			}
			//???????
			if (mesh->color.size() > 0)
			{
				writeBuffer(buffer, &mesh->color[0], vcount * sizeof(unsigned int));
				tinygltf::Accessor accessor;
				accessor.bufferView = gltfmodel.bufferViews.size() - 1;
				accessor.byteOffset = 0;
				accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
				accessor.count = vcount;
				accessor.type = TINYGLTF_TYPE_SCALAR;
				gltfmodel.accessors.push_back(accessor);
				primit.attributes["COLOR"] = gltfmodel.accessors.size() - 1;
			}
			 
			//????????
			if (mesh->texcoord.size() > 0)
			{
				writeBuffer(buffer, &mesh->texcoord[0], vcount * sizeof(osg::Vec2f));
				tinygltf::Accessor accessor;
				accessor.bufferView = gltfmodel.bufferViews.size() - 1;
				accessor.byteOffset = 0;
				accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				accessor.count = vcount;
				accessor.type = TINYGLTF_TYPE_VEC2;
				gltfmodel.accessors.push_back(accessor);
				primit.attributes["TEXCOORD_0"] = gltfmodel.accessors.size() - 1;
			}
			
			//????????
			{

				unsigned int indices = mesh->indices.size();
				if (vcount > 65535) {
				 
					writeBuffer(buffer, mesh->indices.data(), indices * sizeof(unsigned int), TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
				}
				else {
					vector<unsigned short> index;
					index.reserve(indices);
					for (size_t i = 0; i < indices; i++) {
						index.push_back(mesh->indices[i]);
					}
					writeBuffer(buffer, index.data(), indices * sizeof(unsigned short), TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
				}
				tinygltf::Accessor iccessor;
				iccessor.bufferView = gltfmodel.bufferViews.size() - 1;
				iccessor.byteOffset = 0;
				iccessor.componentType = vcount > 65535 ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT : TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
				iccessor.count = indices;
				iccessor.type = TINYGLTF_TYPE_SCALAR;
				gltfmodel.accessors.push_back(iccessor);

				primit.indices = gltfmodel.accessors.size() - 1;
			}
			//???��???
			primit.material = mesh->packedMaterial;
			primit.mode = TINYGLTF_MODE_TRIANGLES;

			//??????

			gltfmesh.primitives.push_back(primit);

			//LOG(INFO) << "save primitives vertexcount:" << vcount << " size:" << gltfbufferSize - cursz;
			return true;
		}


		//��?????buffer
		void writeBuffer(tinygltf::Buffer & buffer0, const void * data, size_t writesize, int target = TINYGLTF_TARGET_ARRAY_BUFFER) {
			 
			tinygltf::BufferView view;
			view.buffer = 0;
			view.byteOffset = buffer0.data.size();
			view.byteLength = writesize;

			buffer0.data.resize(buffer0.data.size() + writesize);
			memcpy(&buffer0.data[view.byteOffset], data, writesize);
			//writesize ??????????4 
			while (buffer0.data.size() % 4 != 0) {
				buffer0.data.push_back(0);
			}
			view.target = target;

			gltfmodel.bufferViews.push_back(view);
		}
		tinygltf::Model gltfmodel;
		tinygltf::Mesh  gltfmesh; 
		stringstream & gltfbufferStream;

		//???????????
		SubScenePacker * group = nullptr;

		SceneOutputConfig * config = nullptr;
	};
	 
	bool Scene2Gltf::write( shared_ptr<SubScenePacker> group, stringstream& buffer, string & content) {

		 Scene2GltfInner exportor(group.get(), buffer, config);

		 auto jcont = exportor.process();
		 content = jcont.dump();

		return true;
	}
	 
}
