#include <rva.h>
#include <string.h> // inline memcpy (rep movs)

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
