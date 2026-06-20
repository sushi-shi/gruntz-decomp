// WinAPIModule.cpp - the Utils::WinAPI ToolHelp32 module-walk helper, in its own
// TU to keep its symbol set out of WinAPI.cpp's already-matched functions
// (entropy follows header churn; see docs/matching-patterns.md).
//
//   LegacyFindModule - walks the ToolHelp32 module list of a process, looking
//                      for a module by its th32ModuleID and copying its
//                      MODULEENTRY32 out. The ToolHelp32 entry points are
//                      resolved dynamically off KERNEL32.DLL (GetProcAddress),
//                      the legacy way that also works on the NT line that lacks
//                      a static ToolHelp32 import.
//
// Field names are placeholders; only the OFFSETS, the dynamic import strings,
// and the code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <string.h>   // inline memcpy (rep movs)

// ---------------------------------------------------------------------------
// Minimal Win32 surface. Do NOT pull in <windows.h>. Only the KERNEL32 imports
// this helper calls directly, as a __declspec(dllimport) __stdcall block (the
// FF15 [IAT] form), keeping the visible symbol set small.
// ---------------------------------------------------------------------------
typedef int            BOOL;
typedef unsigned long  DWORD;
typedef const char *   LPCSTR;
typedef void *         HANDLE;
typedef unsigned char *LPBYTE;
typedef void *         HMODULE;
typedef void *         FARPROC_;

extern "C" {
__declspec(dllimport) HMODULE  __stdcall GetModuleHandleA(LPCSTR lpModuleName);
__declspec(dllimport) FARPROC_ __stdcall GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
__declspec(dllimport) BOOL     __stdcall CloseHandle(HANDLE hObject);
}

// The ToolHelp32 module-walk entry points, resolved at runtime by name.
struct MODULEENTRY32 {
    DWORD  dwSize;          // +0x000
    DWORD  th32ModuleID;    // +0x004
    DWORD  th32ProcessID;   // +0x008
    DWORD  GlblcntUsage;    // +0x00c
    DWORD  ProccntUsage;    // +0x010
    LPBYTE modBaseAddr;     // +0x014
    DWORD  modBaseSize;     // +0x018
    HMODULE hModule;        // +0x01c
    char   szModule[256];   // +0x020
    char   szExePath[260];  // +0x120  -> total 0x224
};

typedef HANDLE (__stdcall *PFNCREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL   (__stdcall *PFNMODULEWALK)(HANDLE hSnapshot, MODULEENTRY32 *lpme);

namespace Utils {
namespace WinAPI {

#define TH32CS_SNAPMODULE 0x00000008

// ---------------------------------------------------------------------------
// LegacyFindModule
// Snapshots the module list of process th32ProcessID, finds the module whose
// th32ModuleID equals moduleID, and copies bufSize bytes of its MODULEENTRY32
// into outBuf. Returns nonzero iff the module was found.
RVA(0x118f60, 0x134)
int LegacyFindModule(DWORD th32ProcessID, DWORD moduleID, void *outBuf, DWORD bufSize)
{
    int found = 0;
    MODULEENTRY32 me32 = { 0 };

    HMODULE k32 = GetModuleHandleA("KERNEL32.DLL");
    if (!k32)
        return 0;

    PFNCREATESNAPSHOT pCreateSnapshot =
        (PFNCREATESNAPSHOT)GetProcAddress(k32, "CreateToolhelp32Snapshot");
    if (!pCreateSnapshot)
        return 0;

    PFNMODULEWALK pModuleFirst = (PFNMODULEWALK)GetProcAddress(k32, "Module32First");
    if (!pModuleFirst)
        return 0;

    PFNMODULEWALK pModuleNext = (PFNMODULEWALK)GetProcAddress(k32, "Module32Next");
    if (!pModuleNext)
        return 0;

    HANDLE snap = pCreateSnapshot(TH32CS_SNAPMODULE, th32ProcessID);
    if (snap == (HANDLE)-1)
        return 0;

    me32.dwSize = sizeof(me32);
    if (pModuleFirst(snap, &me32)) {
        do {
            if (me32.th32ModuleID == moduleID) {
                memcpy(outBuf, &me32, bufSize);
                found = 1;
            }
        } while (!found && pModuleNext(snap, &me32));
    }

    CloseHandle(snap);
    return found;
}

} // namespace WinAPI
} // namespace Utils
