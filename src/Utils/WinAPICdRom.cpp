#include <StdAfx.h>

#include <Ints.h>

#include <Utils/WinAPICdRom.h>

#include <Io/Settings.h>

#include <stdio.h>

namespace {
#include <Utils/FileExists.h>
}

char g_cdDriveLetter;

i32 IsGruntzCDInAnyDrive() {
    char letter = GetGruntzDriveLetter();
    return letter != 0;
}

char CheckCdRomRegistry() {

    char drivePath[32];
    char sDir[256];
    Settings reg;
    char cdDrive;
    i32 i;

    if (reg.load(settingsPath())) {
        const std::string sDrive = reg.getString("CdRom Drive");
        if (!sDrive.empty() && sDrive[0] > 20) {
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

char GetGruntzDriveLetter() {
    if (g_cdDriveLetter == 0) {

        char drivePath[32];
        char exePath[256];
        Settings reg;
        char drivePathScan[256];
        char letter;

        if (reg.load(settingsPath())) {
            const std::string value = reg.getString("CdRom Drive");
            if (!value.empty() && static_cast<i8>(value[0]) > 0x14) {
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
