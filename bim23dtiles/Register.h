#pragma once
#include "stdafx.h"

namespace ETOP {

	class Register
	{
	public:
		Register();
		~Register();

		//static json tianAnMen();

		static string GetMachineCode();//?????????

		static bool checkLic(string &code);

		static bool GetAuthContent(json auth);

	};


}