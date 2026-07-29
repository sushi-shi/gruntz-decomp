#include <Gruntz/HeapDiag.h> // own extern surface
#include <Win32.h>           // OutputDebugStringA
#include <malloc.h>          // _HEAPINFO / _heapchk / _heapwalk / _HEAPOK / _USEDENTRY
#include <stdio.h>           // sprintf
#include <string.h>          // memset

#include <rva.h>
#include <tlhelp32.h>
#include <ProcAddr.h> // the FARPROC / real-prototype pair

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
i32 FileExists(char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
}

namespace ApiCallerStubs {} // namespace ApiCallerStubs

// The walk gate is POSITIVE-form so cl shrink-wraps `push esi` into the
// conditionally-entered walk block (retail saves only ebx/edi at entry) -
// docs/patterns/positive-gate-enables-shrink-wrap.md.
RVA(0x00118a30, 0xda)
int HeapCheckDump(int walkOnBad) {
    _HEAPINFO hinfo;
    char buf[80];
    int status = _heapchk();
    OutputDebugStringA("Checking heap...\n");
    ApiCallerStubs::winapi_118b50_OutputDebugStringA(status);
    if (walkOnBad != 0 && status != _HEAPOK) {
        memset(&hinfo, 0, sizeof(hinfo));
        _heapwalk(&hinfo);
        OutputDebugStringA("Walking heap...\n");
        hinfo._pentry = 0;
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
        ApiCallerStubs::winapi_118b50_OutputDebugStringA(r);
        OutputDebugStringA("Finished walking heap.");
    }
    return status;
}

namespace ApiCallerStubs {
    RVA(0x00118b50, 0x5b)
    void winapi_118b50_OutputDebugStringA(i32 status) {
        switch (status) {
            case -3:
                OutputDebugStringA("Heap return value: _HEAPBADBEGIN\n");
                return;
            case -4:
                OutputDebugStringA("Heap return value: _HEAPBADNODE\n");
                return;
            case -6:
                OutputDebugStringA("Heap return value: _HEAPBADPTR\n");
                return;
            case -1:
                OutputDebugStringA("Heap return value: _HEAPEMPTY\n");
                return;
            case -2:
                OutputDebugStringA("Heap return value: _HEAPOK\n");
                return;
            default:
                OutputDebugStringA("Heap return value: Unknown return value!\n");
                return;
        }
    }
} // namespace ApiCallerStubs

RVA(0x00118bf0, 0xb4)
int HeapStats() {
    _HEAPINFO hinfo;
    char buf[128];
    int status = _heapchk();
    OutputDebugStringA("Getting heap statistics...");
    ApiCallerStubs::winapi_118b50_OutputDebugStringA(status);
    unsigned long total = 0, used = 0, free = 0;
    if (status == _HEAPOK) {
        hinfo._pentry = 0;
        hinfo._size = 0;
        hinfo._useflag = 0;
        _heapwalk(&hinfo);
        hinfo._pentry = 0;
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

// -------------------------------------------------------------------------
// FindProcessByName
// Scans running processes via a Toolhelp32 snapshot, case-insensitively comparing
// each process's main-module name (its full path when `name` itself contains a
// backslash, else its base name) against `name`. Counts matches; on the first match,
// opens the process (PROCESS_QUERY_INFORMATION) into *pHandleOut. Returns 1 once
// `wantCount` matches are seen, else 0.
// @early-stop
// memset-lowering wall. The exit-block-layout half is FIXED (measured 2026-07-27,
// 86.14 -> 93.54): the ret-count screen read base 3 / retail 2, and the extra one was
// the Process32First failure carrying its own `CloseHandle; return 0` epilogue where
// retail 0x11914d branches it into the loop's shared CloseHandle tail. Wrapping the
// walk in `if (pFirst(...)) { ... }` over a single trailing CloseHandle+return-0
// reproduced retail's block layout.
// Residual is the MSVC /O2 inline-memset shape on the two Toolhelp snapshot records:
// retail splits the leading dwSize dword out of the `rep stosd` (count 0x49 from
// struct+4, dwSize stored separately / from the live zero-reg), our build emits the
// full-width `rep stosd` (0x4a) from struct+0. Not exit layout; not source-steerable
// from `memset(&pe,0,sizeof pe); pe.dwSize = sizeof pe;`. Final-sweep.
RVA(0x00118ce0, 0x1f5)
i32 FindProcessByName(const char* name, i32 wantCount, HANDLE* pHandleOut) {
    if (name == 0 || *name == 0) {
        return 0;
    }
    if (pHandleOut != 0) {
        *pHandleOut = 0;
    }

    i32 isFullPath = 0;
    if (strstr(name, "\\") != 0) {
        isFullPath = 1;
    }

    HMODULE hK32 = GetModuleHandleA("KERNEL32.DLL");
    if (hK32 == 0) {
        return 0;
    }

    // GetProcAddress hands the address back as a FARPROC; the caller knows the
    // export's prototype (<ProcAddr.h>)
    ProcAddr<PFN_CreateSnapshot> snapProc;
    snapProc.m_raw = GetProcAddress(hK32, "CreateToolhelp32Snapshot");
    PFN_CreateSnapshot pCreate = snapProc.m_fn;
    if (pCreate == 0) {
        return 0;
    }
    ProcAddr<PFN_Process32> walkProc;
    walkProc.m_raw = GetProcAddress(hK32, "Process32First");
    PFN_Process32 pFirst = walkProc.m_fn;
    if (pFirst == 0) {
        return 0;
    }
    walkProc.m_raw = GetProcAddress(hK32, "Process32Next");
    PFN_Process32 pNext = walkProc.m_fn;
    if (pNext == 0) {
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
    // retail 0x11914d: ONE CloseHandle + return-0 tail - the Process32First failure
    // branches straight into it with hSnap still live, it is not its own exit
    if (pFirst(hSnap, &pe)) {
        do {
            MODULEENTRY32 me;
            memset(&me, 0, sizeof(me));
            if (LegacyFindModule(pe.th32ProcessID, pe.th32ModuleID, &me, sizeof(me))) {
                if (isFullPath) {
                    if (_stricmp(name, me.szExePath) == 0) {
                        matchCount++;
                        if (matchCount == 1 && pHandleOut != 0) {
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
                        if (matchCount == 1 && pHandleOut != 0) {
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
