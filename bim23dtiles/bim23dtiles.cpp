// ifc23dtiles.cpp : ??????????��????????
//

#include "stdafx.h"
//#include <Windows.h>
#include <gflags/gflags.h>
//#include <XBSJRegister.h>

#include "Register.h"
using namespace ETOP;

#include <chrono>
using namespace std::chrono;

#include<ctime>

//#ifdef XBSJ_TASK
DEFINE_string(taskname, "task_pdtile", "?????????");
DEFINE_string(taskserver, "tcp://127.0.0.1:5555", "??????????");
//#endif


DEFINE_string(task, "", "???????");
DEFINE_bool(machinecode, false, "?????????????");

DEFINE_string(shellids, "", "???ids");

#include "util.h" 
#include "ModelInput.h"
#include "SceneOutputConfig.h"

using namespace XBSJ;

#include <filesystem>
using namespace std::filesystem;


void errorOut(string error) {

	LOG(ERROR) << error << ":" << FLAGS_task;

	error = string_To_UTF8(error);
	json ret;
	ret["error"] = error;
	cout << ret.dump(4) << endl;
}



int main(int argc, char * argv[])
{
 
	//1??????????
	auto exefolder = getFileFolder(argv[0]);
	if (exefolder.empty())
		exefolder = ".";

#if WIN32
	ModelInput::ExeFolder = exefolder + "\\";
#else
	ModelInput::ExeFolder = exefolder + "/";
#endif

	google::InitGoogleLogging(argv[0]);
	//????????????????
	//FLAGS_alsologtostderr = 1;
	//???????exe??????
	if (FLAGS_log_dir == "")
		FLAGS_log_dir = exefolder;

	gflags::SetVersionString("1.0.1");
	gflags::SetUsageMessage("usage??bim23dtiles.exe --task=c:\\work\\task.json");
	google::ParseCommandLineFlags(&argc, &argv, false);
	gflags::ParseCommandLineFlags(&argc, &argv, false);


	//LOG(ERROR) << "?????????" << ETOP::Register::GetMachineCode() << endl;

	//?????
	if (FLAGS_task.empty()) {

		errorOut("task ??????");
		return 0;
	}


	//???��??????
	json taskConfig;
	if (!FLAGS_task.empty()) {
		ifstream infile(FLAGS_task, ios::in);
		if (!infile) {
			errorOut("????????????");
			return 0;
		}
		try
		{
			infile >> taskConfig;
		}
		catch (const std::exception&)
		{
			infile.close();
			errorOut("??????????????");
			return 0;
		}
		infile.close();
	}

	//???ids
	json idsConfig;
	if (!FLAGS_shellids.empty()) {
		ifstream infile(FLAGS_shellids, ios::in);
		if (!infile) {
			errorOut("???ID????????");
			return 0;
		}
		try
		{
			infile >> idsConfig;
		}
		catch (const std::exception&)
		{
			infile.close();
			errorOut("???ID??????????");
			return 0;
		}
		infile.close();
	}

	//?????��????????
	auto cdelete = taskConfig["delete"];
	if (cdelete.is_boolean()) {
		if (cdelete.get<bool>()) {
			std::filesystem::path ifile(FLAGS_task.c_str());
			#if 0
			if (!::DeleteFile()) {
				LOG(WARNING) << "DeleteFile failed";
			}
			#else
			if(!std::filesystem::remove(ifile))
				LOG(WARNING) << "DeleteFile failed";

			#endif
		}
	}


	//??????????????????????????????????????????
	// auto cauth = taskConfig["auth"];
	// if (!cauth.is_object()) {
	// 	errorOut("???auth????");
	// 	return 0;
	// }

	//????????????
	auto cinputs = taskConfig["inputs"];
	if (!cinputs.is_array()) {
		ProgressHelper::finish();
		LOG(ERROR) << "input config failed";
		return false;
	}
	
	bool needPayMode = true;
	/*
	bool needPayMode = false;
	//?????????????????srs????enu??????????????paymode
	for (size_t i = 0; i < cinputs.size(); i++)
	{
		auto input = cinputs[i];
		if (!input.is_object()) {
			continue;
		}
		auto csrs = input["srs"];
		if (csrs.is_string()) {
			auto srs = csrs.get<string>();
			if (srs.find("ENU:") == string::npos) {
				needPayMode = true;
				break;
			}
		}
	}
	*/

	
	//#define UNAUTH 0

//#ifndef UNAUTH
//??????
	//cauth["tool"] = "bim23tiles";

	//ETOP::Register::createLic();
	// cauth["exefolder"] = exefolder;

	// string authcontent;
	// if (!ETOP::Register::GetAuthContent(cauth)) {
	// 	errorOut("auth error");
	// 	LOG(ERROR) << "?????????" << ETOP::Register::GetMachineCode() << endl;
	// 	return 0;
	// }
	

//#endif 


	//?��?????????????��???????? enu???


	//??��???????????????????????ok?????????? 
	json ret;
	ret["auth"] = "ok";
	cout << ret.dump() << endl;


	ProgressHelper::init(FLAGS_taskserver, FLAGS_taskname);

	ProgressHelper pglobal("bime23dtiles", 1);
 
	clock_t startTime, endTime;
	startTime = clock();//??????

	//???????
	shared_ptr<SceneOutputConfig>  output = make_shared<SceneOutputConfig>();
	if (!output->config(taskConfig)) {
		ProgressHelper::finish();
		errorOut("output config failed");
		return 0;
	}
 
 

	//?????????????????��???
	{

		//1, ??????????????��??
		list<shared_ptr<ModelInput>>  inputs;


		for (size_t i = 0; i < cinputs.size(); i++)
		{
			if (!ModelInput::GenInputs(inputs, cinputs[i],idsConfig)) {
				LOG(WARNING) << "gen inputs failed:"<<i;
			}
		}
 
		ProgressHelper pinputs("process inputs", inputs.size(), 0.95);
		size_t i = 0;


		for (auto it = inputs.begin(); it != inputs.end();)
		{
			auto input = *it;

			i++;
			ProgressHelper pinput("process input:" + str(i), 1);

			//1?????????????
			{
				ProgressHelper pinput("load input:" + str(i), 1, 0.8);

				if (!input->load()) {
					LOG(WARNING) << "input config failed";
					inputs.erase(it++);
					continue;
				}
			}
			
			//2?????????????
			{
				ProgressHelper pinput("process input:" + str(i), 1, 0.2);
				if (!output->process(input, i)) {
					errorOut("writeTileset failed");
					inputs.erase(it++);
					continue;
				}
			}
			

			inputs.erase(it++);
	
			
		}
 
	}

	//????????
	{
		ProgressHelper pinputs("postProcess", 1, 0.05);
		if (!output->postProcess()) {
			errorOut("postProcess failed");
		}
	}


	LOG(INFO) << "finished";
	ProgressHelper::finish();
	endTime = clock();//???????
	cout << "The run time is: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
	return 0;
}

