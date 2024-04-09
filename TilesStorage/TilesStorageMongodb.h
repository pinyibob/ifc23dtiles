#pragma once
#include "TilesStorage.h"

struct _mongoc_client_t;
struct _mongoc_database_t;
struct _mongoc_collection_t;
struct _mongoc_gridfs_t;

namespace XBSJ {

	class TilesStorageMongodb :public TilesStorage
	{
	public:
		TilesStorageMongodb();
		~TilesStorageMongodb();
	public:
		virtual bool  saveJson(string file, json & content) ;
		virtual bool  saveFile(string file, string &content);
		virtual bool  saveSceneTree(json & scenetree);
	protected:
		virtual	bool  init(json &config) ;

		_mongoc_client_t *client = nullptr;
		_mongoc_database_t *database = nullptr;
		//_mongoc_collection_t *jsonCollection = nullptr;
		//_mongoc_collection_t * tilesCollection = nullptr;
		//_mongoc_collection_t *treeCollection = nullptr;

		_mongoc_gridfs_t   *   gridfs = nullptr;


		string databaseName = "bimstorage";


		string modelCollectionName = "modeltiles";
		//string jsonCollectionName = "jsonfiles";
		//string tilesCollectionName = "bimtiles";
		string treeCollectionName = "scenetree";
		string modelName = "models";

		string gridfsName = "tiles";
		bool   zip = true;

		//开始之后获得模型id
		string modelID = "";
	};


}
