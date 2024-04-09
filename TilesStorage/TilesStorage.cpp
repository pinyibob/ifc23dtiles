#include "TilesStorage.h"
#include "TilesStorageFile.h"
#include "TilesStorageMongodb.h"
#include "TilesStorageSqlite.h"

namespace XBSJ
{

	TilesStorage::TilesStorage() {

	}
	TilesStorage::~TilesStorage(){


	}

	bool  TilesStorage::init(json &config)
	{
		return false;
	}

	shared_ptr<TilesStorage> TilesStorage::createStorage(json &config, LogFunction func) {

		shared_ptr<TilesStorage> storage = nullptr;

		//根据config去判定
		
		//如果里包含sqlite 那么使用sqlite配置
		auto csqlite = config["sqlite"];
		if (csqlite.is_object()) {

			storage = make_shared<TilesStorageSqlite>();
			storage->logFunc = func;
			if (!storage->init(csqlite))
			{
				return nullptr;
			}
			return move(storage);
		}
		
		 //如果包含mongodb那么使用mongodb配置
		auto cmongodb = config["mongodb"];
		if (cmongodb.is_object()) {

			storage = make_shared<TilesStorageMongodb>();
			storage->logFunc = func;
			if (!storage->init(cmongodb))
			{
				return nullptr;
			}
			return move(storage);
		}
	 
		//否则使用文件存储

		storage = make_shared<TilesStorageFile>();
		storage->logFunc = func;
		if (!storage->init(config))
		{
			return nullptr;
		}
		return move(storage); 
	}
}