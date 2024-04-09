#include "Register.h"
#include <iostream>
#include <atlbase.h>
#include "stdafx.h"
#include <fstream>
#include <Windows.h>
#include <botan/botan.h>
#include <botan/hex.h>
#include <botan/cipher_mode.h>
#include <botan/rsa.h>
#include <botan/pubkey.h>
using namespace Botan;


string GetMachaineId() {
	//DWORD   serial;
	DWORD   n;
	char    guid[MAX_PATH];
	HKEY    hSubKey;

	n = sizeof(guid);

	RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Cryptography", 0,
		KEY_READ | KEY_WOW64_64KEY, &hSubKey);

	RegQueryValueExA(hSubKey, "MachineGuid", NULL,
		NULL, (LPBYTE)guid, &n);

	RegCloseKey(hSubKey);
	//printf ("MachineGuid: %s\n", guid);
	return guid;
}
//进制权值
int power(int R, int turn)
{
	int ans = 1;
	while (turn--)
	{
		ans = ans * R;
	}
	return ans;
}
//**************************************基本操作函数**********************************//
//任意进制转十进制(All Radix to Int) 参数：任意进制字符串 进制R 作用：利用按权展开加和，返回十进制
int Atoi(string &S, int R)
{
	int ans = 0;
	for (int i = 0; i < S.size(); i++)//按权展开
	{
		if (S[i] - '0' >= 0 && S[i] - '0' <= 9) {
			ans += (S[i] - '0') * power(R, S.size() - i - 1);
		}
		else {
			ans += (S[i] - 'A' + 10) * power(R, S.size() - i - 1);
		}
		//ans += (S[i] - '0') * power(R, S.size() - i - 1);
	}

	return ans;
}
//十进制转任意进制（Int to All Radix）参数：十进制数Num,进制R 作用：利用除留余数法，返回逆序的余数，即返回所要转换的进制数
string Itoa(int Num, int R)
{
	string remain = "";
	int temp;
	do {
		temp = Num % R;//取余
		Num /= R;
		if (temp >= 10)
			remain += temp - 10 + 'A';//任意进制为大于基数大于10的进制 例如，十六进制
		else remain += temp + '0';
	} while (Num);
	reverse(remain.begin(), remain.end());//逆序
	return remain;
}

time_t str_to_time_t(const string& ATime, const string& AFormat = "%d-%d-%d")
{
	struct tm tm_Temp;
	time_t time_Ret;
	try
	{
		int i = sscanf(ATime.c_str(), AFormat.c_str(),// "%d/%d/%d %d:%d:%d" ,
			&(tm_Temp.tm_year),
			&(tm_Temp.tm_mon),
			&(tm_Temp.tm_mday),
			&(tm_Temp.tm_hour),
			&(tm_Temp.tm_min),
			&(tm_Temp.tm_sec),
			&(tm_Temp.tm_wday),
			&(tm_Temp.tm_yday));

		tm_Temp.tm_year -= 1900;
		tm_Temp.tm_mon--;
		tm_Temp.tm_hour = 24;
		tm_Temp.tm_min = 0;
		tm_Temp.tm_sec = 0;
		tm_Temp.tm_isdst = 0;
		time_Ret = mktime(&tm_Temp);
		return time_Ret;
	}
	catch (...) {
		return 0;
	}
}

//读入文本
string getText(string filename) {
	ifstream infile(filename);
	string plaintext;
	getline(infile, plaintext);
	infile.close();
	return plaintext;
}

//创建并写入文本
void writeCiphertext(string ciphertext, string fileName) {
	ofstream outfile(fileName, ofstream::out);
	outfile << ciphertext;
	outfile.close();
}
//公共密钥
std::string pemDataText = u8R"///(-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCmgwF8tgR7sQCv8UzYIVUtK5qV
7OuhhBL73KA/2vzZSBkRu9LeEeP0SseMHpo0e8v7dEKFufr0BFz5K2dyvIHfXBl8
kxo7h5gWeom1OUmJzVJ0yzoDjP9ZZfkGXVW2wRYEeztpbGp2PM7+iimPUbIhCLKK
yxIhXYIo9BOYEB+mgwIDAQAB
-----END PUBLIC KEY-----)///";


std::vector<uint8_t> pemData(pemDataText.data(), pemDataText.data() + pemDataText.length());
unique_ptr<Public_Key> publicKey{ Botan::X509::load_key(pemData) };

namespace ETOP {
	Register::Register()
	{
	}


	Register::~Register()
	{
	}

	bool Register::GetAuthContent(json  auth) {
		auto inputfile = auth["file"];

		if (!inputfile.is_string()) {
			LOG(ERROR) << "need file config";
			return false;
		}
		auto filepath = inputfile.get<string>();
		//inputfile = UTF8_To_string(inputfile);
		auto exefolder = auth["exefolder"].get<string>()+"\\etop.lic";
		
		//常规处理任务
		string authInfo;
		if (!filepath.empty()) {
			ifstream infile;
			infile.open(filepath, ios::in);
			//ifstream infile(filepath, ios::in);
			if (!infile) {
				cout << "授权文件打开失败:" << filepath << endl;
				infile.open(exefolder, ios::in);
				if (!infile) {
					cout << "授权文件打开失败:" << exefolder << endl;
					return 0;
				} 
				
				cout << "授权文件打开成功:" << exefolder << endl;
			}
			try
			{
				infile >> authInfo;
			}
			catch (const std::exception&)
			{
				infile.close();
				//errorOut("任务文件解析失败");
				cout << "任务文件解析失败:" << filepath << endl;
				return 0;
			}
			infile.close();
		}

		bool isAuth=checkLic(authInfo);

		//auto exefolder = getFileFolder

		return isAuth;
	}

	
	bool Register::checkLic(string & lic) {

		//1、截取aeskey
		string  aesKeyHex = lic.substr(0, 64);
		vector<uint8_t> aesKey = hex_decode(aesKeyHex);

		//2、获取加密文本长度
		int  encDataLength = Atoi(lic.substr(64, 2), 16);

		//3、获取授权信息加密字符
		string encDataHex = lic.substr(66, encDataLength);
		vector<uint8_t> encData = hex_decode(encDataHex);
		
		//4、获取RSA签名
		string signHex = lic.substr(66 + encDataLength);
		vector<uint8_t> signature = hex_decode(signHex);

		// verify signature
		Botan::PK_Verifier verifier(*publicKey, "EMSA1(SHA-256)");
		verifier.update(encData);

		if (!verifier.check_signature(signature)) {
			return false;
		}

		//解码授权信息--encData明文信息
		std::unique_ptr<Botan::BlockCipher> cipher(Botan::BlockCipher::create("AES-256"));
		cipher->set_key(aesKey);
		cipher->decrypt(encData);

		json encDataJson = json::parse(encData);
		//1、判断机器码是否一致
		auto code = encDataJson["code"].get<string>();
		if (GetMachineCode() != code) {
			LOG(ERROR) << "授权码已失效1:" << code << endl;
			return false;
		}
		//2、判断有效期
		//获取系统时间  
		time_t now_time = time(0);
		string &expTime = encDataJson["expTime"].get<string>();
		time_t exp_time = str_to_time_t(expTime);
		////获取本地时间  
		//tm*  t_tm = localtime(&now_time);
		//time_t mk_time = mktime(t_tm);
		if (exp_time < now_time) {
			LOG(ERROR) << "授权码已失效2" << endl;
			return false;
		}

		return true;
	}

	
	

	string Register::GetMachineCode() {

		string code = GetMachaineId().c_str();
		writeCiphertext(code,"机器码.txt");
		return code;
	}

}