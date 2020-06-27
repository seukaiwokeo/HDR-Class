#include "hdr.h"

HDRPacker::HDRPacker()
{

}

HDRPacker::~HDRPacker()
{

}

uint32 GetFileSize(std::string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : 0;
}

std::string basename(const std::string& filename)
{
	if (filename.empty()) {
		return {};
	}

	auto len = filename.length();
	auto index = filename.find_last_of("/\\");

	if (index == std::string::npos) {
		return filename;
	}

	if (index + 1 >= len) {

		len--;
		index = filename.substr(0, len).find_last_of("/\\");

		if (len == 0) {
			return filename;
		}

		if (index == 0) {
			return filename.substr(1, len - 1);
		}

		if (index == std::string::npos) {
			return filename.substr(0, len);
		}

		return filename.substr(index + 1, len - index - 1);
	}

	return filename.substr(index + 1, len - index);
}

static char* ReadAllBytes(const char* filename, int* read)
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

static bool startsWith(const string& s, const string& prefix) {
	return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

void HDRPacker::Pack()
{
	vector<KoFile> files;
	// import the .uif and .dxt files in the UI folder and push back to the vector
	std::string pattern("UI");
	std::string base = pattern;
	pattern.append("\\*.uif");
	WIN32_FIND_DATAA data;
	HANDLE hFind;
	if ((hFind = FindFirstFileA(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			files.push_back(KoFile(string(base + "/" + data.cFileName), GetFileSize(base + "\\" + data.cFileName)));
		} while (FindNextFileA(hFind, &data) != 0);
		FindClose(hFind);
	}

	pattern = "UI";
	base = pattern;
	pattern.append("\\*.dxt");
	ZeroMemory(&data, sizeof(data));
	hFind = NULL;
	if ((hFind = FindFirstFileA(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			files.push_back(KoFile(string(base + "/" + data.cFileName), GetFileSize(string(base + "\\" + data.cFileName))));
		} while (FindNextFileA(hFind, &data) != 0);
		FindClose(hFind);
	}

	if (files.size() == 0)
		return;

	bool firstCreation = false;
	// update the new file count in hdr
	uint32 fCount = 0;
	uint32 nCount;
	FILE* fp = fopen("UI\\ui.hdr", "r+b");
	if (fp != nullptr) {
		fseek(fp, 0, SEEK_SET);
		fread(&fCount, sizeof(fCount), 1, fp); // get total file count in hdr
		nCount = fCount + files.size(); // set the new one
		fseek(fp, 0, SEEK_SET);
		fwrite(&nCount, sizeof(nCount), 1, fp); // write it
		fclose(fp);
	}
	else {
		std::ofstream createHDR("UI\\ui.hdr");
		createHDR.close();
		nCount = files.size();
		fp = fopen("UI\\ui.hdr", "r+b");
		fwrite(&nCount, sizeof(nCount), 1, fp); // write the file count
		fclose(fp);
		firstCreation = true;
	}

	// packaging the files in vector
	uint32 updateCount = 0;
	int itr = 1;
	for (KoFile file : files)
	{
		// ----------- add informations to hdr
		uint32 startOffset = GetFileSize(string("UI\\ui.src"))); // get the last address of src
		uint32 SizeInBytes = file.Size;
		// does the file already exist in hdr?
		DWORD find = FindPattern(basename(file.Name));
		if (find != NULL && !firstCreation) // if it does, update it's information.
		{
			fp = fopen("UI\\ui.hdr", "r+b");
			fseek(fp, find + file.Name.size() - 3, SEEK_SET);
			fwrite(&startOffset, sizeof(startOffset), 1, fp);
			fseek(fp, find + file.Name.size() + 1 , SEEK_SET);
			fwrite(&SizeInBytes, sizeof(SizeInBytes), 1, fp);
			fclose(fp);
			updateCount++;
		}
		else { // if it doesn't, insert it
			ByteBuffer buff;
			buff << basename(file.Name) << startOffset << SizeInBytes;
			std::ofstream hdr("UI\\ui.hdr"), ios::binary | ios::app);
			hdr.write((char*)buff.contents(), buff.size());
			hdr.close();
		}
		// ----------- insert the data into src
		std::ofstream src("UI\\ui.src"), ios::binary | ios::app);
		std::ifstream _koFile(file.Name.c_str(), std::ios::binary);
		src << _koFile.rdbuf();
		src.close();
		_koFile.close();
		// remove file
		std::remove(file.Name.c_str());
		itr++;
	}

	if (updateCount > 0) // if is there any file which has been updated, decrase the file count incrased before from hdr because we didn't insert them. we updated.
	{
		if (fopen("UI\\ui.hdr", "r+b"))
		{
			uint32 currentFileCount;
			fseek(fp, 0x0, SEEK_SET);
			fread(&currentFileCount, sizeof(currentFileCount), 1, fp);
			currentFileCount -= updateCount;
			fseek(fp, 0x0, SEEK_SET);
			fwrite(&currentFileCount, sizeof(currentFileCount), 1, fp);
			fclose(fp);
		}
	}
}


DWORD HDRPacker::FindPattern(std::string fileName)
{
	int memSize;
	char* memory = ReadAllBytes("UI\\ui.hdr"), &memSize);

	char* pattern = (char*)fileName.c_str();
	uint32 mask = strlen(pattern);
	if (mask > memSize) return NULL;

	for (int i = 0; i < memSize; i++)
	{
		bool found = true;
		for (int j = 0; j < mask; j++)
			if (memory[i + j] != pattern[j])
				found = false;
		if (found)
			return i;
	}

	return NULL;
}

void HDRPacker::Unpack()
{
	vector<HDR> files;
	uint32 totalFile = 0;

	ByteBuffer content;
	BYTE buf;
	FILE* fp = fopen("UI\\ui.hdr", "rb");
	while (fread(&buf, sizeof(BYTE), 1, fp) == 1)
	{
		content.append(buf);
	}
	fclose(fp);

	// HDR files into vector
	content >> totalFile;
	for (uint32 i = 0; i < totalFile; i++) {
		HDR hdr;
		int32 nameLen;
		content >> nameLen;
		BYTE* name = new BYTE[nameLen];
		for (uint32 j = 0; j < nameLen; j++)
			content >> name[j];
		hdr.Name = (char*)name;
		content >> hdr.Offset >> hdr.SizeInBytes;
		files.push_back(hdr);
	}

	// unpack files those are in the vector
	for (HDR hdr : files)
	{
		BYTE* buff = new BYTE[hdr.SizeInBytes+1];

		fp = fopen("UI\\ui.src", "rb"); // open src file
		fseek(fp, hdr.Offset, SEEK_SET); // goto offset
		fread(&buff[0], sizeof(BYTE), (size_t)hdr.SizeInBytes, fp); // read the bytes from offset to size
		fclose(fp); // close the file

		ofstream file;
		file.open(string("UI\\" + hdr.Name).c_str(), ios::out | ios::binary); // save the data which has been readed before, as file
		file.write((const char*)buff, hdr.SizeInBytes);
		file.close();
	}
	
}
