#include <rva.h>

#include <Utils/WinAPICdRom.h>

#include <Mfc.h>

#include <Utils/RegistryHelper.h>

#include <stdio.h>

DATA(0x0022b25c)
char g_cdDriveLetter;

RVA(0x0001fd50, 0xf)
i32 IsGruntzCDInAnyDrive() {
    char letter = GetGruntzDriveLetter();
    return letter != 0;
}

RVA(0x0001fd70, 0x45)
i32 FileExistsCopy(char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000) != -1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0001fde0, 0x189)
char CheckCdRomRegistry() {
    DWORD valueSize;
    char value[32];
    char drivePath[32];
    char cwdPath[256];
    Utils::RegistryHelper reg;
    char letter;
    i32 i;

    if (reg.Open("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL)) {
        valueSize = 0x1e;
        value[0] = 0;
        if (reg.GetValueString("CdRom Drive", value, &valueSize, NULL)
            && static_cast<i8>(value[0]) > 0x14) {
            letter = value[0];
            sprintf(drivePath, "%c:\\", letter);
            if (GetDriveTypeA(drivePath) == DRIVE_CDROM) {
                return letter;
            }
        }
    }

    GetCurrentDirectoryA(0xff, cwdPath);
    cwdPath[3] = 0;
    if (GetDriveTypeA(cwdPath) == DRIVE_CDROM) {
        letter = cwdPath[0];
        return letter;
    }

    letter = 'A';
    for (i = 0; i < 26; i++) {
        sprintf(cwdPath, "%c:\\", letter);
        if (GetDriveTypeA(cwdPath) == DRIVE_CDROM) {
            return letter;
        }
        letter++;
    }
    letter = 0;
    return letter;
}

RVA(0x0001ffe0, 0x192)
char GetGruntzDriveLetter() {
    if (g_cdDriveLetter == 0) {
        DWORD valueSize;
        char value[32];
        char drivePath[32];
        char exePath[256];
        Utils::RegistryHelper reg;
        char drivePathScan[256];
        char letter;

        if (reg.Open("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL)) {
            valueSize = 0x1e;
            value[0] = 0;
            if (reg.GetValueString("CdRom Drive", value, &valueSize, NULL)
                && static_cast<i8>(value[0]) > 0x14) {
                char regLetter = value[0];
                sprintf(drivePath, "%c:\\", regLetter);
                if (GetDriveTypeA(drivePath) == DRIVE_CDROM) {
                    letter = regLetter;
                    sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                    if (FileExistsCopy(exePath)) {
                        goto found;
                    }
                }
            }
        }

        for (letter = 'A'; letter <= 'Z'; letter++) {
            sprintf(drivePathScan, "%c:\\", letter);
            if (GetDriveTypeA(drivePathScan) == DRIVE_CDROM) {
                sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                if (FileExistsCopy(exePath)) {
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

// cl emits the header-inline `~RegistryHelper() { Close(); }` out of line here
// (a 5-byte `jmp ?Close@...`), because the three unwind funclets that clean up a
// stack RegistryHelper take its address: winapicdrom CheckCdRomRegistry /
// GetGruntzDriveLetter and gruntzmgr LaunchPortalExe. Retail keeps the one copy
// right after this TU's band, reached through the ILT thunk at 0x00004115.
RVA_COMPGEN(0x000201f0, 0x5, ??1RegistryHelper@Utils@@QAE@XZ)
