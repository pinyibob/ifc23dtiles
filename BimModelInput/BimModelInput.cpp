// BimModelInput.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#define GOOGLE_GLOG_DLL_DECL
#include <glog/logging.h>
#include "stdafx.h"
#include "BimModelInput.h"
#include <FreeImage.h> 

#include <fstream>
#include <set>
#include <algorithm>
using namespace std;

#include "util.h"

namespace XBSJ {

	ModelTexture::ModelTexture() {


	}
	ModelTexture::~ModelTexture() {
		if (freeimage)
		{
			FreeImage_Unload((FIBITMAP*)freeimage);
			freeimage = nullptr;
		}
	}

	//�ж����������Ƿ����
	bool ModelMaterial::equals(shared_ptr<ModelMaterial> other) {
		if (doubleSide != other->doubleSide)
			return false;
		if (diffuseTexture != other->diffuseTexture)
			return false;
		if (shinniness != other->shinniness)
			return false;
		if (diffuse != other->diffuse)
			return false;
		if (ambient != other->ambient)
			return false;
		if (specular != other->specular)
			return false;
		if (emissive != other->emissive)
			return false;

		return true;
	}

	size_t filesize(string path) {
		ifstream fsRead;

		size_t ret = 0;
		fsRead.open(path.c_str(), ios::in | ios::binary);
		if (fsRead) {
			fsRead.seekg(0, fsRead.end);
			ret = fsRead.tellg();
			fsRead.close();
		}
		return ret;
	}

	bool  ModelTexture::equals(shared_ptr<ModelTexture> other) {

		if (path != other->path)
			return false;

		return true;
	}
	FREE_IMAGE_FORMAT ext2fmt(string ext)
	{
		FREE_IMAGE_FORMAT ft;
		if (ext == "jpg") {
			ft = FIF_JPEG;
		}
		else if (ext == "png") {
			ft = FIF_PNG;
		}
		else if (ext == "bmp") {
			ft = FIF_BMP;
		}
		else if (ext == "dds") {
			ft = FIF_DDS;
		}
		else if (ext == "gif") {
			ft = FIF_GIF;
		}
		else if (ext == "tif") {
			ft = FIF_TIFF;
		}
		else
		 ft = FIF_UNKNOWN;
		return ft;
	}

	bool   ModelTexture::loadImage() {

		//1,��ú�׺ ���ݺ�׺���Ƹ�ʽ
		string ext = path.substr(path.find_last_of(".") + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		FREE_IMAGE_FORMAT ft = ext2fmt(ext);

		//2,���û��ͼƬ���ݣ���ô���Ա��ؼ���
		if (imageData.empty())
		{
			//����tga�ļ���ʹ��stbתpng
			if (ext == "tga")
			{
				imageData = tga2png(path);
				ft = FIF_PNG;
			}
			else {
				//��ȡ���ڴ�
				imageData = readAll(path);
			}
		}
		 
		//�ٴμ�������Ƿ�����
		if (imageData.empty() || ft == FIF_UNKNOWN)
		{
			LOG(ERROR) << "failed to load image :" << path;
			return false;
		}

		//�ڴ��ȡ
		auto *  mem = FreeImage_OpenMemory((BYTE*)imageData.data(), imageData.size());
		if (!mem) {
			return false;
		}
		FIBITMAP  * fimage = nullptr;
		fimage = FreeImage_LoadFromMemory(ft, mem); 
		FreeImage_CloseMemory(mem);

		if (!fimage) {
			LOG(ERROR) << "failed to load image :" << path;
			return false;
		}
 
		datasize = imageData.size();
 
		//һЩpng��Ȼλ���64λ��ת��32λ�洢
		auto bpp = FreeImage_GetBPP(fimage);

		if (bpp == 64) {

			auto b32 = FreeImage_ConvertTo32Bits(fimage);
			if (b32) {
				FreeImage_Unload(fimage);
				fimage = b32;
			}
			bpp = FreeImage_GetBPP(fimage);
		}
		else if (bpp == 48) {

			auto b24 = FreeImage_ConvertTo24Bits(fimage);
			if (b24) {
				FreeImage_Unload(fimage);
				fimage = b24;
			}
			bpp = FreeImage_GetBPP(fimage);
		}


		width = FreeImage_GetWidth(fimage);
		height = FreeImage_GetHeight(fimage);
		if (bpp == 32) {
			bool hasalpha = false;
			//�ж��Ƿ�ȫ����͸���������������ôת��24λ
			for (unsigned int i = 0; i < width; i++) {
				for (unsigned int j = 0; j < height; j++) {
					RGBQUAD color;
					if (FreeImage_GetPixelColor(fimage, i, j, &color)) {
						if (color.rgbReserved < 255) {
							hasalpha = true;
							break;
						}
					}

				}
				if (hasalpha)
					break;
			}

			//���û��͸��
			if (!hasalpha) {
				auto b24 = FreeImage_ConvertTo24Bits(fimage);
				if (b24) {
					FreeImage_Unload(fimage);
					fimage = b24;
				}
				bpp = FreeImage_GetBPP(fimage);
			}
		}
		freeimage = fimage;

		return true;
	}

	std::shared_ptr<BimScene> BimScene::copy() const
	{
		auto i = std::make_shared<BimScene>();
		std::memcpy(i.get(), this, sizeof(this));
		return i;
	}

	shared_ptr<ModelMaterial> BimScene::getMaterial(shared_ptr<ModelMaterial> & material) {

		for (auto & m : materials) {
			if (m->equals(material)) {
				return m;
			}
		}
		//������Ҫ������

		materials.push_back(material);

		return material;
	}

	shared_ptr<ModelTexture>  BimScene::getTexture(shared_ptr<ModelTexture> & texture) {

		for (auto & t : textures) {
			if (t->equals(texture)) {
				return t;
			}
		}
		textures.push_back(texture);

		texture->loadImage();

		return texture;

	}

	osg::BoundingBoxd  ModelMesh::caculBox() {

		osg::BoundingBoxd box;

		for (auto &v : vertexes) {

			box.expandBy(v);
		}

		return move(box);
	}
	osg::BoundingBoxd BimElement::caculBox() {
		osg::BoundingBoxd box;

		for (auto &m : meshes) {

			auto b = m->caculBox();
			if (b.valid())
				box.expandBy(b);
		}

		if (box.radius() > 1000) {

		}

		return move(box);

	}

	int BimScene::getParamIndex(string name, string type) {

		int index = -1;
		for (auto p : params) {
			index++;
			if (p.name == name && p.type == type)
				return index;
		}
		BimParam bp;
		bp.name = name;
		bp.type = type;

		index = params.size();
		params.push_back(bp);
		return index;
	}


	list<shared_ptr<ModelInputPlugin>> ModelInputPlugin::plugins;
}