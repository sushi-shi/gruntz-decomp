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
#include <string.h>
#include <stdio.h>

namespace Utils {
    namespace WinAPI {

        char GetGruntzDriveLetter();

        // -------------------------------------------------------------------------
        // FileExists
        // Tests a path via OpenFile(OF_EXIST). Returns false for a null/empty path.
        // (Re-emitted again elsewhere in this TU, same bytes.)
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
        // ActiveWait
        // Busy-waits ~milliseconds using timeGetTime (no Sleep; spins).
        RVA(0x0013dfe0, 0x21)
        void ActiveWait(u32 milliseconds) {
            DWORD target = timeGetTime() + milliseconds;
            while (timeGetTime() < target)
                ;
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
        // Engine-label backlog stubs.
        // -------------------------------------------------------------------------
        // @confidence: high
        // @source: tomalla
        // @stub
        RVA(0x00118ce0, 0x1f5)
        void Stub_118ce0() {}
    } // namespace WinAPI
} // namespace Utils
