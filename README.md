# PECleaner
 
A lightweight Windows tool that strips identifying metadata from PE binaries (`.exe`, `.dll`, `.sys`) to protect the anonymity of the author.
 
---
 
## Overview
 
When you compile a Windows binary, the compiler and linker embed a significant amount of identifying information directly into the file — build timestamps, PDB paths containing your username, unique build GUIDs, toolchain fingerprints, and more.
 
**PECleaner** removes all of this metadata by directly patching the PE file on disk using memory-mapped I/O.
 
---
 
## What it cleans
 
| Target | Location | Description |
|---|---|---|
| **Timestamp** | `IMAGE_FILE_HEADER` | Compilation date |
| **PDB Path** | Debug Directory (CodeView) | Full path including username (e.g. `C:\Users\john\...`) |
| **Build GUID** | Debug Directory (CodeView) | Unique identifier generated at each compilation |
| **Debug Directory** | `DataDirectory[DEBUG]` | Entire debug directory structure |
| **Rich Header** | Between DOS header and NT headers | XOR-encoded toolchain fingerprint (MSVC version, linker...) |
| **Checksum** | `OptionalHeader.CheckSum` | PE integrity checksum |
| **Resource Section** | `DataDirectory[RESOURCE]` | Version info, company name, original filename, icons... |
| **DOS Stub** | Between `IMAGE_DOS_HEADER` and NT headers | 16-bit code + linker-specific patterns |
| **DOS Header reserved fields** | `IMAGE_DOS_HEADER` | `e_res`, `e_res2`, `e_oemid`, `e_oeminfo` |
| **Version fields** | `IMAGE_OPTIONAL_HEADER` | Linker, image, OS and subsystem version numbers |
| **Export Directory** | `DataDirectory[EXPORT]` | Module name and exported function names |
| **Custom section names** | `IMAGE_SECTION_HEADER` | Non-standard section names that could identify a project |
 
---
 
## Usage
 
```
PECleaner.exe <path to binary>
```
 
```
PECleaner.exe C:\path\to\target.sys
```
 
### Example output
 
```
[+] File mapped at: 0000020F3ACF0000 (Size: 15360 bytes)
[~] Analysing PE file ...
[+] Valid PE format
[+] TimeDateStamp cleared (was: 0x6A63830A)
[+] PDB path found: C:\Users\john\source\repos\MyDriver\build\MyDriver.pdb
[+] GUID found: A53E1E5E40D21A448D4DFFAECEC9FEF7
[+] Debug directory cleared
[+] Rich Header found (key: 0x4F3D2A1B, size: 96 bytes)
[+] Rich Header cleared
[+] Checksum cleared (was: 0xBFEC)
[~] No resource section found
[+] DOS stub found (size: 168 bytes)
[+] DOS stub cleared
[+] Linker version found    : 14.51
[+] Version fields cleared
[+] DOS header reserved fields cleared
[~] No export directory found
[~] No custom section names found
[+] File unmapped and saved.
```
 
---
 
## How it works
 
PECleaner uses Windows memory-mapped I/O (`CreateFileMapping` / `MapViewOfFile`) to patch the binary directly on disk without loading it. Modifications are written back automatically when the file is unmapped.
 
RVA to raw file offset translation is handled manually to correctly navigate the PE structure when the file is flat-mapped rather than loaded by the Windows loader.
 
---
 
## Building
 
Open in Visual Studio and build in Release x64.
 
Requires: Windows SDK, MSVC toolchain.
 
---
 
## Notes
 
- The binary remains **fully functional** after cleaning — only metadata is removed.
- The checksum is zeroed rather than recalculated. This is valid for most binaries — only kernel components and system DLLs strictly require a valid checksum.
---
 
## Disclaimer
 
This tool is intended for developers who want to protect their personal information embedded in binaries they distribute. Use responsibly.
