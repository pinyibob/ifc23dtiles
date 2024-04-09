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

 
	//��һ������ֱ�ӹ���
	TexturePacker::TexturePacker(shared_ptr<ModelTexture> ptex, double scale) {

		width = ptex->width *  scale;
		height = ptex->height *  scale;

		//��ͼƬ����С����
		width = fmin(width, 1024);
		height = fmin(height, 1024);

		
		//Ϊ�˿��Ǽ����� ������������2���ݴη��ϲ�  ���1024
		width = min2(width);
		height = min2(height);

		//�����ݽ�������
		freeimage = FreeImage_Rescale((FIBITMAP*)ptex->freeimage, width, height);
		//y ��ת
		FreeImage_FlipVertical(freeimage);
		//�����������ͼ��ʽ
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
		//���ݵ�һ���������������Ƿ���͸����
		if (!freeimage) {
			freeimage = FreeImage_Allocate(width, height, bpp);
			hasalpha = bpp == 32;
		}

		//���͸���Ȳ�ͬ����ôҲ���ܺϲ�
		if ((bpp == 32) != hasalpha) {
			return ret;
		}
	 

		

		int sw = ptex->width *  scale;
		int sh = ptex->height *  scale;

		//��������һ�£���������ܳ���1024
		sw = fmin(width, sw);
		sh = fmin(height, sh);

		//��÷�������
		auto rect = binpack->allocate(sw, sh);
		if (rect.w == 0 || rect.h == 0)
			return ret;


		//ע�������y�ķ�Χ�����Ե�ʱ���ٿ�
		ret.x = rect.x * 1.0f / width;
		ret.y = rect.y * 1.0f / height;
		ret.w = rect.w * 1.0f / width;
		ret.h = rect.h * 1.0f / height;

		//��texture��������
		auto * simage = FreeImage_Rescale((FIBITMAP*)ptex->freeimage, sw, sh);

		FreeImage_FlipVertical(simage);

		/*
		//תΪ32λ
		if (FreeImage_GetBPP(simage) != 32) {
			auto * simag32 = FreeImage_ConvertTo32Bits(simage);
			FreeImage_Unload(simage);
			simage = simag32;
		}
		*/
		//�����ݿ������� ���һ����������256 ֱ�Ӹ���ԭʼλ��
		FreeImage_Paste(freeimage, simage, rect.x, rect.y, 256);
		FreeImage_Unload(simage);


	//	FreeImage_Save(FIF_PNG, freeimage, "C:\\gisdata\\scene\\dae\\1.png");

		return ret;
	}

	void  TexturePacker::updateImageData(bool crncompressed) {

		//�����32λ����ô����Ϊpng 	//���򱣴�Ϊ jpg
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

		//ִ��crnѹ��
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