#include "TilesStorageFile.h"
#include <fstream>
using namespace std;
#include <util.h>

namespace XBSJ
{
	TilesStorageFile::TilesStorageFile()
	{

	}


	TilesStorageFile::~TilesStorageFile()
	{

	}

	bool  TilesStorageFile::init(json &config) {
			
		//寻找path配置，并确定path路径存在
		auto cpath = config["path"];
		if (!cpath.is_string())
		{
			OUTPUTLOG("需要配置path", L_ERROR);
			return false;
		}

		folder = cpath.get<string>();
		folder = UTF8_To_string(folder);

		if (!mkDirs(folder.c_str()) )
		{
			OUTPUTLOG("mkDirs failed", L_ERROR);
			return false;
		}
 
		return  true;
	}
	bool  TilesStorageFile::saveJson(string file, json & content) {

		if (!isAbsolutePath(file))
		{
			file = combinePath(folder, file);
		}

		if (!mkFileDirs(file.c_str())) {

			OUTPUTLOG("mkDirs failed", L_ERROR);
			return false;
		}

		ofstream of(file);
		if (!of)
		{
			OUTPUTLOG("ofstream failed", L_ERROR);
			return false;
		}
			
#ifdef _DEBUG
		of << content.dump(4) << std::endl;
#else
		of << content.dump() << std::endl;
#endif
		of.close();

		OUTPUTLOG("file saved:" + file, L_INFO);

		return true;
	}

	bool  TilesStorageFile::saveSceneTree(json & scenetree)
	{
		return saveJson("scenetree.json", scenetree);
	}
	 bool  TilesStorageFile::saveFile(string file, string &content) {

		 if (!isAbsolutePath(file))
		 {
			 file = combinePath(folder, file);
		 }

		 if (!mkFileDirs(file.c_str())) {

			 OUTPUTLOG("mkDirs failed", L_ERROR);
			 return false;
		 }
		 ofstream of(file, ios::binary);
		 if (!of)
		 {
			 OUTPUTLOG("ofstream failed", L_ERROR);
			 return false;
		 }
		 of.write(const_cast<char *>(content.data()), content.size());
		 of.close();

		 OUTPUTLOG("file saved:" + file, L_INFO);

		 return true; 
	}
	 
}


