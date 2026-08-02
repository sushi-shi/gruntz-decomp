#include <rva.h>

#include <Win32.h>

#include <ProcAddr.h>

#include <string.h>
#include <tlhelp32.h>

typedef HANDLE(WINAPI* PFNCREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL(WINAPI* PFNMODULEWALK)(HANDLE hSnapshot, MODULEENTRY32* lpme);

namespace Utils {
    namespace WinAPI {

        // @early-stop
        RVA(0x00118f60, 0x134)
        i32 LegacyFindModule(DWORD th32ProcessID, DWORD moduleID, void* outBuf, DWORD bufSize) {
            i32 found = 0;
            MODULEENTRY32 me32 = {0};

            HMODULE k32 = GetModuleHandleA("KERNEL32.DLL");
            if (!k32) {
                return 0;
            }

            ProcAddr<PFNCREATESNAPSHOT> snapProc;
            snapProc.m_raw = GetProcAddress(k32, "CreateToolhelp32Snapshot");
            PFNCREATESNAPSHOT pCreateSnapshot = snapProc.m_fn;
            if (!pCreateSnapshot) {
                return 0;
            }

            ProcAddr<PFNMODULEWALK> walkProc;
            walkProc.m_raw = GetProcAddress(k32, "Module32First");
            PFNMODULEWALK pModuleFirst = walkProc.m_fn;
            if (!pModuleFirst) {
                return 0;
            }

            walkProc.m_raw = GetProcAddress(k32, "Module32Next");
            PFNMODULEWALK pModuleNext = walkProc.m_fn;
            if (!pModuleNext) {
                return 0;
            }

            HANDLE snap = pCreateSnapshot(TH32CS_SNAPMODULE, th32ProcessID);
            if (snap == INVALID_HANDLE_VALUE) {
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
