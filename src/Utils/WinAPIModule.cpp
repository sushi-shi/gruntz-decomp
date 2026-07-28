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
        //
        // @early-stop
        // 99.90%: one swapped pair. After the inlined memcpy's `rep movsb` clobbers
        // esi/edi, cl restores the two spilled locals in the order edi<-[esp+0x14]
        // (pModuleNext), esi<-[esp+0x10] (snap); retail restores esi first (the
        // most-recently-spilled slot, and the one the next `push esi / call edi`
        // consumes first). Same slots, same spill sites, same uses - only the two
        // reload instructions are transposed. Tried: declaring `snap` up front with the
        // other locals (slot-neutral, no change) and `found = 1` before the memcpy
        // (no change). Everything else, incl. the four GetProcAddress arms and the
        // rep movsd/movsb split, is byte-exact.
        RVA(0x00118f60, 0x134)
        i32 LegacyFindModule(DWORD th32ProcessID, DWORD moduleID, void* outBuf, DWORD bufSize) {
            i32 found = 0;
            MODULEENTRY32 me32 = {0};

            HMODULE k32 = GetModuleHandleA("KERNEL32.DLL");
            if (!k32) {
                return 0;
            }

            // language-forced: GetProcAddress returns FARPROC
            PFNCREATESNAPSHOT pCreateSnapshot = reinterpret_cast<PFNCREATESNAPSHOT>(
                GetProcAddress(k32, "CreateToolhelp32Snapshot")
            );
            if (!pCreateSnapshot) {
                return 0;
            }

            // language-forced: GetProcAddress returns FARPROC
            PFNMODULEWALK pModuleFirst =
                reinterpret_cast<PFNMODULEWALK>(GetProcAddress(k32, "Module32First"));
            if (!pModuleFirst) {
                return 0;
            }

            // language-forced: GetProcAddress returns FARPROC
            PFNMODULEWALK pModuleNext =
                reinterpret_cast<PFNMODULEWALK>(GetProcAddress(k32, "Module32Next"));
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
