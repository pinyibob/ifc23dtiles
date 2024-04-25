#include "TilesStorageMongodb.h"
#include <mongoc/mongoc.h>

#define GETCONFIG(V) auto c##V=config[#V];if(c##V.is_string() && c##V.get<string>()!="")  V = c##V.get<string>();

#include "GZipCodec.h"
#include "util.h"
#include "md5.h"

using std::stringstream;
namespace XBSJ
{
	TilesStorageMongodb::TilesStorageMongodb()
	{
		mongoc_init();
	}


	TilesStorageMongodb::~TilesStorageMongodb()
	{
		/*
		if (jsonCollection) {
			mongoc_collection_destroy(jsonCollection);
			jsonCollection = nullptr;
		}
		if (tilesCollection) {
			mongoc_collection_destroy(tilesCollection);
			tilesCollection = nullptr;
		}

		if (treeCollection) {
			mongoc_collection_destroy(treeCollection);
			treeCollection = nullptr;
		}
		*/
		 
		if (gridfs) {

			mongoc_gridfs_destroy(gridfs);
			gridfs = nullptr;
		}
		 
		if (database) {
			mongoc_database_destroy(database);
			database = nullptr;
		}
		if (client) {
			mongoc_client_destroy(client);
			client = nullptr;
		}


		mongoc_cleanup();
	}

	bool  TilesStorageMongodb::init(json &config) {


		//��ȡ�������
		GETCONFIG(databaseName);
		//GETCONFIG(modelCollectionName);
	//	GETCONFIG(jsonCollectionName);
		//GETCONFIG(tilesCollectionName);
		GETCONFIG(treeCollectionName);
		GETCONFIG(modelName);
		GETCONFIG(modelID);
		GETCONFIG(gridfsName);

		auto czip = config["zip"];
		if (czip.is_boolean()) {
			zip = czip.get<bool>();
		}

		//http://mongoc.org/libmongoc/current/mongoc_uri_t.html 

		auto curl = config["url"];
		if (!curl.is_string()) {
			OUTPUTLOG("��Ҫ����url", L_ERROR);
			return false;
		}
		auto url = curl.get<string>();
		bson_error_t error;
		auto uri = mongoc_uri_new_with_error(url.c_str(), &error);
		if (!uri) {
			OUTPUTLOG("����urlʧ��:" + url + " error:" + error.message, L_ERROR);
			return false;
		}

		client = mongoc_client_new_from_uri(uri);
		if (!client) {
			OUTPUTLOG("get client ʧ��", L_ERROR);
			return false;
		}
		mongoc_uri_destroy(uri);


		mongoc_client_set_appname(client, "cesiumlab");



		database = mongoc_client_get_database(client, databaseName.c_str());

		//����һ��ping���� ������ݿ���������
		auto command = BCON_NEW("ping", BCON_INT32(1));
		bson_t  reply;
		auto	retval = mongoc_client_command_simple(client, "admin", command, NULL, &reply, &error);
		if (!retval) {
			OUTPUTLOG(string("connect db  ʧ��:") + error.message, L_ERROR);
			return false;
		}
		bson_destroy(&reply);
		bson_destroy(command);

		//��������������
		//jsonCollection = mongoc_client_get_collection(client, databaseName.c_str(), jsonCollectionName.c_str());
		//tilesCollection = mongoc_client_get_collection(client, databaseName.c_str(), tilesCollectionName.c_str());
		//treeCollection = mongoc_client_get_collection(client, databaseName.c_str(), treeCollectionName.c_str());

		//����gridfs�ļ�ϵͳ
		gridfs = mongoc_client_get_gridfs(client, databaseName.c_str(), gridfsName.c_str(), &error);

		//�½�һ��ģ����Ϣ��¼ ���������id
		auto modelCollection = mongoc_client_get_collection(client, databaseName.c_str(), modelCollectionName.c_str());

		struct tm t;    //tm�ṹָ��
		time_t now;     //����time_t���ͱ���
		time(&now);     //��ȡϵͳ���ں�ʱ��
		#ifdef WIN32
		localtime_s(&t, &now);   //��ȡ�������ں�ʱ��
		#else
		localtime_r(&now, &t);   //��ȡ�������ں�ʱ��
		#endif

		bson_oid_t oid;

		//���û������modelID����ô��ʼ��һ��
		if (modelID == "") {
			bson_oid_init(&oid, NULL);
			char buf[25];
			bson_oid_to_string(&oid, buf);
			modelID = buf;
		}
		else {
			//������ַ������ȡ
			bson_oid_init_from_string(&oid, modelID.c_str());
		}

		auto newModel = BCON_NEW(
			"_id", BCON_OID(&oid),
			"name", BCON_UTF8(modelName.c_str()),
			"date", BCON_DATE_TIME(mktime(&t) * 1000)
		);

		//�������ݿ�
		if (!mongoc_collection_insert_one(modelCollection, newModel, NULL, &reply, &error)) {
			OUTPUTLOG(string("inser model   ʧ��:") + error.message, L_ERROR);
			return EXIT_FAILURE;
		}

		bson_destroy(&reply);
		bson_destroy(newModel);
		mongoc_collection_destroy(modelCollection);
		 
		return  true;
	}

	bool  TilesStorageMongodb::saveJson(string filepath, json & content) {

		auto data = content.dump();

		return saveFile(filepath, data);
	}


	struct  mongoc_stream_string {
		mongoc_stream_t vtable;
		
		std::stringstream buffer;
		
		size_t left() {
			auto s = buffer.tellg();

			buffer.seekg(0, std::ios::end);

			auto e = buffer.tellg();
			buffer.seekg(s, std::ios::beg);

			return size_t(e-s);
		}
	};

	void mongoc_stream_string_destroy(mongoc_stream_t *stream) {

		mongoc_stream_string * str = (mongoc_stream_string *)stream;
		str->buffer.clear();
	}
	ssize_t mongoc_stream_string_read(mongoc_stream_t *stream,
		mongoc_iovec_t *iov,
		size_t iovcnt,
		size_t min_bytes,
		int32_t timeout_msec)
	{
		mongoc_stream_string * str = (mongoc_stream_string *)stream;

		ssize_t nread;
		size_t i;
		
		ssize_t ret = 0;

		size_t left = str->left();

		for (i = 0; i < iovcnt; i++) {
			//nread = _read(file->fd, iov[i].iov_base, iov[i].iov_len);
			
			if (iov[i].iov_len < left)
			{
				nread = iov[i].iov_len;
			}
			else {
				nread = left;
			}
			if(nread > 0)
				str->buffer.read((char*)iov[i].iov_base, nread);

			left -= nread;
		    
			if (nread < 0) {
				ret = ret ? ret : -1;
				goto RETURN;
			}
			else if (nread == 0) {
				ret = ret ? ret : 0;
				goto RETURN;
			}
			else {
				ret += nread;
				if (nread != iov[i].iov_len) {
					ret = ret ? ret : -1;
					goto RETURN;
				}
			}
		}	
	RETURN:

		return ret;
	}

	string getMD5(string & content) {

	 
		MD5 md5(content);

		return md5.toStr();
	}


	bool  TilesStorageMongodb::saveFile(string filepath, string &content) {

		if (!gridfs) {
			OUTPUTLOG("empty gridfs", L_ERROR);
			return false;
		}
		//�����·���޸���
		replace(filepath, "\\", "/");
		if (zip)
		{
			string ziped;

			if (!GZipCodec::Compress(content, ziped)) {

				OUTPUTLOG("zip failed", L_ERROR);
				return false;
			}
			content = move(ziped);
		}
		string filename = (modelID + "/" + filepath).c_str();


		/*
		auto opts = bson_new();
		BSON_APPEND_UTF8(opts, "bucketName", gridfsName.c_str());

		auto gridfs = mongoc_gridfs_bucket_new(database, opts, NULL, NULL);

		bson_value_t file_id;
		auto fileopts = bson_new();
		BSON_APPEND_UTF8(fileopts, "md5", gridfsName.c_str());

		auto  upload_stream = mongoc_gridfs_bucket_open_upload_stream(gridfs, filename.c_str(),  fileopts, &file_id, NULL);

		mongoc_stream_write(upload_stream, (void *)content.data(), content.size(), 0);
		mongoc_stream_destroy(upload_stream);
		mongoc_gridfs_bucket_destroy(gridfs);

		bson_destroy(opts);
		bson_destroy(fileopts);
		*/
	
		string md5 = getMD5(content);
		bson_error_t error;
  
		mongoc_gridfs_file_opt_t opt = {0};
		opt.filename = filename.c_str();
		opt.md5 = md5.c_str();

		//���ڸ㶨���Զ���stream
		mongoc_stream_string stream;
		stream.buffer = move(std::stringstream(content));
		stream.vtable = { 0 };
		stream.vtable.readv = mongoc_stream_string_read;
		stream.vtable.destroy = mongoc_stream_string_destroy;

		auto  file = mongoc_gridfs_create_file_from_stream(gridfs,(mongoc_stream_t*)&stream, &opt);

		if (!mongoc_gridfs_file_save(file)) {
			mongoc_gridfs_file_error(file, &error); 

			OUTPUTLOG(error.message, L_ERROR);
			return false;
		}

		mongoc_gridfs_file_destroy(file);
 
		return true;
	}

	void  insertDocument(mongoc_bulk_operation_t * bulk, json & node, string &modelid, char * parentid)
	{
		auto nid = node["id"];
		string id = "";
		if (nid.is_number_integer()) {
			stringstream ss;
			ss << nid.get<int>();
			id = ss.str();
		}
		else {
			id = nid.get<string>();
		}

		auto cname = node["name"];
		auto ctype = node["type"];
		auto cprops = node["props"];

		auto doc = BCON_NEW(
			"nid", BCON_UTF8(id.c_str()),
			"modelid", BCON_UTF8(modelid.c_str()),
			"parent", BCON_UTF8(parentid)
		);

		if (cname.is_string()) {
			BSON_APPEND_UTF8(doc, "name", cname.get<string>().c_str());
		}
		if (ctype.is_string()) {
			BSON_APPEND_UTF8(doc, "type", ctype.get<string>().c_str());
		}
		if (cprops.is_string()) {
			BSON_APPEND_UTF8(doc, "props", cprops.get<string>().c_str());
		}
		auto csphere = node["sphere"];
		if (csphere.is_array()) {
			double x = csphere[0].get<double>();
			double y = csphere[1].get<double>();
			double z = csphere[2].get<double>();
			double r = csphere[3].get<double>();

			BSON_APPEND_DOUBLE(doc, "x", x);
			BSON_APPEND_DOUBLE(doc, "y", y);
			BSON_APPEND_DOUBLE(doc, "z", z);
			BSON_APPEND_DOUBLE(doc, "r", r);
		}

		mongoc_bulk_operation_insert(bulk, doc);
		bson_destroy(doc);

		//��������������
		auto children = node["children"];
		if (!children.is_array())
			return;

		for (auto &j : children) {
			insertDocument(bulk, j, modelid, const_cast<char*>(id.c_str()));
		}
	}

	bool  TilesStorageMongodb::saveSceneTree(json & scenetree) {

		auto treeCollection = mongoc_client_get_collection(client, databaseName.c_str(), treeCollectionName.c_str());

		//�ݹ������������   id name type props sphere(x,y,z,r) parent
		auto bulk = mongoc_collection_create_bulk_operation_with_opts(treeCollection, NULL);

		bson_t reply;
		bson_error_t error;

		//��������
		insertDocument(bulk, scenetree, modelID, NULL);

		auto ret = mongoc_bulk_operation_execute(bulk, &reply, &error);

		auto str = bson_as_canonical_extended_json(&reply, NULL);
		{
			OUTPUTLOG(string("bulk insert reply:") + str, L_INFO);
		}
		bson_free(str);

		if (!ret) {

			OUTPUTLOG(string("bulk insert failed:") + error.message, L_ERROR);
		}

		bson_destroy(&reply);
		mongoc_bulk_operation_destroy(bulk);

		mongoc_collection_destroy(treeCollection);

		return ret != 0;
	}


}


