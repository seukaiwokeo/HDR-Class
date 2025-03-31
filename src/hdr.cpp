#include "hdr.h"
#include <format>
#include <wincrypt.h>
#include <fstream>
#include <thread>

#ifndef MS_ENHANCED_PROV
#define MS_ENHANCED_PROV "Microsoft Enhanced Cryptographic Provider v1.0"
#endif

using namespace std::string_literals;

CHDRSystem::CHDRSystem() {
	std::array<char, MAX_PATH> WP{};
	if (GetCurrentDirectoryA(MAX_PATH, WP.data())) {
		m_strBasePath = std::string(WP.data()) + "\\";
	}

	const auto pathIni = m_strBasePath + "Path.ini";
	int dirCount = GetPrivateProfileIntA("Dir", "count", 0, pathIni.c_str());

	m_Dirs.reserve(dirCount);
	for (int i = 0; i < dirCount; i++) {
		std::array<char, MAX_PATH> v{};
		GetPrivateProfileStringA("Dir", std::to_string(i).c_str(), "", v.data(), MAX_PATH, pathIni.c_str());

		std::string b = v.data();
		if (!b.empty()) {
			m_Dirs.push_back(std::move(b));
		}
	}
}

CHDRSystem::~CHDRSystem() {
	if (bChanged) {
		rCount--;
		if (rCount < 0 || rCount > 10) rCount = 0;

		const auto pathIni = m_strBasePath + "Path.ini";
		WritePrivateProfileStringA("Version", "Count", std::to_string(rCount).c_str(), pathIni.c_str());
	}

	if (hKey) CryptDestroyKey(hKey);
	if (hHash) CryptDestroyHash(hHash);
	if (hCryptProv) CryptReleaseContext(hCryptProv, 0);
}

bool CHDRSystem::InitCrypto() {
    constexpr BYTE KEY_DATA[] = "owsd9012%$1as!wpow1033b%!@%12";
    constexpr DWORD KEY_DATA_LEN = sizeof(KEY_DATA) - 1;
    
    if (!CryptAcquireContextW(&hCryptProv, nullptr, MS_ENHANCED_PROV, PROV_RSA_FULL, 0)) {
        if (!CryptAcquireContextW(&hCryptProv, nullptr, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
            return false;
        }
    }

    if (!CryptCreateHash(hCryptProv, CALG_SHA, 0, 0, &hHash)) {
        CryptReleaseContext(hCryptProv, 0);
        return false;
    }
    
    if (!CryptHashData(hHash, KEY_DATA, KEY_DATA_LEN, CRYPT_USERDATA)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        return false;
    }

    if (!CryptDeriveKey(hCryptProv, CALG_RC4, hHash, 0x00800000, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        return false;
    }

    return true;
}

BOOL CHDRSystem::ReadCrypted(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) const {
	if (!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)) {
		return FALSE;
	}

	 return CryptDecrypt(hKey, hHash, TRUE, 0, static_cast<BYTE*>(lpBuffer), lpNumberOfBytesRead);
}

BOOL CHDRSystem::WriteCrypted(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWrite, LPOVERLAPPED lpOverlapped) const {
	if (!CryptEncrypt(hKey, hHash, TRUE, 0, static_cast<BYTE*>(lpBuffer), lpNumberOfBytesWrite, nNumberOfBytesToWrite)) {
		return FALSE;
	}

	return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWrite, lpOverlapped);
}

void CHDRSystem::Pack() {
	const auto pathIni = m_strBasePath + "Path.ini";
	rCount = GetPrivateProfileIntA("Version", "Count", 10, pathIni.c_str());
	if (rCount < 0 || rCount > 10) {
		rCount = 0;
	}

	for (const auto& v : m_Dirs) {
		Pack(v);
	}
}

bool CHDRSystem::Prepare(const std::string& v) {
    const auto path = std::format("{}{}\\", m_strBasePath, v);
    std::filesystem::remove(std::format("{}{}2.hdr", path, v));
    std::filesystem::remove(std::format("{}{}2.src", path, v));

    if (rCount == 0 || bRepair) {
        return false;
    }

    return true;
}

void CHDRSystem::Unpack() {
	for (const auto& v : m_Dirs) {
		Unpack(v);
	}
}

void CHDRSystem::Unpack(const std::string& v) {
    rCount = 10;

    const std::string path = m_strBasePath + v + "\\";
    const auto hdrPath = std::format("{}{}.hdr", path, v);
    const auto srcPath = std::format("{}{}.src", path, v);

    auto hFile = CreateFileA(hdrPath.c_str(), FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    auto hSource = CreateFileA(srcPath.c_str(), FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE || hSource == INVALID_HANDLE_VALUE) {
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        if (hSource != INVALID_HANDLE_VALUE) CloseHandle(hSource);
        return;
    }

    DWORD dwCount = 0;
    DWORD version = 0;
    DWORD rowCount = 0;

    auto cleanup = [&]() {
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        if (hSource != INVALID_HANDLE_VALUE) CloseHandle(hSource);
        std::filesystem::remove(hdrPath);
        std::filesystem::remove(srcPath);
        };

    if (ReadFile(hFile, &version, sizeof(version), &dwCount, nullptr)) {
        if (version < 100000) {
            SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
        }
        else {
            if (!InitCrypto()) {
                cleanup();
                return;
            }
        }

        if (hCryptProv != 0) {
            ReadCrypted(hFile, &rowCount, sizeof(rowCount), &dwCount);
        }
        else if (!ReadFile(hFile, &rowCount, sizeof(rowCount), &dwCount, nullptr)) {
            cleanup();
            return;
        }

        std::vector<BYTE> buffer;
        for (DWORD i = 0; i < rowCount; i++) {
            try {
                DWORD nameLen = 0;
                if (!ReadFile(hFile, &nameLen, hCryptProv == 0 ? sizeof(nameLen) : 2, &dwCount, nullptr)) {
                    break;
                }

                std::string fileName(nameLen, 0);
                if (!ReadFile(hFile, fileName.data(), nameLen, &dwCount, nullptr)) {
                    break;
                }

                DWORD offset = 0, length = 0;
                if (!ReadFile(hFile, &offset, sizeof(offset), &dwCount, nullptr)) {
                    break;
                }
                if (!ReadFile(hFile, &length, sizeof(length), &dwCount, nullptr)) {
                    break;
                }

                const auto fullPath = std::format("{}{}", path, fileName);
                if (fileName.find('\\') != std::string::npos && !WriteDirectory(GetDirectory(fullPath))) {
                    continue;
                }

                auto nFile = CreateFileA(fullPath.c_str(), FILE_WRITE_DATA, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (nFile == INVALID_HANDLE_VALUE) {
                    continue;
                }

                SetFilePointer(hSource, offset, nullptr, FILE_BEGIN);
                buffer.resize(length);

                if (!ReadFile(hSource, buffer.data(), length, &dwCount, nullptr) ||
                    !WriteFile(nFile, buffer.data(), length, &dwCount, nullptr)) {
                    CloseHandle(nFile);
                    continue;
                }

                CloseHandle(nFile);
            }
            catch (const std::exception&) {
                continue;
            }
        }
    }

    cleanup();
}

void CHDRSystem::Pack(const std::string& v, bool ignoreUnpack) {
    if (!Prepare(v) && !ignoreUnpack) {
        Unpack(v);
    }

    const std::string path = m_strBasePath + v + "\\";
    const auto hdrPath = std::format("{}{}.hdr", path, v);
    const auto srcPath = std::format("{}{}.src", path, v);

    auto hFile = CreateFileA(hdrPath.c_str(), FILE_READ_DATA | FILE_WRITE_DATA, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    auto hSource = CreateFileA(srcPath.c_str(), FILE_READ_DATA | FILE_WRITE_DATA, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE || hSource == INVALID_HANDLE_VALUE) {
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        if (hSource != INVALID_HANDLE_VALUE) CloseHandle(hSource);
        return;
    }

    std::unordered_map<std::string, DWORD> m_hdrFileList;
    DWORD version = 200000;
    DWORD rowCount = 0;
    DWORD rowBefore = 0;
    DWORD dwCount = 0;

    if (ReadFile(hFile, &version, sizeof(version), &dwCount, nullptr)) {
        if (version < 100000) {
            SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
        }
        else if (!InitCrypto()) {
            CloseHandle(hFile);
            CloseHandle(hSource);
            return;
        }

        if (hCryptProv != NULL) {
            ReadCrypted(hFile, &rowCount, sizeof(rowCount), &dwCount);
        }
        else if (!ReadFile(hFile, &rowCount, sizeof(rowCount), &dwCount, nullptr)) {
            CloseHandle(hFile);
            CloseHandle(hSource);
            return;
        }

        if (rowCount == 0) {
            SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
            WriteFile(hFile, &version, sizeof(version), &dwCount, nullptr);
            WriteFile(hFile, &rowCount, sizeof(rowCount), &dwCount, nullptr);
        }
        else {
            rowBefore = rowCount;
            for (DWORD i = 0; i < rowCount; i++) {
                DWORD nameLen = 0;
                if (!ReadFile(hFile, &nameLen, hCryptProv == NULL ? sizeof(nameLen) : 2, &dwCount, nullptr)) {
                    break;
                }

                std::string fileName(nameLen, 0);
                if (!ReadFile(hFile, fileName.data(), nameLen, &dwCount, nullptr)) {
                    break;
                }

                DWORD offsetPointer = SetFilePointer(hFile, 0, nullptr, FILE_CURRENT);

                std::string lowerFileName = fileName;
                std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(),
                    [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

                m_hdrFileList[lowerFileName] = offsetPointer;
                SetFilePointer(hFile, 8, nullptr, FILE_CURRENT);
            }
        }
    }

    std::vector<BYTE> buffer;
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (!entry.is_directory()) {
                try {
                    const auto fileName = entry.path().filename().string();
                    if (fileName.ends_with(".hdr") || fileName.ends_with(".src")) {
                        continue;
                    }
                }
                catch (const std::filesystem::filesystem_error&) {
                    continue;
                }

                const auto fullPath = entry.path().string();
                const auto name = fullPath.substr(path.length(), fullPath.length() - path.length());

                auto hTemp = CreateFileA(fullPath.c_str(), FILE_READ_DATA | FILE_WRITE_DATA,
                    FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, nullptr);

                if (hTemp == INVALID_HANDLE_VALUE) {
                    continue;
                }

                std::string lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                    [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

                auto itr = m_hdrFileList.find(lowerName);
                const DWORD offset = SetFilePointer(hSource, 0, nullptr, FILE_END);
                const DWORD length = GetFileSize(hTemp, nullptr);

                if (itr != m_hdrFileList.end()) {
                    SetFilePointer(hFile, itr->second, nullptr, FILE_BEGIN);
                    if (!WriteFile(hFile, &offset, sizeof(offset), &dwCount, nullptr) ||
                        !WriteFile(hFile, &length, sizeof(length), &dwCount, nullptr)) {
                        CloseHandle(hTemp);
                        continue;
                    }
                }
                else {
                    const DWORD nameLen = static_cast<DWORD>(name.length());
                    SetFilePointer(hFile, 0, nullptr, FILE_END);

                    if (!WriteFile(hFile, &nameLen, hCryptProv == NULL ? sizeof(nameLen) : 2, &dwCount, nullptr) ||
                        !WriteFile(hFile, name.c_str(), nameLen, &dwCount, nullptr) ||
                        !WriteFile(hFile, &offset, sizeof(offset), &dwCount, nullptr) ||
                        !WriteFile(hFile, &length, sizeof(length), &dwCount, nullptr)) {
                        CloseHandle(hTemp);
                        continue;
                    }
                    rowCount++;
                }

                buffer.resize(length);
                SetFilePointer(hTemp, 0, nullptr, FILE_BEGIN);

                if (!ReadFile(hTemp, buffer.data(), length, &dwCount, nullptr) ||
                    !WriteFile(hSource, buffer.data(), length, &dwCount, nullptr)) {
                    CloseHandle(hTemp);
                    continue;
                }

                CloseHandle(hTemp);
                std::filesystem::remove(entry);
            }
        }
    }
    catch (const std::exception&) {
    }

    if (rowCount != rowBefore) {
        bChanged = true;
        if (hCryptProv != NULL) {
            SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
            WriteFile(hFile, &version, sizeof(version), &dwCount, nullptr);
            SetFilePointer(hFile, 4, nullptr, FILE_BEGIN);
            WriteCrypted(hFile, &rowCount, sizeof(rowCount), &dwCount);
        }
        else {
            SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
            WriteFile(hFile, &version, sizeof(version), &dwCount, nullptr);
            WriteFile(hFile, &rowCount, sizeof(rowCount), &dwCount, nullptr);
        }
    }

    CloseHandle(hFile);
    CloseHandle(hSource);
}

bool CHDRSystem::WriteDirectory(const std::string& directoryPath)
{
    namespace fs = std::filesystem;
    try {
        if (fs::exists(directoryPath) && fs::is_directory(directoryPath)) {
            return TRUE;
        }

        bool created = fs::create_directories(directoryPath);
        if (created) {
            fs::path dir(directoryPath);
            while (!dir.empty() && dir != dir.root_path()) {
                SetFileAttributesA(dir.string().c_str(), FILE_ATTRIBUTE_NORMAL);
                dir = dir.parent_path();
            }
        }

        return created;
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

std::string CHDRSystem::GetDirectory(const std::string& path)
{
	size_t found = path.find_last_of("/\\");
	return(path.substr(0, found));
}
