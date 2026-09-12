#include "cleaner.h"

bool clearTimeStamp(LPVOID pBase)
{
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew);
    DWORD oldTimestamp = pNtHeaders->FileHeader.TimeDateStamp;

    if (oldTimestamp == 0)
    {
        std::cout << "[~] No time stamp found\n";
        return true;
    }

    pNtHeaders->FileHeader.TimeDateStamp = 0;

    std::cout << "[+] TimeDateStamp cleared (was: 0x" << std::hex << std::uppercase << oldTimestamp << std::dec << ")\n";

    return true;
}

bool clearDebugDirectory(LPVOID pBase)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + pDosHeader->e_lfanew);

    IMAGE_DATA_DIRECTORY debugDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

    if (debugDir.VirtualAddress == 0 || debugDir.Size == 0)
    {
        std::cout << "[~] No debug directory found\n";
        return true;
    }

    DWORD debugOffset = utils::rvaToOffset(pNtHeaders, debugDir.VirtualAddress);
    if (debugOffset == 0)
    {
        std::cout << "[-] Failed to resolve debug directory offset\n";
        return false;
    }

    PIMAGE_DEBUG_DIRECTORY pDebug = (PIMAGE_DEBUG_DIRECTORY)((BYTE*)pBase + debugOffset);
    DWORD count = debugDir.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    for (DWORD i = 0; i < count; i++)
    {
        if (pDebug[i].PointerToRawData != 0 && pDebug[i].SizeOfData != 0)
        {
            BYTE* pRawData = (BYTE*)pBase + pDebug[i].PointerToRawData;

            if (pDebug[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                std::cout << "[+] PDB path found: " << (char*)(pRawData + 24) << "\n";
                std::cout << "[+] GUID found: ";
                for (int j = 0; j < 16; j++) std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)((BYTE*)pRawData + 4)[j];
                std::cout << std::dec << "\n";
            }

            memset(pRawData, 0, pDebug[i].SizeOfData);
        }

        memset(&pDebug[i], 0, sizeof(IMAGE_DEBUG_DIRECTORY));
    }

    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = 0;
    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size = 0;
    std::cout << "[+] Debug directory cleared\n";

    return true;
}

bool clearRichHeader(LPVOID pBase)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;

    BYTE* pSearch = (BYTE*)pBase + sizeof(IMAGE_DOS_HEADER);
    BYTE* pNtStart = (BYTE*)pBase + pDosHeader->e_lfanew;
    DWORD* pRich = nullptr;
    for (BYTE* p = pSearch; p <= pNtStart - 8; p += 4)
    {
        if (*(DWORD*)p == 0x68636952)
        {
            pRich = (DWORD*)p;
            break;
        }
    }

    if (!pRich)
    {
        std::cout << "[~] No Rich Header found\n";
        return true;
    }

    DWORD xorKey = *(pRich + 1);
    DWORD* pDanS = nullptr;
    for (DWORD* p = (DWORD*)pSearch; p < pRich; p++)
    {
        if ((*p ^ xorKey) == 0x536E6144)
        {
            pDanS = p;
            break;
        }
    }

    if (!pDanS)
    {
        std::cout << "[-] Failed to find DanS marker\n";
        return false;
    }

    BYTE* clearStart = (BYTE*)pDanS;
    BYTE* clearEnd = (BYTE*)(pRich + 2);
    DWORD size = (DWORD)(clearEnd - clearStart);

    std::cout << "[+] Rich Header found (key: 0x" << std::hex << std::uppercase << xorKey << std::dec << ", size: " << size << " bytes)\n";

    memset(clearStart, 0, size);

    std::cout << "[+] Rich Header cleared\n";
    return true;
}

bool clearChecksum(LPVOID pBase)
{
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew);

    DWORD oldChecksum = pNtHeaders->OptionalHeader.CheckSum;

    if (oldChecksum == 0)
    {
        std::cout << "[~] Checksum already null\n";
        return true;
    }

    pNtHeaders->OptionalHeader.CheckSum = 0;

    std::cout << "[+] Checksum cleared (was: 0x" << std::hex << std::uppercase << oldChecksum << std::dec << ")\n";

    return true;
}

bool clearResourceSection(LPVOID pBase)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + pDosHeader->e_lfanew);

    IMAGE_DATA_DIRECTORY rsrcDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

    if (rsrcDir.VirtualAddress == 0 || rsrcDir.Size == 0)
    {
        std::cout << "[~] No resource section found\n";
        return true;
    }

    DWORD rsrcOffset = utils::rvaToOffset(pNtHeaders, rsrcDir.VirtualAddress);
    if (rsrcOffset == 0)
    {
        std::cout << "[-] Failed to resolve resource section offset\n";
        return false;
    }

    std::cout << "[+] Resource section found (size: " << rsrcDir.Size << " bytes)\n";
    memset((BYTE*)pBase + rsrcOffset, 0, rsrcDir.Size);

    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = 0;
    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = 0;

    std::cout << "[+] Resource section cleared\n";
    return true;
}

bool clearDosStub(LPVOID pBase)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;

    BYTE* pStubStart = (BYTE*)pBase + sizeof(IMAGE_DOS_HEADER);
    BYTE* pStubEnd = (BYTE*)pBase + pDosHeader->e_lfanew;
    DWORD size = (DWORD)(pStubEnd - pStubStart);

    if (size == 0)
    {
        std::cout << "[~] No DOS stub found\n";
        return true;
    }

    std::cout << "[+] DOS stub found (size: " << size << " bytes)\n";
    memset(pStubStart, 0, size);
    std::cout << "[+] DOS stub cleared\n";

    return true;
}

bool clearDosHeader(LPVOID pBase)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;

    if (pDosHeader->e_oemid == 0 && pDosHeader->e_oeminfo == 0 &&
        pDosHeader->e_res[0] == 0 && pDosHeader->e_res2[0] == 0)
    {
        std::cout << "[~] DOS header reserved fields already null\n";
        return true;
    }

    memset(pDosHeader->e_res, 0, sizeof(pDosHeader->e_res));
    memset(pDosHeader->e_res2, 0, sizeof(pDosHeader->e_res2));
    pDosHeader->e_oemid = 0;
    pDosHeader->e_oeminfo = 0;

    std::cout << "[+] DOS header reserved fields cleared\n";
    return true;
}

bool clearVersionFields(LPVOID pBase)
{
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew);
    PIMAGE_OPTIONAL_HEADER pOpt = &pNtHeaders->OptionalHeader;

    if (pOpt->MajorLinkerVersion == 0 && pOpt->MinorLinkerVersion == 0 &&
        pOpt->MajorImageVersion == 0 && pOpt->MinorImageVersion == 0 &&
        pOpt->MajorOperatingSystemVersion == 0 && pOpt->MinorOperatingSystemVersion == 0 &&
        pOpt->MajorSubsystemVersion == 0 && pOpt->MinorSubsystemVersion == 0)
    {
        std::cout << "[~] Version fields already null\n";
        return true;
    }

    std::cout << "[+] Linker version found    : " << (int)pOpt->MajorLinkerVersion << "." << (int)pOpt->MinorLinkerVersion << "\n";
    std::cout << "[+] Image version found     : " << pOpt->MajorImageVersion << "." << pOpt->MinorImageVersion << "\n";
    std::cout << "[+] OS version found        : " << pOpt->MajorOperatingSystemVersion << "." << pOpt->MinorOperatingSystemVersion << "\n";
    std::cout << "[+] Subsystem version found : " << pOpt->MajorSubsystemVersion << "." << pOpt->MinorSubsystemVersion << "\n";

    pOpt->MajorLinkerVersion = 0;
    pOpt->MinorLinkerVersion = 0;
    pOpt->MajorImageVersion = 0;
    pOpt->MinorImageVersion = 0;
    pOpt->MajorOperatingSystemVersion = 0;
    pOpt->MinorOperatingSystemVersion = 0;
    pOpt->MajorSubsystemVersion = 0;
    pOpt->MinorSubsystemVersion = 0;

    std::cout << "[+] Version fields cleared\n";
    return true;
}

bool clearExportDirectory(LPVOID pBase)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + pDosHeader->e_lfanew);

    IMAGE_DATA_DIRECTORY exportDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (exportDir.VirtualAddress == 0 || exportDir.Size == 0)
    {
        std::cout << "[~] No export directory found\n";
        return true;
    }

    DWORD exportOffset = utils::rvaToOffset(pNtHeaders, exportDir.VirtualAddress);
    if (exportOffset == 0)
    {
        std::cout << "[-] Failed to resolve export directory offset\n";
        return false;
    }

    PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)pBase + exportOffset);

    if (pExport->Name != 0)
    {
        DWORD nameOffset = utils::rvaToOffset(pNtHeaders, pExport->Name);
        if (nameOffset != 0)
            std::cout << "[+] Export module name found: " << (char*)((BYTE*)pBase + nameOffset) << "\n";
    }

    memset((BYTE*)pBase + exportOffset, 0, exportDir.Size);

    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = 0;
    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = 0;

    std::cout << "[+] Export directory cleared\n";
    return true;
}

bool clearSectionNames(LPVOID pBase)
{
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew);
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);
    WORD numSections = pNtHeaders->FileHeader.NumberOfSections;

    bool foundCustom = false;

    for (WORD i = 0; i < numSections; i++)
    {
        char name[9] = { 0 };
        memcpy(name, pSection[i].Name, 8);

        if (strcmp(name, ".text") == 0 || strcmp(name, ".data") == 0 ||
            strcmp(name, ".rdata") == 0 || strcmp(name, ".bss") == 0 ||
            strcmp(name, ".edata") == 0 || strcmp(name, ".idata") == 0 ||
            strcmp(name, ".pdata") == 0 || strcmp(name, ".rsrc") == 0 ||
            strcmp(name, ".reloc") == 0 || strcmp(name, "INIT") == 0 ||
            strcmp(name, "PAGE") == 0) continue;

        std::cout << "[+] Custom section name found: " << name << " → cleared\n";
        memset(pSection[i].Name, 0, 8);
        foundCustom = true;
    }

    if (!foundCustom)
        std::cout << "[~] No custom section names found\n";

    return true;
}

bool cleaner::cleanPE(LPVOID pBase)
{
    std::cout << "[~] Analysing PE file ... \n";

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cout << "[-] Invalid PE \n";
        return false;
    }

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + pDosHeader->e_lfanew);

    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        std::cout << "[-] Invalid PE \n";
        return false;
    }

    std::cout << "[+] Valid PE format \n";

    if (!clearTimeStamp(pBase))        std::cout << "[-] Failed to clear the timestamp\n";
    if (!clearDebugDirectory(pBase))   std::cout << "[-] Failed to clear the debug directory\n";
    if (!clearRichHeader(pBase))       std::cout << "[-] Failed to clear the rich header\n";
    if (!clearChecksum(pBase))         std::cout << "[-] Failed to clear the checksum\n";
    if (!clearResourceSection(pBase))  std::cout << "[-] Failed to clear the resource section\n";
    if (!clearDosStub(pBase))          std::cout << "[-] Failed to clear the DOS stub\n";
    if (!clearVersionFields(pBase))    std::cout << "[-] Failed to clear the version fields\n";
    if (!clearDosHeader(pBase))        std::cout << "[-] Failed to clear the DOS header\n";
    if (!clearExportDirectory(pBase))  std::cout << "[-] Failed to clear the export directory\n";
    if (!clearSectionNames(pBase))     std::cout << "[-] Failed to clear the section names\n";

    return true;
}