#include "B3dm.h"


//https://github.com/AnalyticalGraphicsInc/3d-tiles/blob/master/TileFormats/Batched3DModel/README.md

B3dm::B3dm()
{
}


B3dm::~B3dm()
{
}


//["BYTE", "UNSIGNED_BYTE", "SHORT", "UNSIGNED_SHORT", "INT", "UNSIGNED_INT", "FLOAT", "DOUBLE"]
//["SCALAR", "VEC2", "VEC3", "VEC4"]
int getEleSize(string & componentType, string & type) {

	int  csize = 1;
	if (componentType == "BYTE" || componentType == "UNSIGNED_BYTE")
		csize = 1;
	else if (componentType == "SHORT" || componentType == "UNSIGNED_SHORT")
		csize = 2;
	else if (componentType == "INT" || componentType == "UNSIGNED_INT" || componentType == "FLOAT")
		csize = 4;
	else if (componentType == "DOUBLE")
		csize = 4;
	else
		LOG(ERROR) << "��֧�ֵ�componentType:" << componentType;

	int tsize = 1;
	if (type == "SCALAR")
		tsize = 1;
	else if (type == "VEC2")
		tsize = 2;
	else if (type == "VEC3")
		tsize = 3;
	else if (type == "VEC4")
		tsize = 4;
	else
		LOG(ERROR) << "��֧�ֵ�type:" << type;

	return tsize * csize;
}


bool B3dm::load(istream & stm) {

	//���ļ�
	Header header;
	stm.read((char*)&header, sizeof(Header));
	//��֤
	if (!header.valid()) {

		LOG(ERROR) << "��������";
		return false;
	}
	string ftjson;

	if (header.featureTableJSONByteLength > 0) {
		ftjson.resize(header.featureTableJSONByteLength);
		stm.read(const_cast<char *>(ftjson.data()), ftjson.size());
	}


	//��ȡfeatureTableBinary

	string ftbin;
	if (header.featureTableBinaryByteLength > 0) {
		ftbin.resize(header.featureTableBinaryByteLength);
		stm.read(const_cast<char *>(ftbin.data()), ftbin.size());
	}


	//����featuretable
	if (!ftjson.empty()) {


		try {

			json featureTableJSON;
			featureTableJSON = json::parse(ftjson);
			LOG_IF(INFO, reportInfo) << endl << "featureTableJSON:" << featureTableJSON.dump(4);
			//����BATCH_LENGTH
			if (featureTableJSON.find("BATCH_LENGTH") != featureTableJSON.end()) {
				BATCH_LENGTH = featureTableJSON["BATCH_LENGTH"].get<uint32_t>();
			}

		}
		catch (exception ex) {
			LOG(ERROR) << ex.what();
			return false;
		}
	}

	//��ȡbatchTableJSON
	if (header.batchTableJSONByteLength > 0) {

		//https://github.com/AnalyticalGraphicsInc/3d-tiles/tree/master/TileFormats/BatchTable	
		//��������洢����ӳ��
		//���json������������
		//1, һ������ ���ȵ���batchlength
		//2, һ������ byteOffset, componentType, type �������Ե�ӳ�䣬ָ���˸��ֶδ洢��batchTableBinary��
		json batchTableJSON;
		//�����洢һЩ�������͵�����  
		string batchTableBinary;
		string btjson;
		btjson.resize(header.batchTableJSONByteLength);
		stm.read(const_cast<char *>(btjson.data()), btjson.size());

		if (header.batchTableBinaryByteLength > 0) {

			//�ⲿ�ֲο� https://github.com/AnalyticalGraphicsInc/3d-tiles/tree/master/TileFormats/BatchTable ����
			batchTableBinary.resize(header.batchTableBinaryByteLength);
			stm.read(const_cast<char *>(batchTableBinary.data()), batchTableBinary.size());
		}

		//����
		try {
			batchTableJSON = json::parse(btjson);
			//LOG_IF(INFO, reportInfo) << endl << "batchTableJSON:" << batchTableJSON.dump(4);
			//�������
			for (json::iterator it = batchTableJSON.begin(); it != batchTableJSON.end(); ++it) {
				string key = it.key();
				auto obj = it.value();

				shared_ptr<BatchData> bd = make_shared<BatchData>();
				bd->name = key;

				if (obj.is_array() || obj.is_object()) {
					bd->jsonData = obj;
				}
				else {
					//��ȡoffset
					int byteOffset = obj["byteOffset"].get<int>();
					string  componentType = obj["componentType"].get<string>();
					string  type = obj["type"].get<string>();
					//����componentType �� type BATCH_LENGTH �����С
					//���BATCH_LENGTH Ϊ 0, ��ô�������
					int size = getEleSize(componentType, type) * BATCH_LENGTH;
					//��������
					bd->binData.componentType = componentType;
					bd->binData.type = type;
					bd->binData.data.resize(size);
					memcpy(const_cast<char *>(bd->binData.data.data()), const_cast<char *>(batchTableBinary.data()) + byteOffset, size);
				}
				batchDatas.push_back(bd);
			}
		}
		catch (exception ex) {
			LOG(ERROR) << ex.what();
			return false;
		}
	}

	return true;
}
size_t componentTypeSize(string componentType) {

	if (componentType == "BYTE" || componentType == "UNSIGNED_BYTE")
		return 1;

	else if (componentType == "SHORT" || componentType == "UNSIGNED_SHORT")
		return 2;

	else if (componentType == "INT" || componentType == "UNSIGNED_INT")
		return 4;
	else if (componentType == "FLOAT")
		return 4;
	else
		return 8;

}

bool B3dm::save(ostream& stm, string& gltfcontent) {

	//����featureTableBinary
	string featureTableBinary;
	//����featureTableJSON
	json featureTableJSON;
	featureTableJSON["BATCH_LENGTH"] = BATCH_LENGTH;
	string jtjson = featureTableJSON.dump();
	while (jtjson.size() % 8 != 0) {
		jtjson.push_back(' ');
	}

	//batchTabel
	json batchTableJSON;
	string batchTableBinary;
	for (auto & d : batchDatas) {
		//�����json
		if (!d->jsonData.is_null()) {
			batchTableJSON[d->name] = d->jsonData;
		}
		//������
		else {
			json bin;
			bin["componentType"] = d->binData.componentType;
			bin["type"] = d->binData.type;

			size_t csize = componentTypeSize(d->binData.componentType);
			while (batchTableBinary.size() % csize != 0) {
				batchTableBinary.push_back(0);
			}
			auto offset = batchTableBinary.size();
			bin["byteOffset"] = offset;

			batchTableJSON[d->name] = bin;
			batchTableBinary.resize(offset + d->binData.data.size());

			memcpy(const_cast<char *>(batchTableBinary.data()) + offset, d->binData.data.data(), d->binData.data.size());
		
			while (batchTableBinary.size() % 8 != 0) {
				batchTableBinary.push_back(0);
			}
		}
	}

	//����btjson
	string btjson;
	if (!batchTableJSON.is_null()) {
		btjson = batchTableJSON.dump();
		//�ֽڶ���
		while (btjson.size() % 8 != 0) {
			btjson.push_back(' ');
		}
		LOG_IF(INFO, reportInfo) << endl << "batchTableJSON:" << batchTableJSON.dump(4);
	}

	//����header
	Header header;
	header.version = 1;
	header.featureTableJSONByteLength = jtjson.size();
	header.featureTableBinaryByteLength = featureTableBinary.size();
	header.batchTableJSONByteLength = btjson.size();
	header.batchTableBinaryByteLength = batchTableBinary.size();

	//�����ܳ���
	header.byteLength = sizeof(Header) + header.batchTableBinaryByteLength + header.batchTableJSONByteLength +
		header.featureTableJSONByteLength + header.featureTableBinaryByteLength + gltfcontent.size();
	
	
	//д��header
	stm.write((char*)&header, sizeof(Header));

	//д��featureTableJSON
	stm.write(const_cast<char *>(jtjson.data()), jtjson.size());
	//д��featureTableBinary
	if (!featureTableBinary.empty())
		stm.write(const_cast<char *>(featureTableBinary.data()), featureTableBinary.size());
	if (!btjson.empty())
		stm.write(const_cast<char *>(btjson.data()), btjson.size());
	if (!batchTableBinary.empty())
		stm.write(const_cast<char *>(batchTableBinary.data()), batchTableBinary.size());
	
	stm.write(gltfcontent.data(), gltfcontent.size());
 
	return true;
}
