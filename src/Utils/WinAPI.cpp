// WinAPI.cpp - Utils::WinAPI free-function helpers (a thin layer over a handful
// of KERNEL32 / WINMM imports + the registry/config wrapper). These are static
// free functions in namespace Utils::WinAPI; only offsets + code bytes are
// load-bearing. We do NOT pull in <windows.h> - the minimal __declspec(dllimport)
// __stdcall block reproduces the FF15 [IAT] direct-call form and keeps the
// visible symbol set small (entropy follows header churn; see
// docs/matching-patterns.md).
#include "RegistryHelper.h"
#include <string.h>
#include <stdio.h>

typedef unsigned int UINT;
typedef int          HFILE;

struct OFSTRUCT {
    unsigned char cBytes;
    unsigned char fFixedDisk;
    unsigned short nErrCode;
    unsigned short Reserved1;
    unsigned short Reserved2;
    char szPathName[128];
};

extern "C" {
__declspec(dllimport) HFILE __stdcall OpenFile(LPCSTR lpFileName, OFSTRUCT *lpReOpenBuff, UINT uStyle);
__declspec(dllimport) UINT  __stdcall GetDriveTypeA(LPCSTR lpRootPathName);
__declspec(dllimport) DWORD __stdcall timeGetTime(void);
}

namespace Utils {
namespace WinAPI {

char GetGruntzDriveLetter();

// -------------------------------------------------------------------------
// FileExists  @ RVA 0x1189c0 (69 B) and again @ RVA 0x1fd70 (same bytes,
// re-emitted in this TU). Tests a path via OpenFile(OF_EXIST). Returns false
// for a null/empty path.
// -------------------------------------------------------------------------
// @address: 0x1189c0
int FileExists(char *szPath)
{
    OFSTRUCT of;

    if (!szPath)
        return 0;
    if (!*szPath)
        return 0;
    return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
}

// -------------------------------------------------------------------------
// ActiveWait  @ RVA 0x13dfe0 (33 B). Busy-waits ~milliseconds using
// timeGetTime (no Sleep; spins).
// -------------------------------------------------------------------------
// @address: 0x13dfe0
void ActiveWait(unsigned int milliseconds)
{
    DWORD target = timeGetTime() + milliseconds;
    while (timeGetTime() < target)
        ;
}

// -------------------------------------------------------------------------
// IsGruntzCDInAnyDrive  @ RVA 0x1fd50 (15 B). True iff a CD drive holding the
// Gruntz disc was found.
// -------------------------------------------------------------------------
// @address: 0x1fd50
int IsGruntzCDInAnyDrive()
{
    char letter = GetGruntzDriveLetter();
    return letter != 0;
}

// File-scope cache of the discovered Gruntz CD drive letter (binary: byte
// @0x62b25c). Zero = not yet probed / not found. Once a drive is found the
// letter is memoised so later calls return it without re-scanning.
static char s_cdDriveLetter;

// -------------------------------------------------------------------------
// GetGruntzDriveLetter  @ RVA 0x1ffe0 (402 B). Finds the drive letter of the
// CD holding the Gruntz disc, memoising the result in s_cdDriveLetter:
//   1. read the saved drive letter from HKLM\Software\Monolith Productions\
//      Gruntz\1.0 value "CdRom Drive"; if it is a real letter, a CD-ROM, and
//      holds "<L>:\GAME\GRUNTZ.EXE", use it;
//   2. otherwise scan drives A..Z for a CD-ROM holding that marker file.
// Returns the letter (0 if none). The local RegistryHelper's destructor
// (Close) runs at scope exit -> the C++ EH frame.
// -------------------------------------------------------------------------
// @address: 0x1ffe0
char GetGruntzDriveLetter()
{
    if (s_cdDriveLetter == 0) {
        unsigned int valueSize;
        char value[32];
        char drivePath[32];
        char exePath[256];
        Utils::RegistryHelper reg;
        char drivePathScan[256];
        char letter;

        if (reg.Open("Monolith Productions", "Gruntz", "1.0", 0,
                     (HKEY)0x80000002 /*HKEY_LOCAL_MACHINE*/, 0)) {
            valueSize = 0x1e;
            value[0] = 0;
            if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
                && (signed char)value[0] > 0x14) {
                letter = value[0];
                sprintf(drivePath, "%c:\\", letter);
                if (GetDriveTypeA(drivePath) == 5 /*DRIVE_CDROM*/) {
                    sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                    if (FileExists(exePath))
                        goto found;
                }
            }
        }

        for (letter = 'A'; letter <= 'Z'; letter++) {
            sprintf(drivePathScan, "%c:\\", letter);
            if (GetDriveTypeA(drivePathScan) == 5 /*DRIVE_CDROM*/) {
                sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                if (FileExists(exePath))
                    goto found;
            }
        }
        return 0;

found:
        s_cdDriveLetter = letter;
        return letter;
    }
    return s_cdDriveLetter;
}

} // namespace WinAPI
} // namespace Utils
