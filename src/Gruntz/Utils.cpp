#include <rva.h>

#include <Gruntz/Utils.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GruntDirStatics.h>
#include <Utils/FileExists.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>

typedef BOOL(WINAPI* PROCESSWALK)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef HANDLE(WINAPI* CREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL(WINAPI* MODULEWALK)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

RVA(0x00118930, 0x15)
void SetActiveAndFocus(HWND hWnd) {
    SetActiveWindow(hWnd);
    SetFocus(hWnd);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00118960, 0x20)
void SetTopmostStyle(HWND hWnd) {
    LONG s = GetWindowLongA(hWnd, GWL_EXSTYLE);
    if (s) {
        SetWindowLongA(hWnd, GWL_EXSTYLE, s | WS_EX_TOPMOST);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00118990, 0x20)
void ClearTopmostStyle(HWND hWnd) {
    LONG s = GetWindowLongA(hWnd, GWL_EXSTYLE);
    if (s) {
        SetWindowLongA(hWnd, GWL_EXSTYLE, s & ~WS_EX_TOPMOST);
    }
}

RVA_COMPGEN(0x001189c0, 0x45, ?FileExists@@YAHPBD@Z)

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00118a30, 0xda)
int CheckHeap(BOOL bWalkIfErr) {
    int val = _heapchk();

    OutputDebugString("Checking heap...\n");
    OutputHeapReturnValue(val);

    if (bWalkIfErr && val != _HEAPOK) {
        _HEAPINFO hi;
        memset(&hi, 0, sizeof(_HEAPINFO));

        int val = _heapwalk(&hi);
        int heapstatus;

        OutputDebugString("Walking heap...\n");

        hi._pentry = NULL;
        while ((heapstatus = _heapwalk(&hi)) == _HEAPOK) {
        }

        char buf[80];
        sprintf(
            buf,
            "HEAP: %6s block at %Fp of size %4.4X\n",
            (hi._useflag == _USEDENTRY ? "USED" : "FREE"),
            hi._pentry,
            hi._size
        );
        OutputDebugString(buf);

        OutputHeapReturnValue(heapstatus);
        OutputDebugString("Finished walking heap.");
    }

    return (val);
}

RVA(0x00118b50, 0x80)
void OutputHeapReturnValue(int val) {
    switch (val) {
        case _HEAPBADBEGIN: {
            ::OutputDebugString("Heap return value: _HEAPBADBEGIN\n");
            break;
        }

        case _HEAPBADNODE: {
            ::OutputDebugString("Heap return value: _HEAPBADNODE\n");
            break;
        }

        case _HEAPBADPTR: {
            ::OutputDebugString("Heap return value: _HEAPBADPTR\n");
            break;
        }

        case _HEAPEMPTY: {
            ::OutputDebugString("Heap return value: _HEAPEMPTY\n");
            break;
        }

        case _HEAPOK: {
            ::OutputDebugString("Heap return value: _HEAPOK\n");
            break;
        }

        default: {
            ::OutputDebugString("Heap return value: Unknown return value!\n");
            break;
        }
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00118bf0, 0xb4)
int HeapStats() {
    int val = _heapchk();

    OutputDebugString("Getting heap statistics...");
    OutputHeapReturnValue(val);

    DWORD dwTotal = 0;
    DWORD dwUsed = 0;
    DWORD dwFree = 0;

    if (val == _HEAPOK) {
        _HEAPINFO hi;
        memset(&hi, 0, sizeof(_HEAPINFO));

        int val = _heapwalk(&hi);
        int heapstatus;

        hi._pentry = NULL;
        while ((heapstatus = _heapwalk(&hi)) == _HEAPOK) {
            dwTotal += hi._size;

            if (hi._useflag == _USEDENTRY) {
                dwUsed += hi._size;
            } else {
                dwFree += hi._size;
            }
        }
    }

    char buf[128];
    sprintf(buf, "Heap stats: Total = %lu, Free = %lu, Used = %lu\n", dwTotal, dwUsed, dwFree);
    OutputDebugString(buf);

    return (val);
}

RVA(0x00118ce0, 0x1f5)
BOOL ExistProcess(const char* sExe, int thresh, HANDLE* phProcess) {
    if (sExe == NULL) {
        return (FALSE);
    }
    if (sExe[0] == '\0') {
        return (FALSE);
    }

    if (phProcess) {
        *phProcess = NULL;
    }

    BOOL bFullPath = FALSE;

    if (strstr(sExe, "\\")) {
        bFullPath = TRUE;
    }

    HMODULE hKernel = GetModuleHandle("KERNEL32.DLL");
    if (!hKernel) {
        return (FALSE);
    }

    CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
    PROCESSWALK pProcess32First = NULL;
    PROCESSWALK pProcess32Next = NULL;

    // API-forced: GetProcAddress returns FARPROC.
    pCreateToolhelp32Snapshot =
        reinterpret_cast<CREATESNAPSHOT>(GetProcAddress(hKernel, "CreateToolhelp32Snapshot"));
    if (!pCreateToolhelp32Snapshot) {
        return (FALSE);
    }

    // API-forced: GetProcAddress returns FARPROC.
    pProcess32First = reinterpret_cast<PROCESSWALK>(GetProcAddress(hKernel, "Process32First"));
    if (!pProcess32First) {
        return (FALSE);
    }

    // API-forced: GetProcAddress returns FARPROC.
    pProcess32Next = reinterpret_cast<PROCESSWALK>(GetProcAddress(hKernel, "Process32Next"));
    if (!pProcess32Next) {
        return (FALSE);
    }

    HANDLE hProcessSnap = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return (FALSE);
    }

    PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32);

    int count = 0;

    if (pProcess32First(hProcessSnap, &pe32)) {
        MODULEENTRY32 me32 = {0};

        do {
            if (GetProcessModule(
                    pe32.th32ProcessID,
                    pe32.th32ModuleID,
                    &me32,
                    sizeof(MODULEENTRY32)
                )) {
                if (bFullPath) {
                    if (stricmp(me32.szExePath, sExe) == 0) {
                        count++;

                        if (count == 1 && phProcess) {
                            *phProcess =
                                OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, me32.th32ProcessID);
                        }

                        if (count >= thresh) {
                            return (TRUE);
                        }
                    }
                } else {
                    if (stricmp(me32.szModule, sExe) == 0) {
                        count++;

                        if (count == 1 && phProcess) {
                            *phProcess =
                                OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, me32.th32ProcessID);
                        }

                        if (count >= thresh) {
                            return (TRUE);
                        }
                    }
                }
            }
        } while (pProcess32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return (FALSE);
}

RVA(0x00118f60, 0x134)
BOOL GetProcessModule(DWORD dwPID, DWORD dwModuleID, LPMODULEENTRY32 lpMe32, DWORD cbMe32) {
    BOOL bRet = FALSE;
    BOOL bFound = FALSE;
    HANDLE hModuleSnap = NULL;
    MODULEENTRY32 me32 = {0};

    HMODULE hKernel = GetModuleHandle("KERNEL32.DLL");
    if (!hKernel) {
        return (FALSE);
    }

    CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
    MODULEWALK pModule32First = NULL;
    MODULEWALK pModule32Next = NULL;

    // API-forced: GetProcAddress returns FARPROC.
    pCreateToolhelp32Snapshot =
        reinterpret_cast<CREATESNAPSHOT>(GetProcAddress(hKernel, "CreateToolhelp32Snapshot"));
    if (!pCreateToolhelp32Snapshot) {
        return (FALSE);
    }

    // API-forced: GetProcAddress returns FARPROC.
    pModule32First = reinterpret_cast<MODULEWALK>(GetProcAddress(hKernel, "Module32First"));
    if (!pModule32First) {
        return (FALSE);
    }

    // API-forced: GetProcAddress returns FARPROC.
    pModule32Next = reinterpret_cast<MODULEWALK>(GetProcAddress(hKernel, "Module32Next"));
    if (!pModule32Next) {
        return (FALSE);
    }

    hModuleSnap = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if (hModuleSnap == INVALID_HANDLE_VALUE) {
        return (FALSE);
    }

    me32.dwSize = sizeof(MODULEENTRY32);

    if (pModule32First(hModuleSnap, &me32)) {
        do {
            if (me32.th32ModuleID == dwModuleID) {
                CopyMemory(lpMe32, &me32, cbMe32);
                bFound = TRUE;
            }
        } while (!bFound && pModule32Next(hModuleSnap, &me32));

        bRet = bFound;
    } else {
        bRet = FALSE;
    }

    CloseHandle(hModuleSnap);

    return (bRet);
}

RVA(0x001190f0, 0xda)
CString TimeToString(DWORD dwTime) {
    int nHours = dwTime / 3600000;
    dwTime -= nHours * 3600000;

    int nMinutes = dwTime / 60000;
    dwTime -= nMinutes * 60000;

    int nSeconds = dwTime / 1000;

    char buf[64];
    sprintf(buf, "%i:%02i:%02i", nHours, nMinutes, nSeconds);

    CString str(buf);
    return (str);
}

RVA(0x00119210, 0x66)
void DissectTime(DWORD dwTime, int* pHour, int* pMin, int* pSec) {
    *pHour = dwTime / 3600000;
    dwTime -= *pHour * 3600000;

    *pMin = dwTime / 60000;
    dwTime -= *pMin * 60000;

    *pSec = dwTime / 1000;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001192a0, 0x1d)
void TerminateString(char* text, i32 limit) {
    i32 i = 0;
    while (i < limit && *text != 0) {
        text++;
        i++;
    }
    *text = 0;
}

RVA(0x001192d0, 0x39)
BOOL BlockScreenSaver(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SYSCOMMAND) {
        i32 sc = wParam & 0xfff0;
        if (sc == SC_SCREENSAVE || sc == SC_MONITORPOWER) {
            if (!IsIconic(hWnd)) {
                return 1;
            }
        }
    }
    return 0;
}
