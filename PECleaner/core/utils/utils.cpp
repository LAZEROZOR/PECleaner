#include "utils.h"

DWORD utils::rvaToOffset(PIMAGE_NT_HEADERS pNtHeaders, DWORD rva)
{
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);
    WORD numSections = pNtHeaders->FileHeader.NumberOfSections;

    for (WORD i = 0; i < numSections; i++)
    {
        if (rva >= pSection[i].VirtualAddress &&
            rva < pSection[i].VirtualAddress + pSection[i].Misc.VirtualSize)
        {
            return (rva - pSection[i].VirtualAddress) + pSection[i].PointerToRawData;
        }
    }
    return 0;
}

bool utils::openFileAndMap(const char* filePath, MappedFile* outMapped)
{
    outMapped->hFile = CreateFileA(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (outMapped->hFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "[-] Failed to open the file \n";
        return false;
    }

    outMapped->fileSize = GetFileSize(outMapped->hFile, NULL);

    outMapped->hMapping = CreateFileMappingA(outMapped->hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!outMapped->hMapping)
    {
        std::cout << "[-] Failed to create file mapping \n";
        CloseHandle(outMapped->hFile);
        return false;
    }

    outMapped->pBase = MapViewOfFile(outMapped->hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!outMapped->pBase)
    {
        std::cout << "[-] Failed to map view of file \n";
        CloseHandle(outMapped->hMapping);
        CloseHandle(outMapped->hFile);
        return false;
    }

    return true;
}

void utils::closeMappedFile(MappedFile* mapped)
{
    if (mapped->pBase) UnmapViewOfFile(mapped->pBase);
    if (mapped->hMapping) CloseHandle(mapped->hMapping);
    if (mapped->hFile != INVALID_HANDLE_VALUE) CloseHandle(mapped->hFile);
}