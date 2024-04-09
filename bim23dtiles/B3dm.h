#pragma once

#include "stdafx.h"

//https://github.com/AnalyticalGraphicsInc/3d-tiles/blob/master/TileFormats/Batched3DModel/README.md




class B3dm
{
public:
	B3dm();
	~B3dm();


	struct Header {
		char magic[4] = { 'b','3','d','m' };
		uint32_t version;
		uint32_t byteLength;
		uint32_t featureTableJSONByteLength;
		uint32_t featureTableBinaryByteLength;
		uint32_t batchTableJSONByteLength;
		uint32_t batchTableBinaryByteLength;

		bool valid() {
			return magic[0] == 'b' &&  magic[1] == '3' && magic[2] == 'd' && magic[3] == 'm';
		}

		json tojson() {
			json ret;

			ret["version"] = version;
			ret["byteLength"] = byteLength;
			ret["featureTableJSONByteLength"] = featureTableJSONByteLength;
			ret["featureTableBinaryByteLength"] = featureTableBinaryByteLength;
			ret["batchTableJSONByteLength"] = batchTableJSONByteLength;
			ret["batchTableBinaryByteLength"] = batchTableBinaryByteLength;

			return move(ret);
		}
	};

	struct BatchDataBin {
		string componentType;//["BYTE", "UNSIGNED_BYTE", "SHORT", "UNSIGNED_SHORT", "INT", "UNSIGNED_INT", "FLOAT", "DOUBLE"]
		string type;//["SCALAR", "VEC2", "VEC3", "VEC4"]
		string data;//实际二进制数据
	};
	//两种可能，要么是个json数组，要么是个二进制数组
	struct BatchData {
		string name;
		json  jsonData;
		BatchDataBin  binData;
	};

	// batchTable
	vector<shared_ptr<BatchData>>  batchDatas;

	//这个指定了  batchdata中的元素的个数，如果为0，表示逐点 和 positions大小一致
	uint32_t BATCH_LENGTH = 0;

	//用这个指定在加载过程中输出详细信息
	bool    reportInfo = false;



	bool load(istream & stm);
	bool save(ostream & stm, string & gltfcontent);

};

