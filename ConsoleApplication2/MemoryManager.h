#pragma once

#include <iostream>
#include <stdlib.h>
#include <string>
#include <Windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <sstream>

#include "ntinfo.h"
#pragma comment( lib, "psapi" )


std::vector<DWORD> threadList(DWORD pid);
DWORD GetThreadStartAddress(HANDLE processHandle, HANDLE hThread);
DWORD dwGetModuleBaseAddress(TCHAR* lpszModuleName, DWORD pID);
int GetProcessId(WCHAR* ProcName);
HANDLE getProcessByExe(WCHAR* ProcName, DWORD& pID);
DWORD getThreadBaseAddress(int threadId, DWORD pID, HANDLE processHandle);
HWND getHWNDByPid(DWORD pID);

DWORD readMemoryPointer(HANDLE processHandle, std::vector<DWORD> path);