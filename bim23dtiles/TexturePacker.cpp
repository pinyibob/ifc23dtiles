#include "TexturePacker.h"
#include "Scene2Gltf.h"
#include "BimModelInput.h"
namespace XBSJ {
	

	

	/***************************/
	TexturePacker::TexturePacker(int w, int h) :width(w), height(h)
	{
		binpack = make_shared<BinaryTreePack16>(width, height);
		
	}

	TexturePacker::~TexturePacker()
	{
		if (freeimage)
		{
			FreeImage_Unload(freeimage);
			freeimage = nullptr;
		}
	}

	int min2(int w) {
		int t = 1;
		while (t < 1024)
		{
			t = t << 1;
			if (w < t)
				return t;
		}
		return t;
	}

 
	//用一个纹理直接构造
	TexturePacker::TexturePacker(shared_ptr<ModelTexture> ptex, double scale) {

		width = ptex->width *  scale;
		height = ptex->height *  scale;

		//对图片做大小限制
		width = fmin(width, 1024);
		height = fmin(height, 1024);

		
		//为了考虑兼容性 这里对纹理进行2的幂次方合并  最大1024
		width = min2(width);
		height = min2(height);

		//对数据进行缩放
		freeimage = FreeImage_Rescale((FIBITMAP*)ptex->freeimage, width, height);
		//y 反转
		FreeImage_FlipVertical(freeimage);
		//拷贝纹理的贴图方式
		mode_s = ptex->mode_s;
		mode_t = ptex->mode_t;

		hasalpha = FreeImage_GetBPP(freeimage) == 32;


	//	FreeImage_Save(FIF_PNG, freeimage, "C:\\gisdata\\scene\\dae\\touming\\2.png");

	}

	Rect<float>   TexturePacker::packTexture(shared_ptr<ModelTexture> ptex, double scale) {
		
		Rect<float> ret;
		if (!ptex)
			return ret;

		if (!binpack)
			return ret;

		auto bpp = FreeImage_GetBPP((FIBITMAP*)ptex->freeimage);
		//根据第一个插入的纹理决定是否是透明的
		if (!freeimage) {
			freeimage = FreeImage_Allocate(width, height, bpp);
			hasalpha = bpp == 32;
		}

		//如果透明度不同，那么也不能合并
		if ((bpp == 32) != hasalpha) {
			return ret;
		}
	 

		

		int sw = ptex->width *  scale;
		int sh = ptex->height *  scale;

		//这里限制一下，纹理最大不能超过1024
		sw = fmin(width, sw);
		sh = fmin(height, sh);

		//获得分配区域
		auto rect = binpack->allocate(sw, sh);
		if (rect.w == 0 || rect.h == 0)
			return ret;


		//注意这里的y的范围，调试的时候再看
		ret.x = rect.x * 1.0f / width;
		ret.y = rect.y * 1.0f / height;
		ret.w = rect.w * 1.0f / width;
		ret.h = rect.h * 1.0f / height;

		//对texture进行缩放
		auto * simage = FreeImage_Rescale((FIBITMAP*)ptex->freeimage, sw, sh);

		FreeImage_FlipVertical(simage);

		/*
		//转为32位
		if (FreeImage_GetBPP(simage) != 32) {
			auto * simag32 = FreeImage_ConvertTo32Bits(simage);
			FreeImage_Unload(simage);
			simage = simag32;
		}
		*/
		//把数据拷贝过来 最后一个参数大于256 直接覆盖原始位置
		FreeImage_Paste(freeimage, simage, rect.x, rect.y, 256);
		FreeImage_Unload(simage);


	//	FreeImage_Save(FIF_PNG, freeimage, "C:\\gisdata\\scene\\dae\\1.png");

		return ret;
	}

	void  TexturePacker::updateImageData(bool crncompressed) {

		//如果是32位，那么保存为png 	//否则保存为 jpg
		imageType = hasalpha ? "image/png" : "image/jpeg";
		auto mem = FreeImage_OpenMemory();
		BOOL ret = FreeImage_SaveToMemory(hasalpha ? FIF_PNG : FIF_JPEG, freeimage, mem);
		if (!ret) {
			LOG(WARNING) << "FreeImage_SaveToMemory failed";
			return;
		}
		BYTE * data;
		DWORD len;
		if (!FreeImage_AcquireMemory(mem, &data, &len)) {
			FreeImage_CloseMemory(mem);
			LOG(WARNING) << "FreeImage_SaveToMemory failed";
			return;
		}
		imageData.resize(len);
		memcpy(const_cast<char*>(imageData.data()), data, len);
		FreeImage_CloseMemory(mem);

		//执行crn压缩
		if (crncompressed) {
			//auto data =  ModelTexture::tocrn(imageData);
			auto data = ModelTexture::towebp(imageData);
			if (!data.empty()) {

				LOG(INFO) << "webp compressed from " << imageData.size() << " to " << data.size();
				imageData = move(data);
				crn = true;
				imageType = "image/webp";
			}
		}
		return;
	}
}