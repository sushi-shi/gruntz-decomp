// LegacyFindModule.cpp - toolhelp32-based module enumeration helper.
// LegacyFindModule @0x118f60 (308 B) - __cdecl free function. Dynamically
// resolves CreateToolhelp32Snapshot / Module32First / Module32Next from
// KERNEL32.DLL via GetModuleHandleA + GetProcAddress (both cached in the
// engine's IAT), snapshots modules for a given process ID, walks the list
// looking for one whose th32ModuleID matches the requested ID. On a match
// copies the MODULEENTRY32 byte-for-byte into the caller's output buffer.
// Returns 1 if found, 0 otherwise.
//
// Plain /O2 /MT (no /GX): scalar leaf, no stack C++ object / EH frame.
#include <string.h>   // memcpy (inline rep movs)
#include "../Wap32/Wap32.h"

typedef unsigned long DWORD;
typedef void *        HANDLE;
typedef int           BOOL;

// MODULEENTRY32 layout matching the Win9x ToolHelp definition the engine
// uses (szModule at +0x20, szExePath at +0x120, total sizeof = 0x224).
typedef struct tagMODULEENTRY32 {
    DWORD   dwSize;
    DWORD   th32ModuleID;
    DWORD   th32ProcessID;
    DWORD   GlblcntUsage;
    DWORD   ProccntUsage;
    DWORD   modBaseAddr;
    DWORD   modBaseSize;
    HANDLE  hModule;
    char    szModule[256];
    char    szExePath[260];
} MODULEENTRY32;

extern "C" {
__declspec(dllimport) HANDLE __stdcall GetModuleHandleA(const char *lpModuleName);
__declspec(dllimport) void   *__stdcall GetProcAddress(HANDLE hModule, const char *lpProcName);
__declspec(dllimport) BOOL    __stdcall CloseHandle(HANDLE hObject);
}

typedef HANDLE (__stdcall *fnCreateSnapshot)(DWORD, DWORD);
typedef BOOL   (__stdcall *fnModuleFirst)(HANDLE, MODULEENTRY32 *);
typedef BOOL   (__stdcall *fnModuleNext)(HANDLE, MODULEENTRY32 *);

// ---------------------------------------------------------------------------
// LegacyFindModule  @0x118f60  (308 B, __cdecl int)
//
// @address: 0x118f60
// @size:    0x134
// ---------------------------------------------------------------------------
int __cdecl LegacyFindModule(DWORD processID, DWORD moduleID,
                             MODULEENTRY32 *pme, int size)
{
    // Cache GetProcAddress as a local function pointer so MSVC5 generates
    // `mov <reg>, [__imp_GetProcAddress]; call <reg>` (register-indirect),
    // matching the target's `mov edi,[IAT]; call edi` pattern.
    typedef void *(__stdcall *GPAFn)(HANDLE, const char *);
    GPAFn pfnGPA = GetProcAddress;

    int found = 0;
    MODULEENTRY32 me;
    HANDLE hSnapshot;
    fnModuleFirst fnFirst;
    fnModuleNext fnNext;
    fnCreateSnapshot fnSnapshot;

    HANDLE hKernel32 = GetModuleHandleA("KERNEL32.DLL");
    if (hKernel32 == 0)
        return 0;

    // Resolve the three ToolHelp entry points via cached GetProcAddress.
    fnSnapshot = (fnCreateSnapshot)pfnGPA(hKernel32, "CreateToolhelp32Snapshot");
    if (fnSnapshot == 0)
        return 0;

    fnFirst = (fnModuleFirst)pfnGPA(hKernel32, "Module32First");
    if (fnFirst == 0)
        return 0;

    fnNext = (fnModuleNext)pfnGPA(hKernel32, "Module32Next");
    if (fnNext == 0)
        return 0;

    hSnapshot = fnSnapshot(0x08 /*TH32CS_SNAPMODULE*/, processID);
    if (hSnapshot == (HANDLE)-1)
        return 0;

    me.dwSize = sizeof(MODULEENTRY32);   // 0x224
    if (fnFirst(hSnapshot, &me) != 0) {
        do {
            if (me.th32ModuleID == moduleID) {
                memcpy(pme, &me, size);
                found = 1;
                break;
            }
        } while (fnNext(hSnapshot, &me) != 0);
    }

    CloseHandle(hSnapshot);
    return found;
}
