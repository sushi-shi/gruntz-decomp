#include <rva.h>

#include <Utils/WinAPICdRom.h>

#include <Mfc.h>

#include <Utils/RegMgr.h>

#include <stdio.h>

namespace {
#include <Utils/FileExists.h>
} // namespace

DATA(0x0022b25c)
char g_cdDriveLetter;

RVA(0x0001fd50, 0xf)
i32 IsGruntzCDInAnyDrive() {
    char letter = GetGruntzDriveLetter();
    return letter != 0;
}

RVA_COMPGEN(0x0001fd70, 0x45, ?FileExists@?A0xd84d0d674a5580a4@@YAHPBD@Z)

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0001fde0, 0x189)
char CheckCdRomRegistry() {
    DWORD bufsize;
    char sDrive[32];
    char drivePath[32];
    char sDir[256];
    CRegMgr reg;
    char cdDrive;
    i32 i;

    if (reg.Init("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL)) {
        bufsize = 30;
        sDrive[0] = '\0';
        if (reg.Get("CdRom Drive", sDrive, bufsize, NULL) && sDrive[0] > 20) {
            cdDrive = sDrive[0];
            sprintf(drivePath, "%c:\\", cdDrive);
            if (GetDriveTypeA(drivePath) == DRIVE_CDROM) {
                return cdDrive;
            }
        }
    }

    GetCurrentDirectoryA(255, sDir);
    sDir[3] = '\0';
    if (GetDriveTypeA(sDir) == DRIVE_CDROM) {
        cdDrive = sDir[0];
        return cdDrive;
    }

    cdDrive = 'A';
    for (i = 0; i < 26; i++) {
        sprintf(sDir, "%c:\\", cdDrive);
        if (GetDriveTypeA(sDir) == DRIVE_CDROM) {
            return cdDrive;
        }
        cdDrive++;
    }
    cdDrive = 0;
    return cdDrive;
}

RVA(0x0001ffe0, 0x192)
char GetGruntzDriveLetter() {
    if (g_cdDriveLetter == 0) {
        DWORD valueSize;
        char value[32];
        char drivePath[32];
        char exePath[256];
        CRegMgr reg;
        char drivePathScan[256];
        char letter;

        if (reg.Init("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL)) {
            valueSize = 0x1e;
            value[0] = 0;
            if (reg.Get("CdRom Drive", value, valueSize, NULL)
                && static_cast<i8>(value[0]) > 0x14) {
                char regLetter = value[0];
                sprintf(drivePath, "%c:\\", regLetter);
                if (GetDriveTypeA(drivePath) == DRIVE_CDROM) {
                    letter = regLetter;
                    sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                    if (FileExists(exePath)) {
                        goto found;
                    }
                }
            }
        }

        for (letter = 'A'; letter <= 'Z'; letter++) {
            sprintf(drivePathScan, "%c:\\", letter);
            if (GetDriveTypeA(drivePathScan) == DRIVE_CDROM) {
                sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                if (FileExists(exePath)) {
                    goto found;
                }
            }
        }
        return 0;

    found:
        g_cdDriveLetter = letter;
        return letter;
    }
    return g_cdDriveLetter;
}

RVA_COMPGEN(0x000201f0, 0x5, ??1CRegMgr@@QAE@XZ)
