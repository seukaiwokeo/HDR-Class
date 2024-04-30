#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <map>
#include <format>
#include <wincrypt.h>

class CHDRSystem
{
public:
	bool bRepair = false;
private:
	HCRYPTPROV hCryptProv = NULL;
	HCRYPTKEY hKey = NULL;
	HCRYPTHASH hHash = NULL;
	int rCount, bChanged;
	std::string m_strBasePath;
	std::vector<std::string> m_Dirs;
private:
	bool InitCrypto();
	BOOL ReadCrypted(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	BOOL WriteCrypted(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	bool Prepare(const std::string& v);
public:
	CHDRSystem();
	~CHDRSystem();
	void Pack();
	void Pack(const std::string& v, bool ignoreUnpack = false);
	void Unpack();
	void Unpack(const std::string& v);
private:
	std::string GetDirectory(const std::string& path);
	BOOL WriteDirectory(const std::string& dd);
};
