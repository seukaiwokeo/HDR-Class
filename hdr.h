#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "ByteBuffer.h"

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
public:
	HDRPacker();
	~HDRPacker();
    void Pack();
    void Unpack();
    DWORD FindPattern(std::string name);
};