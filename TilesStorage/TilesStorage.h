#pragma once
#include <string>
#include <vector>
#include <memory>
#include <list>
#include <functional>
#include <json.hpp>

//using namespace std;
using namespace nlohmann;

// #ifdef TILESSTORAGE_EXPORTS
// #define TILESSTORAGE_API __declspec(dllexport)
// #else
// #define TILESSTORAGE_API __declspec(dllimport)
// #endif
using std::string;
using std::shared_ptr;
using std::make_shared;

#define TILESSTORAGE_API

namespace XBSJ {

	/*
	  ?????????›¥ 3d tiles????????????????????????????file??sqlite??mongodb
	*/
	class  TILESSTORAGE_API TilesStorage
	{
	public:
		TilesStorage();
		~TilesStorage();

		enum LogLevel {
			L_INFO,
			L_WARNING,
			L_ERROR
		};

		typedef class TILESSTORAGE_API std::function< void(string, LogLevel, string, long)> LogFunction;

	public:
		static std::shared_ptr<TilesStorage> createStorage(json &config, LogFunction func);
	
	public:
		virtual bool  saveJson(string file, json & content) = 0;
		virtual bool  saveFile(string file, string &content) = 0;
		virtual bool  saveSceneTree(json & scenetree) = 0;
	protected:
	    virtual	bool  init(json &config) = 0;

		LogFunction logFunc = nullptr;
		
	};

	#define OUTPUTLOG(a,b)  if(logFunc!=nullptr)logFunc(a,b,__FILE__,__LINE__);
}
