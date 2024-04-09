#pragma once
#include "TilesStorage.h"

namespace XBSJ {

	class TilesStorageFile :public TilesStorage
	{
	public:
		TilesStorageFile();
		~TilesStorageFile();
	public:
		virtual bool  saveJson(string file, json & content) ;
		virtual bool  saveFile(string file, string &content);
		virtual bool  saveSceneTree(json & scenetree);
	protected:
		virtual	bool  init(json &config) ;
		string folder;
	};


}
