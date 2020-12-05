#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

struct HDR
{
    std::string Name;
    uint32 Offset;
    uint32 SizeInBytes;
};

struct KoFile
{
    std::string Name;
    uint32 Size;
    KoFile(std::string _name, uint32 _size)
    {
        Name = _name;
        Size = _size;
    }
};

class HDRPacker
{
protected:
    char* m_hdr;
    size_t m_hdrSize;
    vector<KoFile> files;
private:
    char* ReadAllBytes(const char* filename, size_t* read);
    DWORD FindPatternEx(char* mem, size_t size, std::string search);
    void CheckHDR(std::string path);
    bool GetAllFiles(const char* sDir);
    uint32 GetFileSize(std::string filename);
public:
    HDRPacker();
    ~HDRPacker();
    void Pack();
    void PackObject();
    void PackItem();
    void PackFX();
    std::string RemoveBasePath(const std::string& filename);
};
