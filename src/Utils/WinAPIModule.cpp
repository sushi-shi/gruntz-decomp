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
#include <string.h> // inline memcpy (rep movs)

// The KERNEL32 imports (GetModuleHandleA / GetProcAddress / CloseHandle) + the
// BOOL/DWORD/LPCSTR/HANDLE/LPBYTE/HMODULE/FARPROC types come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). MODULEENTRY32 and
// TH32CS_SNAPMODULE come from <tlhelp32.h> (the load-bearing 0x224 struct layout).
// The ToolHelp32 entry points are resolved at runtime by name (legacy NT-compat),
// so their signatures are declared below as the GetProcAddress cast targets.
#include <Win32.h>
#include <tlhelp32.h>

typedef HANDLE(WINAPI* PFNCREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL(WINAPI* PFNMODULEWALK)(HANDLE hSnapshot, MODULEENTRY32* lpme);

namespace Utils {
    namespace WinAPI {

        // ---------------------------------------------------------------------------
        // LegacyFindModule
        // Snapshots the module list of process th32ProcessID, finds the module whose
        // th32ModuleID equals moduleID, and copies bufSize bytes of its MODULEENTRY32
        // into outBuf. Returns nonzero iff the module was found.
        RVA(0x00118f60, 0x134)
        i32 LegacyFindModule(DWORD th32ProcessID, DWORD moduleID, void* outBuf, DWORD bufSize) {
            i32 found = 0;
            MODULEENTRY32 me32 = {0};

            HMODULE k32 = GetModuleHandleA("KERNEL32.DLL");
            if (!k32) {
                return 0;
            }

            PFNCREATESNAPSHOT pCreateSnapshot =
                reinterpret_cast<PFNCREATESNAPSHOT>(GetProcAddress(k32, "CreateToolhelp32Snapshot"));
            if (!pCreateSnapshot) {
                return 0;
            }

            PFNMODULEWALK pModuleFirst = reinterpret_cast<PFNMODULEWALK>(GetProcAddress(k32, "Module32First"));
            if (!pModuleFirst) {
                return 0;
            }

            PFNMODULEWALK pModuleNext = reinterpret_cast<PFNMODULEWALK>(GetProcAddress(k32, "Module32Next"));
            if (!pModuleNext) {
                return 0;
            }

            HANDLE snap = pCreateSnapshot(TH32CS_SNAPMODULE, th32ProcessID);
            if (snap == reinterpret_cast<HANDLE>(-1)) {
                return 0;
            }

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
