#pragma once

#include "config.h"
#include <mutex>


std::string BIMMODELINPUT_API UTF8_To_string(const std::string & str);
std::string BIMMODELINPUT_API string_To_UTF8(const std::string & str);
int BIMMODELINPUT_API split(const std::string& str, std::vector<std::string>& ret_, std::string sep);
void BIMMODELINPUT_API replace(std::string& strSource, const std::string& strOld, const std::string& strNew);

std::string BIMMODELINPUT_API getFileFolder(const char *  filename);

//传入的路径必须是arcii的，不可以是utf8路径
bool BIMMODELINPUT_API mkDirs(const char * filename);
//传入的路径必须是arcii的，不可以是utf8路径 这个带文件名的
bool BIMMODELINPUT_API mkFileDirs(const char * filename);

//传入的路径必须是utf8的
bool BIMMODELINPUT_API mkFileDirsWithUTF8Path(const char * filename);

std::string  BIMMODELINPUT_API readAll(std::string &file);
bool BIMMODELINPUT_API  saveAll(std::string &file, std::string &data);

//获取cpu信息
int BIMMODELINPUT_API GetCPUCount();

 

template<typename T> std::string str(T value) {

	std::ostringstream strs;
	strs << value;
	std::string str = strs.str();

	return std::move(str);
}



class BIMMODELINPUT_API MyLocker {
public:
	MyLocker(std::mutex &m):mutex(m){
		mutex.lock();
	}
	~MyLocker(){
		mutex.unlock();
	}
private:
	std::mutex &mutex;
};

bool BIMMODELINPUT_API equals(std::string str1, std::string str2);

bool BIMMODELINPUT_API isAbsolutePath(std::string path);
std::string BIMMODELINPUT_API combinePath(std::string folder, std::string file);

std::string BIMMODELINPUT_API wstring2string(const std::wstring& ws);
std::wstring BIMMODELINPUT_API string2wstring(const std::string& s);
std::string BIMMODELINPUT_API loadResource(std::string  type, int id);

std::string BIMMODELINPUT_API newID();

