// HeapDiag.cpp - the engine's heap self-check / walk diagnostic (RVA 0x118a30).
//
// A __cdecl diagnostic that runs the CRT heap consistency check (_heapchk),
// reports the status through the sibling status-reporter (0x118b50, which itself
// wraps OutputDebugStringA), and - when asked and the heap is NOT _HEAPOK - walks
// the heap to the first non-OK block and dumps it. Misfiled into the CRT/library
// backlog originally; it is game code (it calls the engine's own status reporter,
// not a CRT routine). The CRT heap primitives + OutputDebugStringA are external /
// reloc-masked.
#include <Win32.h>  // OutputDebugStringA
#include <malloc.h> // _HEAPINFO / _heapchk / _heapwalk / _HEAPOK / _USEDENTRY
#include <stdio.h>  // sprintf
#include <string.h> // memset

#include <rva.h>

// ---------------------------------------------------------------------------
// Window / file-probe helpers that lead this TU in retail-RVA order (0x118930..
// 0x1189c0, just before HeapCheckDump). Re-homed from the deleted fake grab-bag
// src/Utils/WinAPI.cpp; they interleave with the heap-diag functions in .text
// (WinAPI 0x118930..0x1189c0, heap-diag 0x118a30..0x118bf0, WinAPI 0x118ce0) -> one
// real TU. Plain file-scope free functions (the fake Utils::WinAPI namespace is gone).
// ---------------------------------------------------------------------------

// SetActiveAndFocus (0x118930): activate + focus the same window.
RVA(0x00118930, 0x15)
void SetActiveAndFocus(HWND hWnd) {
    SetActiveWindow(hWnd);
    SetFocus(hWnd);
}

// SetTopmostStyle (0x118960): OR WS_EX_TOPMOST into the window's extended style.
RVA(0x00118960, 0x20)
void SetTopmostStyle(HWND hWnd) {
    LONG s = GetWindowLongA(hWnd, GWL_EXSTYLE);
    if (s) {
        SetWindowLongA(hWnd, GWL_EXSTYLE, s | WS_EX_TOPMOST);
    }
}

// ClearTopmostStyle (0x118990): clear WS_EX_TOPMOST from the extended style.
RVA(0x00118990, 0x20)
void ClearTopmostStyle(HWND hWnd) {
    LONG s = GetWindowLongA(hWnd, GWL_EXSTYLE);
    if (s) {
        SetWindowLongA(hWnd, GWL_EXSTYLE, s & ~WS_EX_TOPMOST);
    }
}

// FileExists (0x1189c0): OpenFile(OF_EXIST) probe; false for a null/empty path. The
// out-of-line COMDAT copy of the inline helper (byte-identical to the WinAPICdRom /
// SoundFontPath copies); CGruntzMgr's movie-path checks call this one.
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

// The engine wrappers retail calls here. The status reporter (0x118b50) takes the
// heap status - its real owner is this diagnostic TU (its sole caller), DEFINED
// below after HeapCheckDump (it follows in retail-RVA order). The heap-walk wrapper
// (0x1206b0, retail's _heapwalk re-attributed) is declared no-arg but invoked with
// &hinfo, so it is reached through a casted pointer to keep its no-arg mangled
// symbol; it stays external / reloc-masked (defined in src/Stub/ApiCallers.cpp).
namespace ApiCallerStubs {
    void winapi_118b50_OutputDebugStringA(i32 status);
    i32 winapi_1206b0_GetLastError_HeapValidate_HeapWalk();
} // namespace ApiCallerStubs
typedef i32(__cdecl* HeapWalkFn)(_HEAPINFO*);

// @early-stop
// 96.7%: body byte-exact. Residual is the shrink-wrapped `push esi` - retail defers
// the esi callee-save into the (conditionally-entered) walk block; cl pushes all 3
// callee-saved regs at the prologue. The 4-byte esp shift cascades through the first
// _HEAPINFO zero-stores + flips two je targets + the epilogue pop order. Not
// source-steerable (docs/patterns/shrink-wrapped-callee-save-push.md).
RVA(0x00118a30, 0xda)
int HeapCheckDump(int walkOnBad) {
    _HEAPINFO hinfo;
    char buf[80];
    int status = _heapchk();
    OutputDebugStringA("Checking heap...\n");
    ApiCallerStubs::winapi_118b50_OutputDebugStringA(status);
    if (walkOnBad == 0 || status == _HEAPOK) {
        return status;
    }
    memset(&hinfo, 0, sizeof(hinfo));
    (reinterpret_cast<HeapWalkFn>(ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk))(&hinfo);
    OutputDebugStringA("Walking heap...\n");
    hinfo._pentry = 0;
    int r = (reinterpret_cast<HeapWalkFn>(ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk))(&hinfo);
    while (r == _HEAPOK) {
        r = (reinterpret_cast<HeapWalkFn>(ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk))(&hinfo);
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
    return status;
}

// 0x118b50: the heap-status reporter HeapCheckDump funnels through - trace a
// _heapchk()/_heapwalk() status code to the debugger. __cdecl(status). Re-homed
// from src/Stub/ApiCallers.cpp (this is its only caller).
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

// 0x118bf0: HeapStats - the engine's heap-statistics probe (sibling of
// HeapCheckDump). Run the CRT heap check, report the status through the sibling
// reporter (0x118b50), then - when the heap is _HEAPOK - walk every block summing
// its total/used/free byte totals and dump them to the debugger. __cdecl, no args,
// returns the _heapchk status. (Retail labels the %lu args "Total/Free/Used" but
// passes total/used/free in that arg order - the swapped labels are reproduced
// verbatim.) The CRT heap primitives + OutputDebugStringA are external/reloc-masked.
RVA(0x00118bf0, 0xb4)
int HeapStats() {
    _HEAPINFO hinfo;
    char buf[80];
    int status = _heapchk();
    OutputDebugStringA("Getting heap statistics...");
    ApiCallerStubs::winapi_118b50_OutputDebugStringA(status);
    unsigned long total = 0, used = 0, free = 0;
    if (status == _HEAPOK) {
        hinfo._pentry = 0;
        hinfo._size = 0;
        hinfo._useflag = 0;
        (reinterpret_cast<HeapWalkFn>(ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk))(&hinfo);
        hinfo._pentry = 0;
        int r =
            (reinterpret_cast<HeapWalkFn>(ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk))(&hinfo);
        while (r == status) {
            total += hinfo._size;
            if (hinfo._useflag == _USEDENTRY) {
                used += hinfo._size;
            } else {
                free += hinfo._size;
            }
            r = (reinterpret_cast<HeapWalkFn>(ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk))(
                &hinfo
            );
        }
    }
    sprintf(buf, "Heap stats: Total = %lu, Free = %lu, Used = %lu", total, used, free);
    OutputDebugStringA(buf);
    return status;
}

// ---------------------------------------------------------------------------
// Toolhelp32 process-scan support (tail of this TU in retail-RVA order, 0x118ce0).
// The Toolhelp APIs are resolved at run time via GetProcAddress (they are Win9x/NT
// version dependent, so never linked directly); the snapshot records are the REAL
// SDK <tlhelp32.h> PROCESSENTRY32 / MODULEENTRY32 (layout-identical to the former
// hand-rolled ProcEntry32/ModEntry32 views - 0x128/0x224 B, asserted below).
// Re-homed from the deleted fake grab-bag src/Utils/WinAPI.cpp.
// ---------------------------------------------------------------------------
#include <tlhelp32.h>
SIZE(PROCESSENTRY32, 0x128);
SIZE(MODULEENTRY32, 0x224);
typedef HANDLE(WINAPI* PFN_CreateSnapshot)(u32 dwFlags, u32 th32ProcessID);
typedef i32(WINAPI* PFN_Process32)(HANDLE hSnapshot, PROCESSENTRY32* pe);

// Fills a MODULEENTRY32 for the main module of the given process (engine helper at
// 0x118f60; Module32First/Next based). Reloc-masked direct call.
extern "C" i32 LegacyFindModule(u32 pid, u32 moduleId, MODULEENTRY32* out, u32 size);

// -------------------------------------------------------------------------
// FindProcessByName
// Scans running processes via a Toolhelp32 snapshot, case-insensitively comparing
// each process's main-module name (its full path when `name` itself contains a
// backslash, else its base name) against `name`. Counts matches; on the first match,
// opens the process (PROCESS_QUERY_INFORMATION) into *pHandleOut. Returns 1 once
// `wantCount` matches are seen, else 0.
// @early-stop
// memset-lowering + scheduling wall (86.1%): logic + all control flow (the two
// duplicated stricmp arms, OpenProcess-on-first, Process32Next loop) byte-exact.
// Residual is MSVC /O2 inline-memset shape on the two Toolhelp snapshot records:
// retail splits the leading dwSize dword out of the `rep stosd` (count N-1 from
// struct+4, dwSize stored via the live zero-reg / DSE'd), and hoists the
// th32ModuleID load out of the me-memset region; our build emits the full-width
// `rep stosd` from struct+0. Not source-steerable. Final-sweep.
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

    PFN_CreateSnapshot pCreate =
        reinterpret_cast<PFN_CreateSnapshot>(GetProcAddress(hK32, "CreateToolhelp32Snapshot"));
    if (pCreate == 0) {
        return 0;
    }
    PFN_Process32 pFirst = reinterpret_cast<PFN_Process32>(GetProcAddress(hK32, "Process32First"));
    if (pFirst == 0) {
        return 0;
    }
    PFN_Process32 pNext = reinterpret_cast<PFN_Process32>(GetProcAddress(hK32, "Process32Next"));
    if (pNext == 0) {
        return 0;
    }

    HANDLE hSnap = pCreate(TH32CS_SNAPPROCESS, 0);
    if (hSnap == reinterpret_cast<HANDLE>(-1)) {
        return 0;
    }

    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    i32 matchCount = 0;
    if (!pFirst(hSnap, &pe)) {
        CloseHandle(hSnap);
        return 0;
    }

    do {
        MODULEENTRY32 me;
        memset(&me, 0, sizeof(me));
        if (LegacyFindModule(pe.th32ProcessID, pe.th32ModuleID, &me, sizeof(me))) {
            if (isFullPath) {
                if (_stricmp(name, me.szExePath) == 0) {
                    matchCount++;
                    if (matchCount == 1 && pHandleOut != 0) {
                        *pHandleOut = OpenProcess(PROCESS_QUERY_INFORMATION, 0, me.th32ProcessID);
                    }
                    if (matchCount >= wantCount) {
                        return 1;
                    }
                }
            } else {
                if (_stricmp(name, me.szModule) == 0) {
                    matchCount++;
                    if (matchCount == 1 && pHandleOut != 0) {
                        *pHandleOut = OpenProcess(PROCESS_QUERY_INFORMATION, 0, me.th32ProcessID);
                    }
                    if (matchCount >= wantCount) {
                        return 1;
                    }
                }
            }
        }
    } while (pNext(hSnap, &pe));

    CloseHandle(hSnap);
    return 0;
}
