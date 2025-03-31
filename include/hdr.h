#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <filesystem>
#include <map>
#include <wincrypt.h>
#include <format>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <iostream>

class CHDRSystem {
public:
    bool bRepair = false;
private:
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTKEY hKey = NULL;
    HCRYPTHASH hHash = NULL;
    int rCount = 0;
    bool bChanged = false;
    std::string m_strBasePath;
    std::vector<std::string> m_Dirs;

    bool InitCrypto();
    BOOL ReadCrypted(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped = nullptr) const;
    BOOL WriteCrypted(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWrite, LPOVERLAPPED lpOverlapped = nullptr) const;
    bool Prepare(const std::string& v);
public:
    static std::string GetDirectory(const std::string& path);
    static bool WriteDirectory(const std::string& dd);

public:
    CHDRSystem();
    ~CHDRSystem();

    void Pack();
    void Pack(const std::string& v, bool ignoreUnpack = false);
    void Unpack();
    void Unpack(const std::string& v);
};
