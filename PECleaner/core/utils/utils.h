#pragma once
#include <iostream>
#include <Windows.h>

struct MappedFile {
    HANDLE hFile;
    HANDLE hMapping;
    LPVOID pBase;
    DWORD fileSize;
};

namespace utils
{
	DWORD rvaToOffset(PIMAGE_NT_HEADERS pNtHeaders, DWORD rva);
    bool openFileAndMap(const char* filePath, MappedFile* outMapped);
	void closeMappedFile(MappedFile* mapped);
}
