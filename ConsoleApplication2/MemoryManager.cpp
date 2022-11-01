#include "MemoryManager.h"

DWORD readMemoryPointer(HANDLE processHandle, std::vector<DWORD> path)
{
    DWORD baseAddress;

    ReadProcessMemory(processHandle, (LPVOID)(path.at(0)), &baseAddress, sizeof(baseAddress), NULL);

    DWORD pointsAddress = baseAddress;
    for (size_t i = 1; i < path.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(pointsAddress + path.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
    }
    pointsAddress += path.at(path.size() - 1);

    return pointsAddress;
}

HANDLE getProcessByExe(WCHAR* ProcName, DWORD& returnPID)
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        return NULL;
    }

    cProcesses = cbNeeded / sizeof(DWORD);

    HANDLE hP;
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    TCHAR* szProcessTitles[10];
    HANDLE processHandles[10] = { 0 };
    DWORD processIDs[10] = { 0 };
    unsigned int processFound = 0;

    HWND hwnd;

    for (i = 0; i < cProcesses; i++)
    {
        if (aProcesses[i] != 0)
        {
            hP = OpenProcess(PROCESS_QUERY_INFORMATION |
                PROCESS_VM_READ,
                FALSE, aProcesses[i]);

            if (NULL != hP)
            {
                HMODULE hMod;
                DWORD cbNeeded;

                if (EnumProcessModules(hP, &hMod, sizeof(hMod),
                    &cbNeeded))
                {
                    GetModuleBaseName(hP, hMod, szProcessName,
                        sizeof(szProcessName) / sizeof(TCHAR));

                    if (_tcscmp(szProcessName, ProcName) == 0)
                    {
                        hwnd = getHWNDByPid(aProcesses[i]);
                        LRESULT tl = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
                        if (tl)
                        {
                            szProcessTitles[processFound] = new TCHAR[++tl];
                            SendMessage(hwnd, WM_GETTEXT, tl, (LPARAM)szProcessTitles[processFound]);
                            processHandles[processFound] = hP;
                            processIDs[processFound] = aProcesses[i];
                            processFound++;
                        }
                    }
                }
            }
        }
    }
    
    DWORD pID = processIDs[0];
    if (processFound > 1)
    {
        unsigned int desired;
        do
        {
            for (i = 0; i < processFound; i++)
            {
                _tprintf(_T("%d. %s\n"), (i+1), szProcessTitles[i]);
            }
            printf(">> ");
            scanf_s("%u", &desired);
        } while (desired < 1 || desired > processFound);
        pID = processIDs[--desired];
    }
    if (returnPID != NULL)
        returnPID = pID;

    HANDLE ret = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    printf("Found handle: %p\n", ret);

    for (i = 0; i < processFound; i++)
    {
        delete szProcessTitles[i];
    }
    return ret;
}

HWND g_HWND;
HWND getHWNDByPid(DWORD pID)
{
    g_HWND = NULL;
    EnumWindows([] (HWND hwnd, LPARAM param) {
        DWORD lpdwProcessId;
        GetWindowThreadProcessId(hwnd, &lpdwProcessId);
        if (lpdwProcessId == param)
        {
            g_HWND = hwnd;
            return FALSE;
        }
        return TRUE;
    }, pID);
    return g_HWND;
}


DWORD getThreadBaseAddress(int threadId, DWORD pID, HANDLE processHandle)
{
    std::vector<DWORD> threads = threadList(pID);
    //printf("Thread id: %X\n", threads.at(threadId));
    HANDLE hThread = OpenThread(PROCESS_ALL_ACCESS, FALSE, threads.at(threadId));
    //printf("Thread handle: %p\n", hThread);
    return GetThreadStartAddress(processHandle, hThread);
}

DWORD dwGetModuleBaseAddress(TCHAR* lpszModuleName, DWORD pID)
{
    DWORD dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
    MODULEENTRY32 ModuleEntry32 = { 0 };
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32))
    {
        do
        {
            if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0)
            {
                dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32));

    }
    CloseHandle(hSnapshot);
    return dwModuleBaseAddress;
}

int GetProcessId(WCHAR* ProcName) {
    PROCESSENTRY32 pe32;
    HANDLE hSnapshot = NULL;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (wcscmp(pe32.szExeFile, ProcName) == 0)
                break;
        } while (Process32Next(hSnapshot, &pe32));
    }

    if (hSnapshot != INVALID_HANDLE_VALUE)
        CloseHandle(hSnapshot);

    return pe32.th32ProcessID;
}

std::vector<DWORD> threadList(DWORD pid) {
    /* solution from http://stackoverflow.com/questions/1206878/enumerating-threads-in-windows */
    std::vector<DWORD> vect = std::vector<DWORD>();
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h == INVALID_HANDLE_VALUE)
        return vect;

    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    if (Thread32First(h, &te)) {
        do {
            if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
                sizeof(te.th32OwnerProcessID)) {


                if (te.th32OwnerProcessID == pid) {
                    //printf("PID: %04d Thread ID: 0x%04x\n", te.th32OwnerProcessID, te.th32ThreadID);
                    vect.push_back(te.th32ThreadID);
                }

            }
            te.dwSize = sizeof(te);
        } while (Thread32Next(h, &te));
    }

    return vect;
}

DWORD GetThreadStartAddress(HANDLE processHandle, HANDLE hThread) {
    /* rewritten from https://github.com/cheat-engine/cheat-engine/blob/master/Cheat%20Engine/CEFuncProc.pas#L3080 */
    DWORD used = 0, ret = 0;
    DWORD stacktop = 0, result = 0;

    MODULEINFO mi = { 0 };

    GetModuleInformation(processHandle, GetModuleHandle(L"kernel32.dll"), &mi, sizeof(mi));
    stacktop = (DWORD)GetThreadStackTopAddress_x86(processHandle, hThread);

    CloseHandle(hThread);

    if (stacktop) {

        DWORD* buf32 = new DWORD[4096];

        if (ReadProcessMemory(processHandle, (LPCVOID)(stacktop - 4096), buf32, 4096, NULL)) {
            for (int i = 4096 / 4 - 1; i >= 0; --i) {
                if (buf32[i] >= (DWORD)mi.lpBaseOfDll && buf32[i] <= (DWORD)mi.lpBaseOfDll + mi.SizeOfImage) {
                    result = stacktop - 4096 + i * 4;
                    break;
                }

            }
        }

        delete buf32;
    }

    return result;
}