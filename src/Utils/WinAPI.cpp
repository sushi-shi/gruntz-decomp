// WinAPI.cpp - Utils::WinAPI free-function helpers (a thin layer over a handful
// of KERNEL32 / WINMM imports + the registry/config wrapper). These are static
// free functions in namespace Utils::WinAPI; only offsets + code bytes are
// load-bearing.
//
// <Mfc.h> brings <windows.h> KERNEL32 (OpenFile / GetDriveTypeA; UINT / HFILE /
// OFSTRUCT) and the central WINMM timeGetTime decl (the busy-wait clock).
#include <Mfc.h>
#include <Utils/RegistryHelper.h>
#include <rva.h>
#include <stdarg.h> // va_list for the DebugTrace varargs formatter
#include <string.h>
#include <stdio.h>

namespace Utils {
    namespace WinAPI {

        char GetGruntzDriveLetter();

        // -------------------------------------------------------------------------
        // SetActiveAndFocus (0x118930): activate + focus the same window.
        // (Re-homed from ApiMiscHelpers.)
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

        // -------------------------------------------------------------------------
        // FileExists
        // Tests a path via OpenFile(OF_EXIST). Returns false for a null/empty path.
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

        // -------------------------------------------------------------------------
        // FileExists COMDAT sibling copies. The inline FileExists helper is folded
        // by MSVC into other TUs, emitting byte-identical copies at their own RVAs
        // (re-homed from src/Stub/Backlog.cpp, which held these as Stub_0f90f0 /
        // Stub_01fd70). Same bytes as FileExists above.
        RVA(0x000f90f0, 0x45)
        i32 FileExistsCopyF90F0(char* szPath) {
            OFSTRUCT of;

            if (!szPath) {
                return 0;
            }
            if (!*szPath) {
                return 0;
            }
            return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
        }
        RVA(0x0001fd70, 0x45)
        i32 FileExistsCopy1FD70(char* szPath) {
            OFSTRUCT of;

            if (!szPath) {
                return 0;
            }
            if (!*szPath) {
                return 0;
            }
            return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
        }

        // -------------------------------------------------------------------------
        // ActiveWait
        // Busy-waits ~milliseconds using timeGetTime (no Sleep; spins).
        RVA(0x0013dfe0, 0x21)
        void ActiveWait(u32 milliseconds) {
            DWORD target = timeGetTime() + milliseconds;
            while (timeGetTime() < target)
                ;
        }

        // -------------------------------------------------------------------------
        // DebugTrace (0x13e010): printf-style formatter into a 256-byte stack buffer,
        // emitted via OutputDebugStringA. __cdecl varargs. Orphan copy (inlined at all
        // call sites).
        RVA(0x0013e010, 0x32)
        void DebugTrace(const char* fmt, ...) {
            char buf[256];
            va_list ap;
            va_start(ap, fmt);
            vsprintf(buf, fmt, ap);
            va_end(ap);
            OutputDebugStringA(buf);
        }

        // -------------------------------------------------------------------------
        // IsGruntzCDInAnyDrive
        // True iff a CD drive holding the Gruntz disc was found.
        RVA(0x0001fd50, 0xf)
        i32 IsGruntzCDInAnyDrive() {
            char letter = GetGruntzDriveLetter();
            return letter != 0;
        }

        // File-scope cache of the discovered Gruntz CD drive letter. Zero = not yet
        // probed / not found. Once a drive is found the letter is memoised so later
        // calls return it without re-scanning.
        static char s_cdDriveLetter;

        // -------------------------------------------------------------------------
        // GetGruntzDriveLetter
        // Finds the drive letter of the CD holding the Gruntz disc, memoising the
        // result in s_cdDriveLetter:
        //   1. read the saved drive letter from HKLM\Software\Monolith Productions\
        //      Gruntz\1.0 value "CdRom Drive"; if it is a real letter, a CD-ROM, and
        //      holds "<L>:\GAME\GRUNTZ.EXE", use it;
        //   2. otherwise scan drives A..Z for a CD-ROM holding that marker file.
        // Returns the letter (0 if none). The local RegistryHelper's destructor
        // (Close) runs at scope exit -> the C++ EH frame.
        RVA(0x0001ffe0, 0x192)
        char GetGruntzDriveLetter() {
            if (s_cdDriveLetter == 0) {
                u32 valueSize;
                char value[32];
                char drivePath[32];
                char exePath[256];
                Utils::RegistryHelper reg;
                char drivePathScan[256];
                char letter;

                if (reg.Open(
                        "Monolith Productions",
                        "Gruntz",
                        "1.0",
                        0,
                        (HKEY)0x80000002 /*HKEY_LOCAL_MACHINE*/,
                        0
                    )) {
                    valueSize = 0x1e;
                    value[0] = 0;
                    if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
                        && (i8)value[0] > 0x14) {
                        letter = value[0];
                        sprintf(drivePath, "%c:\\", letter);
                        if (GetDriveTypeA(drivePath) == 5 /*DRIVE_CDROM*/) {
                            sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                            if (FileExists(exePath)) {
                                goto found;
                            }
                        }
                    }
                }

                for (letter = 'A'; letter <= 'Z'; letter++) {
                    sprintf(drivePathScan, "%c:\\", letter);
                    if (GetDriveTypeA(drivePathScan) == 5 /*DRIVE_CDROM*/) {
                        sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                        if (FileExists(exePath)) {
                            goto found;
                        }
                    }
                }
                return 0;

            found:
                s_cdDriveLetter = letter;
                return letter;
            }
            return s_cdDriveLetter;
        }

        // -------------------------------------------------------------------------
        // Toolhelp32 process-scan support. The Toolhelp APIs are resolved at run
        // time via GetProcAddress (they are Win9x/NT-version dependent, so never
        // linked directly); the fixed-layout snapshot records are declared here.
        // -------------------------------------------------------------------------
        struct ProcEntry32 {         // PROCESSENTRY32 (0x128 bytes)
            u32 dwSize;              // +0x00
            u32 cntUsage;            // +0x04
            u32 th32ProcessID;       // +0x08
            u32 th32DefaultHeapID;   // +0x0c
            u32 th32ModuleID;        // +0x10
            u32 cntThreads;          // +0x14
            u32 th32ParentProcessID; // +0x18
            i32 pcPriClassBase;      // +0x1c
            u32 dwFlags;             // +0x20
            char szExeFile[260];     // +0x24
        };
        struct ModEntry32 {      // MODULEENTRY32 (0x224 bytes)
            u32 dwSize;          // +0x00
            u32 th32ModuleID;    // +0x04
            u32 th32ProcessID;   // +0x08
            u32 GlblcntUsage;    // +0x0c
            u32 ProccntUsage;    // +0x10
            u8* modBaseAddr;     // +0x14
            u32 modBaseSize;     // +0x18
            void* hModule;       // +0x1c
            char szModule[256];  // +0x20
            char szExePath[260]; // +0x120
        };
        typedef HANDLE(WINAPI* PFN_CreateSnapshot)(u32 dwFlags, u32 th32ProcessID);
        typedef i32(WINAPI* PFN_Process32)(HANDLE hSnapshot, ProcEntry32* pe);

        // Fills a MODULEENTRY32 for the main module of the given process (engine
        // helper at 0x118f60; Module32First/Next based). Reloc-masked direct call.
        extern "C" i32 LegacyFindModule(u32 pid, u32 moduleId, ModEntry32* out, u32 size);

        // -------------------------------------------------------------------------
        // FindProcessByName
        // Scans running processes via a Toolhelp32 snapshot, case-insensitively
        // comparing each process's main-module name (its full path when `name`
        // itself contains a backslash, else its base name) against `name`. Counts
        // matches; on the first match, opens the process (PROCESS_QUERY_INFORMATION)
        // into *pHandleOut. Returns 1 once `wantCount` matches are seen, else 0.
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
                (PFN_CreateSnapshot)GetProcAddress(hK32, "CreateToolhelp32Snapshot");
            if (pCreate == 0) {
                return 0;
            }
            PFN_Process32 pFirst = (PFN_Process32)GetProcAddress(hK32, "Process32First");
            if (pFirst == 0) {
                return 0;
            }
            PFN_Process32 pNext = (PFN_Process32)GetProcAddress(hK32, "Process32Next");
            if (pNext == 0) {
                return 0;
            }

            HANDLE hSnap = pCreate(2 /*TH32CS_SNAPPROCESS*/, 0);
            if (hSnap == (HANDLE)-1) {
                return 0;
            }

            ProcEntry32 pe;
            memset(&pe, 0, sizeof(pe));
            pe.dwSize = sizeof(pe);
            i32 matchCount = 0;
            if (!pFirst(hSnap, &pe)) {
                CloseHandle(hSnap);
                return 0;
            }

            do {
                ModEntry32 me;
                memset(&me, 0, sizeof(me));
                if (LegacyFindModule(pe.th32ProcessID, pe.th32ModuleID, &me, sizeof(me))) {
                    if (isFullPath) {
                        if (_stricmp(name, me.szExePath) == 0) {
                            matchCount++;
                            if (matchCount == 1 && pHandleOut != 0) {
                                *pHandleOut = OpenProcess(0x400, 0, me.th32ProcessID);
                            }
                            if (matchCount >= wantCount) {
                                return 1;
                            }
                        }
                    } else {
                        if (_stricmp(name, me.szModule) == 0) {
                            matchCount++;
                            if (matchCount == 1 && pHandleOut != 0) {
                                *pHandleOut = OpenProcess(0x400, 0, me.th32ProcessID);
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
    } // namespace WinAPI
} // namespace Utils
