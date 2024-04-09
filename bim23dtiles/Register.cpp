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
//����Ȩֵ
int power(int R, int turn)
{
	int ans = 1;
	while (turn--)
	{
		ans = ans * R;
	}
	return ans;
}
//**************************************������������**********************************//
//�������תʮ����(All Radix to Int) ��������������ַ��� ����R ���ã����ð�Ȩչ���Ӻͣ�����ʮ����
int Atoi(string &S, int R)
{
	int ans = 0;
	for (int i = 0; i < S.size(); i++)//��Ȩչ��
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
//ʮ����ת������ƣ�Int to All Radix��������ʮ������Num,����R ���ã����ó����������������������������������Ҫת���Ľ�����
string Itoa(int Num, int R)
{
	string remain = "";
	int temp;
	do {
		temp = Num % R;//ȡ��
		Num /= R;
		if (temp >= 10)
			remain += temp - 10 + 'A';//�������Ϊ���ڻ�������10�Ľ��� ���磬ʮ������
		else remain += temp + '0';
	} while (Num);
	reverse(remain.begin(), remain.end());//����
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

//�����ı�
string getText(string filename) {
	ifstream infile(filename);
	string plaintext;
	getline(infile, plaintext);
	infile.close();
	return plaintext;
}

//������д���ı�
void writeCiphertext(string ciphertext, string fileName) {
	ofstream outfile(fileName, ofstream::out);
	outfile << ciphertext;
	outfile.close();
}
//������Կ
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
		
		//���洦������
		string authInfo;
		if (!filepath.empty()) {
			ifstream infile;
			infile.open(filepath, ios::in);
			//ifstream infile(filepath, ios::in);
			if (!infile) {
				cout << "��Ȩ�ļ���ʧ��:" << filepath << endl;
				infile.open(exefolder, ios::in);
				if (!infile) {
					cout << "��Ȩ�ļ���ʧ��:" << exefolder << endl;
					return 0;
				} 
				
				cout << "��Ȩ�ļ��򿪳ɹ�:" << exefolder << endl;
			}
			try
			{
				infile >> authInfo;
			}
			catch (const std::exception&)
			{
				infile.close();
				//errorOut("�����ļ�����ʧ��");
				cout << "�����ļ�����ʧ��:" << filepath << endl;
				return 0;
			}
			infile.close();
		}

		bool isAuth=checkLic(authInfo);

		//auto exefolder = getFileFolder

		return isAuth;
	}

	
	bool Register::checkLic(string & lic) {

		//1����ȡaeskey
		string  aesKeyHex = lic.substr(0, 64);
		vector<uint8_t> aesKey = hex_decode(aesKeyHex);

		//2����ȡ�����ı�����
		int  encDataLength = Atoi(lic.substr(64, 2), 16);

		//3����ȡ��Ȩ��Ϣ�����ַ�
		string encDataHex = lic.substr(66, encDataLength);
		vector<uint8_t> encData = hex_decode(encDataHex);
		
		//4����ȡRSAǩ��
		string signHex = lic.substr(66 + encDataLength);
		vector<uint8_t> signature = hex_decode(signHex);

		// verify signature
		Botan::PK_Verifier verifier(*publicKey, "EMSA1(SHA-256)");
		verifier.update(encData);

		if (!verifier.check_signature(signature)) {
			return false;
		}

		//������Ȩ��Ϣ--encData������Ϣ
		std::unique_ptr<Botan::BlockCipher> cipher(Botan::BlockCipher::create("AES-256"));
		cipher->set_key(aesKey);
		cipher->decrypt(encData);

		json encDataJson = json::parse(encData);
		//1���жϻ������Ƿ�һ��
		auto code = encDataJson["code"].get<string>();
		if (GetMachineCode() != code) {
			LOG(ERROR) << "��Ȩ����ʧЧ1:" << code << endl;
			return false;
		}
		//2���ж���Ч��
		//��ȡϵͳʱ��  
		time_t now_time = time(0);
		string &expTime = encDataJson["expTime"].get<string>();
		time_t exp_time = str_to_time_t(expTime);
		////��ȡ����ʱ��  
		//tm*  t_tm = localtime(&now_time);
		//time_t mk_time = mktime(t_tm);
		if (exp_time < now_time) {
			LOG(ERROR) << "��Ȩ����ʧЧ2" << endl;
			return false;
		}

		return true;
	}

	
	

	string Register::GetMachineCode() {

		string code = GetMachaineId().c_str();
		writeCiphertext(code,"������.txt");
		return code;
	}

}