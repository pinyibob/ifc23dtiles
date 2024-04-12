// BimModelInput.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "BimModelInput.h"
 
#include <crnlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <webp/encode.h>

using namespace std;

//webp д�ļ��ص�
int webp_write_func(const uint8_t *data, size_t size, const WebPPicture *picture)
{
	string &buf = *(string *)picture->custom_ptr;
	auto offset = buf.size();
	buf.resize(offset + size);
	memcpy((void *)&buf[offset], data, size);
	return 1;
}

int getIntConfig(json &config, string key, int defaultV)
{
	auto c = config[key];
	if (c.is_number_integer())
		return c.get<int>();

	return defaultV;
}

namespace XBSJ {
 
	string  ModelTexture::tga2png(string &path) {

		FILE *f = stbi__fopen(path.c_str(), "rb");
		if (!f)
			return NULL;

		stbi__context s;
		stbi__start_file(&s, f);
		int w, h, comp;

		stbi__result_info ret;
		auto img = stbi__tga_load(&s, &w, &h, &comp, 0, &ret);
		fclose(f);
		if (!img)
		{
			return "";
		}


		int len = 0;
		auto data = stbi_write_png_to_mem((unsigned char *)img, 0, w, h, comp, &len);
		if (len == 0 || !data)
			return "";


		string retdata;
		retdata.resize(len);
		memcpy((void*)retdata.data(), data, len);

		STBI_FREE(data);

		return move(retdata);
	}
	string ModelTexture::tocrn(string & imagedata) {
		//1,�����ڴ����ͼƬ
		int width, height, actual_comps;
		crn_uint32 *pSrc_image = (crn_uint32*)stbi_load_from_memory((const stbi_uc *)imagedata.data(), imagedata.size(), &width, &height, &actual_comps, 4);
		if (!pSrc_image)
		{
			LOG(ERROR) << "stbi_load_from_memory failed load image ";
			return "";
		}
		//2������ѹ������
		bool has_alpha_channel = actual_comps > 3;
		float bitrate = 0.0f;
		int quality_level = -1;
		bool srgb_colorspace = true;
		bool create_mipmaps = true;
		bool output_crn = true;
		crn_format fmt = cCRNFmtInvalid;
		bool use_adaptive_block_sizes = true;
		bool set_alpha_to_luma = false;
		bool convert_to_luma = false;
		bool enable_dxt1a = false;
		const int cDefaultCRNQualityLevel = 128;


		if ((fmt == cCRNFmtDXT5A) && (actual_comps <= 3))
			set_alpha_to_luma = true;

		if ((set_alpha_to_luma) || (convert_to_luma))
		{
			for (int i = 0; i < width * height; i++)
			{
				crn_uint32 r = pSrc_image[i] & 0xFF, g = (pSrc_image[i] >> 8) & 0xFF, b = (pSrc_image[i] >> 16) & 0xFF;
				// Compute CCIR 601 luma.
				crn_uint32 y = (19595U * r + 38470U * g + 7471U * b + 32768) >> 16U;
				crn_uint32 a = (pSrc_image[i] >> 24) & 0xFF;
				if (set_alpha_to_luma) a = y;
				if (convert_to_luma) { r = y; g = y; b = y; }
				pSrc_image[i] = r | (g << 8) | (b << 16) | (a << 24);
			}
		}

		crn_comp_params comp_params;
		comp_params.m_width = width;
		comp_params.m_height = height;
		comp_params.set_flag(cCRNCompFlagPerceptual, srgb_colorspace);
		comp_params.set_flag(cCRNCompFlagDXT1AForTransparency, enable_dxt1a && has_alpha_channel);
		comp_params.set_flag(cCRNCompFlagHierarchical, use_adaptive_block_sizes);
		comp_params.m_file_type = output_crn ? cCRNFileTypeCRN : cCRNFileTypeDDS;
		comp_params.m_format = (fmt != cCRNFmtInvalid) ? fmt : (has_alpha_channel ? cCRNFmtDXT5 : cCRNFmtDXT1);
		//3, ִ��ѹ��
		// Important note: This example only feeds a single source image to the compressor, and it internaly generates mipmaps from that source image.
		// If you want, there's nothing stopping you from generating the mipmaps on your own, then feeding the multiple source images 
		// to the compressor. Just set the crn_mipmap_params::m_mode member (set below) to cCRNMipModeUseSourceMips.
		comp_params.m_pImages[0][0] = pSrc_image;

		if (bitrate > 0.0f)
			comp_params.m_target_bitrate = bitrate;
		else if (quality_level >= 0)
			comp_params.m_quality_level = quality_level;
		else if (output_crn)
		{
			// Set a default quality level for CRN, otherwise we'll get the default (highest quality) which leads to huge compressed palettes.
			comp_params.m_quality_level = cDefaultCRNQualityLevel;
		}

		// Determine the # of helper threads (in addition to the main thread) to use during compression. NumberOfCPU's-1 is reasonable.
#if WIN32
		SYSTEM_INFO g_system_info;
		GetSystemInfo(&g_system_info);
		int num_helper_threads = std::max<int>(0, (int)g_system_info.dwNumberOfProcessors - 1);
#else
		//https://blog.csdn.net/TanChengkai/article/details/101539122
		int num_helper_threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif
		//int num_helper_threads = std::max<int>(0, (int)g_system_info.dwNumberOfProcessors - 1);
		comp_params.m_num_helper_threads = num_helper_threads;

		//comp_params.m_pProgress_func = progress_callback_func;

		// Fill in mipmap parameters struct.
		crn_mipmap_params mip_params;
		mip_params.m_gamma_filtering = srgb_colorspace;
		mip_params.m_mode = create_mipmaps ? cCRNMipModeGenerateMips : cCRNMipModeNoMips;

		crn_uint32 actual_quality_level;
		float actual_bitrate;
		crn_uint32 output_file_size;

		// Now compress to DDS or CRN.
		void *pOutput_file_data = crn_compress(comp_params, mip_params, output_file_size, &actual_quality_level, &actual_bitrate);
		//4������ѹ������
		if (!pOutput_file_data)
		{
			stbi_image_free(pSrc_image);
			LOG(ERROR) << "crn_compress failed";
			return "";
		}

		string ret;
		ret.resize(output_file_size);
		memcpy(const_cast<char*>(ret.data()), pOutput_file_data, ret.size());
		crn_free_block(pOutput_file_data);
		stbi_image_free(pSrc_image);

		return move(ret);
	}

	string ModelTexture::towebp(string & imagedata) {
		//1,�����ڴ����ͼƬ
		int width, height, actual_comps;
		auto stbi_img = stbi_load_from_memory((const stbi_uc *)imagedata.data(), imagedata.size(), &width, &height, &actual_comps, 4);
		if (!stbi_img)
		{
			LOG(ERROR) << "stbi_load_from_memory failed";
			return "";
		}
		string ret;
		//2������ѹ������
		WebPPicture picture;
		WebPConfig config;
		if (WebPPictureInit(&picture) && WebPConfigInit(&config))
		{

			//������������Ʊ��������ͼ������
			config.lossless = 0;  //����
			config.quality = 80.0; // �����
			config.method = 4;	   // ����

			config.alpha_compression = 1;

			picture.use_argb = 1;
			picture.width = width;
			picture.height = height;
			picture.writer = webp_write_func;
			picture.custom_ptr = &ret;

			int suc = 0;
			if (actual_comps == 4)
			{
				//��ȡrgba
				suc = WebPPictureImportRGBA(&picture, stbi_img, 4 * width);
			}
			else
			{
				//ֻ��ȡrgb
				suc = WebPPictureImportRGBX(&picture, stbi_img, 4 * width);
			}
			//ѹ��
			if (suc && WebPEncode(&config, &picture))
			{
				//LOG(INFO) << "stbi_load_from_memory failed";
			}
			else
			{
				LOG(ERROR) << "webp encode failed";
				return "";
			}
			WebPPictureFree(&picture);
			
		}
		
		//�ͷ�ͼ��
		stbi_image_free(stbi_img);

		return move(ret);
	}
}