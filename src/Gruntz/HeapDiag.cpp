#include <rva.h>

#include <Gruntz/HeapDiag.h>

#include <Win32.h>

#include <Enums.h>
#include <Gruntz/GruntDirStatics.h>
#include <ProcAddr.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>

DATA(0x002c3fe8)
i32 g_val_2c3fe8;
DATA(0x002c3ff0)
i32 g_val_2c3ff0;

RVA(0x00118930, 0x15)
void SetActiveAndFocus(HWND hWnd) {
    SetActiveWindow(hWnd);
    SetFocus(hWnd);
}

RVA(0x00118960, 0x20)
void SetTopmostStyle(HWND hWnd) {
    LONG s = GetWindowLongA(hWnd, GWL_EXSTYLE);
    if (s) {
        SetWindowLongA(hWnd, GWL_EXSTYLE, s | WS_EX_TOPMOST);
    }
}

RVA(0x00118990, 0x20)
void ClearTopmostStyle(HWND hWnd) {
    LONG s = GetWindowLongA(hWnd, GWL_EXSTYLE);
    if (s) {
        SetWindowLongA(hWnd, GWL_EXSTYLE, s & ~WS_EX_TOPMOST);
    }
}

RVA(0x001189c0, 0x45)
i32 FileExists(const char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000) != -1;
}

RVA(0x00118a30, 0xda)
int HeapCheckDump(int walkOnBad) {
    _HEAPINFO hinfo;
    char buf[80];
    int status = _heapchk();
    OutputDebugStringA("Checking heap...\n");
    ReportHeapStatus(status);
    if (walkOnBad != 0 && status != _HEAPOK) {
        memset(&hinfo, 0, sizeof(hinfo));
        _heapwalk(&hinfo);
        OutputDebugStringA("Walking heap...\n");
        hinfo._pentry = NULL;
        int r = _heapwalk(&hinfo);
        while (r == _HEAPOK) {
            r = _heapwalk(&hinfo);
        }
        sprintf(
            buf,
            "HEAP: %6s block at %Fp of size %4.4X\n",
            hinfo._useflag == _USEDENTRY ? "USED" : "FREE",
            hinfo._pentry,
            hinfo._size
        );
        OutputDebugStringA(buf);
        ReportHeapStatus(r);
        OutputDebugStringA("Finished walking heap.");
    }
    return status;
}

RVA(0x00118b50, 0x80)
void ReportHeapStatus(i32 status) {
    switch (status) {
        case _HEAPBADBEGIN:
            OutputDebugStringA("Heap return value: _HEAPBADBEGIN\n");
            return;
        case _HEAPBADNODE:
            OutputDebugStringA("Heap return value: _HEAPBADNODE\n");
            return;
        case _HEAPBADPTR:
            OutputDebugStringA("Heap return value: _HEAPBADPTR\n");
            return;
        case _HEAPEMPTY:
            OutputDebugStringA("Heap return value: _HEAPEMPTY\n");
            return;
        case _HEAPOK:
            OutputDebugStringA("Heap return value: _HEAPOK\n");
            return;
        default:
            OutputDebugStringA("Heap return value: Unknown return value!\n");
            return;
    }
}

RVA(0x00118bf0, 0xb4)
int HeapStats() {
    _HEAPINFO hinfo;
    char buf[128];
    int status = _heapchk();
    OutputDebugStringA("Getting heap statistics...");
    ReportHeapStatus(status);
    unsigned long total = 0, used = 0, free = 0;
    if (status == _HEAPOK) {
        memset(&hinfo, 0, sizeof(hinfo));
        _heapwalk(&hinfo);
        hinfo._pentry = NULL;
        int r = _heapwalk(&hinfo);
        while (r == status) {
            total += hinfo._size;
            if (hinfo._useflag == _USEDENTRY) {
                used += hinfo._size;
            } else {
                free += hinfo._size;
            }
            r = _heapwalk(&hinfo);
        }
    }
    sprintf(buf, "Heap stats: Total = %lu, Free = %lu, Used = %lu", total, used, free);
    OutputDebugStringA(buf);
    return status;
}

typedef HANDLE(WINAPI* PFN_CreateSnapshot)(u32 dwFlags, u32 th32ProcessID);
typedef i32(WINAPI* PFN_Process32)(HANDLE hSnapshot, PROCESSENTRY32* pe);

// @early-stop
RVA(0x00118ce0, 0x1f5)
i32 FindProcessByName(const char* name, i32 wantCount, HANDLE* pHandleOut) {
    if (name == NULL || *name == 0) {
        return 0;
    }
    if (pHandleOut != NULL) {
        *pHandleOut = NULL;
    }

    i32 isFullPath = 0;
    if (strstr(name, "\\") != NULL) {
        isFullPath = 1;
    }

    HMODULE hK32 = GetModuleHandleA("KERNEL32.DLL");
    if (hK32 == NULL) {
        return 0;
    }

    ProcAddr<PFN_CreateSnapshot> snapProc;
    snapProc.m_raw = GetProcAddress(hK32, "CreateToolhelp32Snapshot");
    PFN_CreateSnapshot pCreate = snapProc.m_fn;
    if (pCreate == NULL) {
        return 0;
    }
    ProcAddr<PFN_Process32> walkProc;
    walkProc.m_raw = GetProcAddress(hK32, "Process32First");
    PFN_Process32 pFirst = walkProc.m_fn;
    if (pFirst == NULL) {
        return 0;
    }
    walkProc.m_raw = GetProcAddress(hK32, "Process32Next");
    PFN_Process32 pNext = walkProc.m_fn;
    if (pNext == NULL) {
        return 0;
    }

    HANDLE hSnap = pCreate(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    i32 matchCount = 0;

    if (pFirst(hSnap, &pe)) {
        do {
            MODULEENTRY32 me;
            memset(&me, 0, sizeof(me));
            if (Utils::WinAPI::LegacyFindModule(
                    pe.th32ProcessID,
                    pe.th32ModuleID,
                    &me,
                    sizeof(me)
                )) {
                if (isFullPath) {
                    if (_stricmp(name, me.szExePath) == 0) {
                        matchCount++;
                        if (matchCount == 1 && pHandleOut != NULL) {
                            *pHandleOut =
                                OpenProcess(PROCESS_QUERY_INFORMATION, 0, me.th32ProcessID);
                        }
                        if (matchCount >= wantCount) {
                            return 1;
                        }
                    }
                } else {
                    if (_stricmp(name, me.szModule) == 0) {
                        matchCount++;
                        if (matchCount == 1 && pHandleOut != NULL) {
                            *pHandleOut =
                                OpenProcess(PROCESS_QUERY_INFORMATION, 0, me.th32ProcessID);
                        }
                        if (matchCount >= wantCount) {
                            return 1;
                        }
                    }
                }
            }
        } while (pNext(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return 0;
}
