#include "stdafx.h"
#include "util.h"

#include <stdio.h>
//#include <tchar.h>
//#include <windows.h>
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

std::string  UTF8_To_string(const std::string & str)
{
	return str;
}

std::string  string_To_UTF8(const std::string & str)
{
	return str;
}

int split(const string& str, vector<string>& ret_, string sep)
{
	if (str.empty())
	{
		return 0;
	}

	string tmp;
	string::size_type pos_begin = str.find_first_not_of(sep);
	string::size_type comma_pos = 0;

	while (pos_begin != string::npos)
	{
		comma_pos = str.find(sep, pos_begin);
		if (comma_pos != string::npos)
		{
			tmp = str.substr(pos_begin, comma_pos - pos_begin);
			pos_begin = comma_pos + sep.length();
		}
		else
		{
			tmp = str.substr(pos_begin);
			pos_begin = comma_pos;
		}

		if (!tmp.empty())
		{
			ret_.push_back(tmp);
			tmp.clear();
		}
	}
	return 0;
}

void replace(std::string& strSource, const std::string& strOld, const std::string& strNew)
{
	size_t nPos = 0;
	while ((nPos = strSource.find(strOld, nPos)) != strSource.npos)
	{
		strSource.replace(nPos, strOld.length(), strNew);
		nPos += strNew.length();
	}
}

//#include <shlwapi.h>

// int GetCPUCount() {
// 	SYSTEM_INFO si;

// 	GetSystemInfo(&si);

// 	return si.dwNumberOfProcessors;
// }

std::string getFileFolder(const char *  filename)
{
	if (NULL == filename || strlen(filename) == 0)
	{
		return "";
	}
	
#if 0
	TCHAR szParentpath[MAX_PATH] = _T("");
	::lstrcpy(szParentpath, filename);

	::PathRemoveBackslash(szParentpath);//ȥ��·�����ķ�б��  
	::PathRemoveFileSpec(szParentpath);//��·��ĩβ���ļ������ļ��кͷ�б��ȥ�� 
	return string(szParentpath);
#endif
	path ip(filename);
	return ip.parent_path().string();
	
}

bool mkDirs(const char *  lpPath)
{
	if (NULL == lpPath || strlen(lpPath) == 0)
	{
		return false;
	}

	using namespace std::filesystem;
	path ip(lpPath);

	//if (::PathFileExists(lpPath) || ::PathIsRoot(lpPath))
	//	return true;

	// Ϊʲô�Ǹ�Ŀ¼Ҳ���أ�
	if (exists(ip) || ip == ip.root_directory())
		return true;

	auto paPath = ip.parent_path();
	if (paPath == ip)
		return false;

	//assert(0 != _tcscmp(lpPath, szParentpath));
	if (mkDirs(paPath.c_str()))//�ݹ鴴��ֱ����һ����ڻ��Ǹ�Ŀ¼  
	{
		return std::filesystem::create_directory(ip);
		//return ::CreateDirectory(lpPath, NULL);
	}
	else
	{
		return false;
	}

	return true;
}

bool   mkFileDirs(const char * filename) {

	if (NULL == filename || strlen(filename) == 0)
	{
		return false;
	}

	auto folder = getFileFolder(filename);

	return mkDirs(folder.c_str());
}
#include <mutex>

std::mutex  createfolder;
bool mkFileDirsWithUTF8Path(const char * filename) {
	if (NULL == filename || strlen(filename) == 0)
	{
		return false;
	}

	auto folder = getFileFolder(filename);

	folder = UTF8_To_string(folder);

	//�ڴ����ļ��е�ʱ����߳̿��ܻ�������Ӹ���
	createfolder.lock();
	bool ret = mkDirs(folder.c_str());
	createfolder.unlock();
	return ret;
}


bool saveAll(string &file, string &data) {
	ofstream ons(file, ios::binary);
	if (!ons)
		return false;

	ons.write(const_cast<char *>(data.data()), data.size());
	ons.close();
	return true;
}
std::string readAll(string &file) {
	ifstream ins(file, ios::binary);
	if (!ins)
		return "";

	ins.seekg(0, ios::end);
	auto size = (int)ins.tellg();
	ins.seekg(0, ios::beg);

	string buf;
	buf.resize(size);
	ins.read(const_cast<char *>(buf.data()), size);
	ins.close();
	return std::move(buf);
}
 
bool equals(string str1, string str2) {

	::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
	::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

	return str1 == str2;
}

bool isAbsolutePath(string ipath) {
	using std::filesystem::path;
	path ifile(ipath);
	return ifile.has_root_name();

	//return ipath.find(":\\") == 1 || ipath.find(":/") == 1;
}
string combinePath(string folder, string file) {
	using std::filesystem::path;

	std::filesystem::path ifile(folder);
	ifile.append(file);
	return ifile.string();
#if 0
	#if WIN32
		return folder + "\\" + file;
	#else
		return folder + '/' + file;
	#endif
#endif
}


 
#include <codecvt> 
string wstring2string(const wstring& wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> StringConverter;
	return StringConverter.to_bytes(wstr);
}

wstring string2wstring(const string& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> StringConverter;
	return StringConverter.from_bytes(str);
}


// string loadResource(string  type, int id) {

// 	auto s = MAKEINTRESOURCE(id);
// 	HRSRC    hRes = ::FindResource(NULL, s, type.c_str());
// 	if (!hRes)
// 		return "";
// 	HGLOBAL    hMem = ::LoadResource(NULL, hRes);
// 	DWORD    dwSize = ::SizeofResource(NULL, hRes);
// 	LPBYTE pData = (LPBYTE)LockResource(hMem);
// 	string ret;
// 	ret.resize(dwSize);

// 	memcpy(const_cast<char*>(ret.data()), pData, ret.size());

// 	return move(ret);
// }
#include "uuid4.h"
std::string   newID() {

	static bool hasinited = false;
	if (!hasinited) {
		uuid4_init();
		hasinited = true;
	}
	char buf[UUID4_LEN];
	uuid4_generate(buf);
	string gid(buf);
	replace(gid, "-", "");
	return move(gid);
}