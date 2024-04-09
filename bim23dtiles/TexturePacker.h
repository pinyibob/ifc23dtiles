#pragma once
#include "binpack.hpp"
#include "stdafx.h"  
#include <FreeImage.h>
#include "ModelInput.h"

namespace XBSJ {
	 
	 

	class PackedMaterial {
	public:
		//材质相关参数
		ModelColor diffuse = ModelColor(0, 0, 0);
		ModelColor ambient = ModelColor(0, 0, 0);
		ModelColor specular = ModelColor(0, 0, 0);
		ModelColor emissive = ModelColor(0, 0, 0);
		bool       doubleSide = false;
		float     shinniness = 0;
		//是否自定义shader
		bool   customShader = true;
		//是否禁用光照
		bool   nolight = false;

		//合并后的纹理序号
		int   packedTexture = -1;
		//是否有alpha
		bool hasalpha = false;

		float metallicFactor = 0.1;
		float roughnessFactor = 0.7;

	};

	//纹理打包 把 非 reapet 贴图的纹理组合在一起
	class TexturePacker
	{
	public:
		TexturePacker(int width = 1024, int height = 1024);
 
		TexturePacker(shared_ptr<ModelTexture> tex, double scale);
		~TexturePacker();


		//存入一个texture
		 
		Rect<float>  packTexture(shared_ptr<ModelTexture> tex, double scale);

		FIBITMAP * freeimage = nullptr;

		ModelTextureMode mode_s = ModelTextureMode_Clamp; //横向纹理贴法
		ModelTextureMode mode_t = ModelTextureMode_Clamp; //纵向纹理贴法


		//是否透明
		bool hasalpha = false;

		void  updateImageData(bool crn);

		string  imageData;

		bool    crn = false;

		string imageType = "";
	private:
		shared_ptr<BinaryTreePack16> binpack;

		int width = 0;
		int height = 0;
	};
 
}
