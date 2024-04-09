#pragma once
#include "TilesStorage.h"

namespace XBSJ {

	class TilesStorageSqlite :public TilesStorage
	{
	public:
		TilesStorageSqlite();
		~TilesStorageSqlite();
	public:
		virtual bool  saveJson(string file, json & content) ;
		virtual bool  saveFile(string file, string &content);
		virtual bool  saveSceneTree(json & scenetree);
	protected:
		virtual	bool  init(json &config) ;
	};


}
