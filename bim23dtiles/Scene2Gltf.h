#pragma once
#include "stdafx.h" 
namespace XBSJ {

	class SubScenePacker;
	class SceneOutputConfig;
	class Scene2Gltf
	{
	public:
		Scene2Gltf(SceneOutputConfig * cfg);
		~Scene2Gltf();


		bool write(shared_ptr<SubScenePacker> group, stringstream & buffer, string & jsoncontent);
 
		//bool writeV2(shared_ptr<SubScenePacker> group, stringstream & sss, string & jsoncontent);

		 
	private:
		SceneOutputConfig * config = nullptr;
	};


}