#include <rva.h>

#include <Utils/WinAPICdRom.h>

#include <Mfc.h>

#include <Utils/RegistryHelper.h>

#include <stdio.h>

DATA(0x0022b25c)
u8 g_val_22b25c[1];
static char s_cdDriveLetter;

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

RVA(0x0001fde0, 0x189)
char CheckCdRomRegistry() {
    DWORD valueSize;
    char value[32];
    char drivePath[32];
    char cwdPath[256];
    Utils::RegistryHelper reg;
    char letter;
    i32 i;

    if (reg.Open("Monolith Productions", "Gruntz", "1.0", 0, HKEY_LOCAL_MACHINE, 0)) {
        valueSize = 0x1e;
        value[0] = 0;
        if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
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

// @early-stop
RVA(0x0001ffe0, 0x192)
char GetGruntzDriveLetter() {
    if (s_cdDriveLetter == 0) {
        DWORD valueSize;
        char value[32];
        char drivePath[32];
        char exePath[256];
        Utils::RegistryHelper reg;
        char drivePathScan[256];
        char letter;

        if (reg.Open("Monolith Productions", "Gruntz", "1.0", 0, HKEY_LOCAL_MACHINE, 0)) {
            valueSize = 0x1e;
            value[0] = 0;
            if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
                && static_cast<i8>(value[0]) > 0x14) {
                letter = value[0];
                sprintf(drivePath, "%c:\\", letter);
                if (GetDriveTypeA(drivePath) == DRIVE_CDROM) {
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
        s_cdDriveLetter = letter;
        return letter;
    }
    return s_cdDriveLetter;
}
