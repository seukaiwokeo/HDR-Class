#include "hdr.h"

bool is_file_exist(const char* fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

HDRPacker::HDRPacker()
{
	m_hdr = NULL;
	m_hdrSize = 0;
}

HDRPacker::~HDRPacker()
{
	m_hdr = NULL;
	m_hdrSize = 0;
}

uint32 HDRPacker::GetFileSize(std::string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : 0;
}

std::string ReplaceString(std::string subject, const std::string& search,
	const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

DWORD HDRPacker::FindPatternEx(char* mem, size_t size, std::string search)
{
	char* pattern = (char*)search.c_str();
	uint32 mask = search.length();
	if (mask > size) return NULL;
	for (size_t i = 0; i < size; i++)
	{
		bool found = true;
		for (int j = 0; j < mask; j++)
			if (tolower(mem[i + j]) != tolower(pattern[j]))
				found = false;
		if (found)
			return i;
	}
	return NULL;
}

char* HDRPacker::ReadAllBytes(const char* filename, size_t* read)
{
	ifstream ifs(filename, ios::binary | ios::ate);
	ifstream::pos_type pos = ifs.tellg();
	int length = pos;
	char* pChars = new char[length];
	ifs.seekg(0, ios::beg);
	ifs.read(pChars, length);
	ifs.close();
	*read = length;
	return pChars;
}

std::string HDRPacker::RemoveBasePath(const std::string& filename)
{
	size_t found;
	found = filename.find_first_of("\\");
	std::string tmp = filename.substr(found+1, filename.length());
	return ReplaceString(tmp, "/", "\\");
}

bool HDRPacker::GetAllFiles(const char* sDir)
{
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;
	char sPath[2048];
	sprintf(sPath, "%s\\*.*", sDir);

	if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
	{
		printf("Path not found: [%s]\n", sDir);
		return false;
	}
	do
	{
		if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0)
		{
			sprintf(sPath, "%s\\%s", sDir, fdFile.cFileName);
			if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				GetAllFiles(sPath);
			if (string(fdFile.cFileName).find(xorstr(".hdr")) == string::npos && string(fdFile.cFileName).find(xorstr(".src")) == string::npos && string(fdFile.cFileName).find(xorstr(".")) != string::npos && string(fdFile.cFileName) != xorstr(".") && string(fdFile.cFileName) != xorstr("..")) {
				files.push_back(KoFile(string(sPath), fdFile.nFileSizeLow));
			}
		}
	} while (FindNextFile(hFind, &fdFile));

	FindClose(hFind);
	return true;
}

void HDRPacker::CheckHDR(std::string path)
{
	if (!is_file_exist(path.c_str()))
	{
		CreateDirectory("UI", NULL);
		CreateDirectory("Object", NULL);
		CreateDirectory("Item", NULL);
		ofstream hdr(path.c_str());
		hdr.close();
		FILE* fp = fopen(path.c_str(), "r+b");
		fseek(fp, 0, SEEK_SET);
		int defaultSize = 0;
		fwrite(&defaultSize, 4, 1, fp);
		fclose(fp);
	}
}

void HDRPacker::Pack()
{
	CheckHDR(xorstr("UI\\ui.hdr"));
	GetAllFiles("UI");
	if (files.size() == 0)
		return;

	uint32 fCount = 0, nCount = 0, itr = 1;
	m_hdr = ReadAllBytes(xorstr("UI\\ui.hdr"), &m_hdrSize);
	memcpy(&fCount, &m_hdr[0], 4);
	FILE* fp = fopen(xorstr("UI\\ui.hdr"), "r+b");
	int startOffset = GetFileSize(string(xorstr("UI\\ui.src")));
	int SizeInBytes = 0;
	for (KoFile file : files)
	{
		Engine->SetState(string_format(xorstr("---> [%s]"), file.Name.c_str()));
		Engine->SetPercent(round(itr * 100 / files.size()));
		SizeInBytes = file.Size;
		std::string origName = RemoveBasePath(file.Name);
		DWORD find = FindPatternEx(m_hdr, m_hdrSize, origName);
		if (find != NULL)
		{
			fseek(fp, find + origName.length(), SEEK_SET);
			fwrite(&startOffset, 4, 1, fp);
			fseek(fp, find + origName.length() + 4, SEEK_SET);
			fwrite(&SizeInBytes, 4, 1, fp);
		}
		else {
			size_t nameLen = origName.length();
			char* name = (char*)malloc(sizeof(char) * nameLen);

			name = (char*)origName.c_str();
			fseek(fp, 0, SEEK_END);
			fwrite(&nameLen, 4, 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(name, sizeof(char), nameLen, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&startOffset, sizeof(startOffset), 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&SizeInBytes, sizeof(SizeInBytes), 1, fp);
			nCount++;
		}
		std::ofstream src(xorstr("UI\\ui.src"), ios::binary | ios::app);
		std::ifstream _koFile(file.Name.c_str(), std::ios::binary);
		src << _koFile.rdbuf();
		src.close();
		_koFile.close();
		std::remove(file.Name.c_str());
		startOffset += SizeInBytes;
		itr++;
	}
	if (nCount > 0)
	{
		fCount += nCount;
		fseek(fp, 0x0, SEEK_SET);
		fwrite(&fCount, 4, 1, fp);
	}
	fclose(fp);
	files.clear();
}

void HDRPacker::PackObject()
{
	CheckHDR(xorstr("Object\\object.hdr"));
	GetAllFiles("Object");
	if (files.size() == 0)
		return;
	uint32 fCount = 0, nCount = 0, itr = 1;
	m_hdr = ReadAllBytes(xorstr("Object\\object.hdr"), &m_hdrSize);
	memcpy(&fCount, &m_hdr[0], 4);
	FILE* fp = fopen(xorstr("Object\\object.hdr"), "r+b");
	int startOffset = GetFileSize(string(xorstr("Object\\object.src")));
	int SizeInBytes = 0;
	for (KoFile file : files)
	{
		Engine->SetState(string_format(xorstr("---> [%s]"), file.Name.c_str()));
		Engine->SetPercent(round(itr * 100 / files.size()));
		SizeInBytes = file.Size;
		std::string origName = RemoveBasePath(file.Name);
		DWORD find = FindPatternEx(m_hdr, m_hdrSize, origName);
		if (find != NULL)
		{
			fseek(fp, find + origName.length(), SEEK_SET);
			fwrite(&startOffset, 4, 1, fp);
			fseek(fp, find + origName.length() + 4, SEEK_SET);
			fwrite(&SizeInBytes, 4, 1, fp);
		}
		else {
			size_t nameLen = origName.length();
			char* name = (char*)malloc(sizeof(char) * nameLen);

			name = (char*)origName.c_str();
			fseek(fp, 0, SEEK_END);
			fwrite(&nameLen, 4, 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(name, sizeof(char), nameLen, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&startOffset, sizeof(startOffset), 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&SizeInBytes, sizeof(SizeInBytes), 1, fp);
			nCount++;
		}
		std::ofstream src(xorstr("Object\\object.src"), ios::binary | ios::app);
		std::ifstream _koFile(file.Name.c_str(), std::ios::binary);
		src << _koFile.rdbuf();
		src.close();
		_koFile.close();
		std::remove(file.Name.c_str());
		startOffset += SizeInBytes;
		itr++;
	}
	if (nCount > 0)
	{
		fCount += nCount;
		fseek(fp, 0x0, SEEK_SET);
		fwrite(&fCount, 4, 1, fp);
	}
	fclose(fp);
	files.clear();
}

void HDRPacker::PackItem()
{
	CheckHDR(xorstr("Item\\item.hdr"));
	GetAllFiles("Item");
	if (files.size() == 0)
		return;
	uint32 fCount = 0, nCount = 0, itr = 1;
	m_hdr = ReadAllBytes(xorstr("Item\\item.hdr"), &m_hdrSize);
	memcpy(&fCount, &m_hdr[0], 4);
	FILE* fp = fopen(xorstr("Item\\item.hdr"), "r+b");
	int startOffset = GetFileSize(string(xorstr("Item\\item.src")));
	int SizeInBytes = 0;
	for (KoFile file : files)
	{
		Engine->SetState(string_format(xorstr("---> [%s]"), file.Name.c_str()));
		Engine->SetPercent(round(itr * 100 / files.size()));
		SizeInBytes = file.Size;
		std::string origName = RemoveBasePath(file.Name);
		DWORD find = FindPatternEx(m_hdr, m_hdrSize, origName);
		if (find != NULL)
		{
			fseek(fp, find + origName.length(), SEEK_SET);
			fwrite(&startOffset, 4, 1, fp);
			fseek(fp, find + origName.length() + 4, SEEK_SET);
			fwrite(&SizeInBytes, 4, 1, fp);
		}
		else {
			size_t nameLen = origName.length();
			char* name = (char*)malloc(sizeof(char) * nameLen);

			name = (char*)origName.c_str();
			fseek(fp, 0, SEEK_END);
			fwrite(&nameLen, 4, 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(name, sizeof(char), nameLen, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&startOffset, sizeof(startOffset), 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&SizeInBytes, sizeof(SizeInBytes), 1, fp);
			nCount++;
		}
		std::ofstream src(xorstr("Item\\item.src"), ios::binary | ios::app);
		std::ifstream _koFile(file.Name.c_str(), std::ios::binary);
		src << _koFile.rdbuf();
		src.close();
		_koFile.close();
		std::remove(file.Name.c_str());
		startOffset += SizeInBytes;
		itr++;
	}
	if (nCount > 0)
	{
		fCount += nCount;
		fseek(fp, 0x0, SEEK_SET);
		fwrite(&fCount, 4, 1, fp);
	}
	fclose(fp);
	files.clear();
}

void HDRPacker::PackFX()
{
	CheckHDR(xorstr("FX\\fx.hdr"));
	GetAllFiles("FX");
	if (files.size() == 0)
		return;
	uint32 fCount = 0, nCount = 0, itr = 1;
	m_hdr = ReadAllBytes(xorstr("FX\\fx.hdr"), &m_hdrSize);
	memcpy(&fCount, &m_hdr[0], 4);
	FILE* fp = fopen(xorstr("FX\\fx.hdr"), "r+b");
	int startOffset = GetFileSize(string(xorstr("FX\\fx.src")));
	int SizeInBytes = 0;
	for (KoFile file : files)
	{
		Engine->SetState(string_format(xorstr("---> [%s]"), file.Name.c_str()));
		Engine->SetPercent(round(itr * 100 / files.size()));
		SizeInBytes = file.Size;
		std::string origName = RemoveBasePath(file.Name);
		DWORD find = FindPatternEx(m_hdr, m_hdrSize, origName);
		if (find != NULL)
		{
			fseek(fp, find + origName.length(), SEEK_SET);
			fwrite(&startOffset, 4, 1, fp);
			fseek(fp, find + origName.length() + 4, SEEK_SET);
			fwrite(&SizeInBytes, 4, 1, fp);
		}
		else {
			size_t nameLen = origName.length();
			char* name = (char*)malloc(sizeof(char) * nameLen);

			name = (char*)origName.c_str();
			fseek(fp, 0, SEEK_END);
			fwrite(&nameLen, 4, 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(name, sizeof(char), nameLen, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&startOffset, sizeof(startOffset), 1, fp);
			fseek(fp, 0, SEEK_END);
			fwrite(&SizeInBytes, sizeof(SizeInBytes), 1, fp);
			nCount++;
		}
		std::ofstream src(xorstr("FX\\fx.src"), ios::binary | ios::app);
		std::ifstream _koFile(file.Name.c_str(), std::ios::binary);
		src << _koFile.rdbuf();
		src.close();
		_koFile.close();
		std::remove(file.Name.c_str());
		startOffset += SizeInBytes;
		itr++;
	}
	if (nCount > 0)
	{
		fCount += nCount;
		fseek(fp, 0x0, SEEK_SET);
		fwrite(&fCount, 4, 1, fp);
	}
	fclose(fp);
	files.clear();
}
