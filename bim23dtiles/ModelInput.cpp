#include "ModelInput.h"
#include "util.h"
#include "OsgUtil.h" 

#include <set>
#include <dlfcn.h>
#include <filesystem>

using namespace std;

namespace XBSJ {
	ModelInput::ModelInput(json & config, shared_ptr<ModelInputReader> & r, int index)
	{
		cinput = config;
		reader = r;
		readerIndex = index;
	}

	ModelInput::~ModelInput()
	{
		scene.reset();
	}

	bool  ModelInput::load() {

		//解析配置
		if (!reader || readerIndex < 0) {

			LOG(ERROR) << "reader object empty";
			return false;
		}

		//检测输入
		auto cinputfile = cinput["file"];
		if (!cinputfile.is_string()) {
			LOG(ERROR) << "input.file config failed";
			return false;
		}
		inputfile = cinputfile.get<string>();
		inputfile = UTF8_To_string(inputfile);



		//强制双面
		auto cforceDoubleSide = cinput["forceDoubleSide"];
		if (cforceDoubleSide.is_boolean()) {
			forceDoubleSide = cforceDoubleSide.get<bool>();
		}


		//纹理缩放比率
		auto ctextureScaleFactor = cinput["textureScaleFactor"];
		if (ctextureScaleFactor.is_number()) {
			auto v = ctextureScaleFactor.get<double>();
			if (v > 0.5 && v <= 2.0) {
				textureScaleFactor = v;
			}
		}


		//纹理缩放比率
		auto ctextureGeometricErrorFactor = cinput["textureGeometricErrorFactor"];
		if (ctextureGeometricErrorFactor.is_number()) {
			auto v = ctextureGeometricErrorFactor.get<double>();
			if (v > 1 && v <= 100) {
				textureGeometricErrorFactor = v;
			}
		}

		//自定义shader
		auto ccustomShader = cinput["customShader"];
		if (ccustomShader.is_boolean()) {
			customShader = ccustomShader.get<bool>();
		}
		//禁用光照
		auto cnolight = cinput["nolight"];
		if (cnolight.is_boolean()) {
			nolight = cnolight.get<bool>();
		}

		//分组优先级
		auto csplitPriority = cinput["splitPriority"];
		if (csplitPriority.is_string()) {
			auto v = csplitPriority.get<string>();
			if (v == "space" || v == "material") {
				splitPriority = v;
			}
		}

		//分组颗粒度
		auto csplitUnit = cinput["splitUnit"];
		if (csplitUnit.is_string()) {
			auto v = csplitUnit.get<string>();
			if (v == "mesh" || v == "node" || v == "none") {
				splitUnit = v;
			}
		}

		//是否保存filename
		auto csavefilename = cinput["savefilename"];
		if (csavefilename.is_boolean()) {
			saveFileName = csavefilename.get<bool>();
		}

		//filename
		auto cfilename = cinput["filename"];
		if (cfilename.is_string()) {
			fileName = cfilename.get<string>();
		}

		//汉字编码
		auto cencodeGBK = cinput["encodeGBK"];
		if (cencodeGBK.is_boolean()) {
			encodeGBK = cencodeGBK.get<bool>();
		}


		//最大数据量
		auto csplitMaxDataSize = cinput["splitMaxDataSize"];
		if (csplitMaxDataSize.is_number_unsigned()) {
			auto v = csplitMaxDataSize.get<unsigned int>();
			if (v >= 20000) {
				splitMaxDataSize = v;
			}
		}

		//是否进行三角面精简
		auto csimplifyMesh = cinput["simplifyMesh"];
		if (csimplifyMesh.is_string()) {
			auto v = csimplifyMesh.get<string>();
			if (v == "none" || v == "before" || v == "after") {
				simplifyMesh = v;
			}
		}


		//颜色倍率
		auto ccolorRatio = cinput["colorRatio"];
		if (ccolorRatio.is_number()) {
			auto v = ccolorRatio.get<double>();
			if (v > 0 && v < 10) {
				colorRatio = v;
			}
		}

		//配置投影参数
		configSrs(cinput);

		//--- 配置对象自己的变量 enuMatrix

		// 修改自己的 enuMatrix
		// osg::Matrixd enuMatrix;

		//加载场景
		auto taskrepoter = make_shared<ProgressHelper>("载入场景", 1, 0.9);

		scene = reader->loadScene(readerIndex);
		
		taskrepoter.reset();

		if (!scene)
			scene = make_shared<BimScene>();

		for (auto & e : scene->elements) {
			elementsBox[e->id] = e->caculBox();
		}
		if (scene->mapCoords.size()) {
			setSrs(scene->crs, scene->mapCoords);
		}
		else if(!scene->crs.empty()){
			setSrs(scene->crs, scene->coords, scene->angles);
		}

		//载入完成后，修改材质配置---此处应该对mesh中的材质做修改
		for (auto &m : scene->materials) {
			m->customShader = customShader;
			m->nolight = nolight;
			if (forceDoubleSide)
				m->doubleSide = true;
		}

		for (auto &e : scene->elements) {
			for (auto &mesh : e->meshes) {
				mesh->material->customShader = customShader;
				mesh->material->nolight = nolight;
				if (forceDoubleSide) {
					mesh->material->doubleSide = true;
				}
			}

		}

		return true;
	}

	bool ModelInput::GenInputs(list<shared_ptr<ModelInput>> & inputs, json & cinput,json & idsinput)
	 {
		//插件
		string plugin = "";

		auto cplugin = cinput["plugin"];
		if (cplugin.is_string()) {
			plugin = cplugin.get<string>();
		}

		auto cinputfile = cinput["file"];
		if (!cinputfile.is_string()) {
			LOG(ERROR) << "need file config";
			return false;
		}
		auto inputfile = cinputfile.get<string>();
		inputfile = UTF8_To_string(inputfile);
		auto ext = inputfile.substr(inputfile.find_last_of("."));
		transform(ext.begin(), ext.end(), ext.begin(), ::tolower);


		//如果没有插件，那么初始化所有插件
		if (ModelInputPlugin::plugins.empty()) {
			InitPlugins();
		}

		LOG(INFO) << "plugin count:" << ModelInputPlugin::plugins.size() << " need plugin:" << plugin;
		shared_ptr<ModelInputReader> reader = nullptr;
		//遍历所有插件寻找可以支持的 reader
		for (auto & p : ModelInputPlugin::plugins) {
			if (p->supportFormat(ext)) {

				if (plugin.empty() || plugin == p->name) {
					reader = p->createReader();

					LOG(INFO) << "find plugin: " << p->name << " to read :" << inputfile;
					break;
				}
			}
		}

		//怎加一点处理，如果说没有寻找到对应的plugin，那么只使用第一个后缀匹配的
		if (!reader && !plugin.empty()) {

			LOG(INFO) << "try find plugin to read ,ignore plugin setting,only match extion:" << inputfile;
			for (auto & p : ModelInputPlugin::plugins) {
				if (p->supportFormat(ext)) {

					reader = p->createReader();

					LOG(INFO) << "find plugin: " << p->name << " to read :" << inputfile;
					break;

				}
			}
		}

		if (!reader) {
			LOG(ERROR) << "can't construct  reader for :" << inputfile;
			return false;
		}

		cinput["exefolder"] = string_To_UTF8(ExeFolder);

		//2, 初始化
		if (!reader->init(cinput,idsinput)) {
			LOG(ERROR) << "reader init failed" << inputfile;
			return false;
		}

		//3，获取支持读取分割
		auto scnt = reader->getNumScene();
		if (scnt <= 0) {
			LOG(ERROR) << "reader scene empty";
			return false;
		}

		//4,构造输入
		for (unsigned int i = 0; i < scnt; i++) {
			auto input = make_shared<ModelInput>(cinput, reader, i);
			inputs.push_back(input);
		}


		return true;

	}

	string ModelInput::ExeFolder = "";
	
#ifdef WIN32
	typedef bool(_stdcall *pfnInitPlugin)();
#else
	typedef bool(__attribute__ ( (__stdcall__)) *pfnInitPlugin)();
#endif

	//载入exe统计目录下的所有插件
	bool ModelInput::InitPlugins() {

		//遍历目录下dll 所有以plugin开头的插件，尝试去获取初始化插件的方法，并执行一下
		using namespace std::filesystem;
#ifdef WIN32
		WIN32_FIND_DATA find;
		auto hError = FindFirstFile((ExeFolder + "\\*.dll").c_str(), &find);
		if (hError == INVALID_HANDLE_VALUE)
			return false;
		do
		{
			// 如果是目录
			if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			string filename = find.cFileName;

			if (filename.find("Plugin") == 0) {
				LOG(INFO) << "find plugin:" << filename;

				filename = ExeFolder + "\\" + filename;
				auto module = LoadLibrary(filename.c_str());
				std::cout << GetLastError() << endl;

				//寻找初始化函数，并执行
				pfnInitPlugin initfunc = (pfnInitPlugin)GetProcAddress(module, "?initPlugin@@YA_NXZ");
				if (initfunc)
				{
					if (!initfunc()) {
						LOG(WARNING) << "initPlugin failed:" << filename;
					}
				}
				else {
					LOG(WARNING) << "GetProcAddress failed:" << filename;
				}
			}

		} while (::FindNextFile(hError, &find));
#else
		using namespace std::filesystem;
		path exedir(ExeFolder);
		for(auto& itr : filesystem::directory_iterator(exedir))
		{
			if(std::filesystem::is_directory(itr.status()))
				continue;
			auto filename = itr.path().filename().string();
			size_t ipos = filename.find("libPlugin");
			if(ipos == 0)
			{
				void* dp = dlopen(filename.c_str(), RTLD_LAZY);
				if(dp)
				{
					pfnInitPlugin initfunc = (pfnInitPlugin)dlsym(dp, "_Z10initPluginv");
					if (initfunc)
					{
						if (!initfunc()) {
							LOG(WARNING) << "initPlugin failed:" << filename;
						}
					}
					else {
						LOG(WARNING) << "GetProcAddress failed:" << filename;
					}
				}
			}
		}
#endif
		return true;
	}

	/*
	void  ModelInput::sceneSpheres(SceneOutputConfig * config, list<json> & nodespheres) {

		//遍历场景树
		updateSphere(sceneTree);

		if (readerIndex == 0)
		{
			nodespheres.push_back(sceneTree);
		}
	}
	*/
	void ModelInput::fillSceneTree(json & sceneTree) {

		if (readerIndex == 0) {
			reader->fillSceneTree(sceneTree);
		}
		updateSphere(sceneTree);
	}

	void ModelInput::updateSphere(json & tree) {
		if (tree.is_null())
			return;
		if (tree.is_array()) {
			for (auto &c : tree) {
				updateSphere(c);
			}
			return;
		}
 
		if (tree.is_object())
		{
			auto  & sphere = tree["sphere"];
			if (!sphere.is_null()) {

				auto id = tree["id"];

				auto it = elementsBox.find(id.get<string>());

				if (it != elementsBox.end()) {

					auto c = it->second.center();

					c = toGloble(c);

					tree["sphere"] = { c.x(), c.y(),c.z() ,it->second.radius() };
				}
			}

		}

		auto & children = tree["children"];
		updateSphere(children);

	}

}