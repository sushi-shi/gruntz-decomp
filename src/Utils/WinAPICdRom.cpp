#include <Mfc.h>
#include <Utils/RegistryHelper.h>
#include <rva.h>
#include <stdio.h>

char GetGruntzDriveLetter();

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
    return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
}

RVA(0x0001fde0, 0x189)
char CheckCdRomRegistry() {
    u32 valueSize;
    char value[32];
    char drivePath[32];
    char cwdPath[256];
    Utils::RegistryHelper reg;
    char letter;
    i32 i;

    if (reg.Open(
            "Monolith Productions",
            "Gruntz",
            "1.0",
            0,
            reinterpret_cast<HKEY>(0x80000002) /*HKEY_LOCAL_MACHINE*/,
            0
        )) {
        valueSize = 0x1e;
        value[0] = 0;
        if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
            && static_cast<i8>(value[0]) > 0x14) {
            letter = value[0];
            sprintf(drivePath, "%c:\\", letter);
            if (GetDriveTypeA(drivePath) == 5 /*DRIVE_CDROM*/) {
                return letter;
            }
        }
    }

    GetCurrentDirectoryA(0xff, cwdPath);
    cwdPath[3] = 0;
    if (GetDriveTypeA(cwdPath) == 5 /*DRIVE_CDROM*/) {
        letter = cwdPath[0];
        return letter;
    }

    letter = 'A';
    for (i = 0; i < 26; i++) {
        sprintf(cwdPath, "%c:\\", letter);
        if (GetDriveTypeA(cwdPath) == 5 /*DRIVE_CDROM*/) {
            return letter;
        }
        letter++;
    }
    letter = 0;
    return letter;
}

static char s_cdDriveLetter;

// -------------------------------------------------------------------------
// GetGruntzDriveLetter (0x1ffe0)
// Finds the drive letter of the CD holding the Gruntz disc, memoising the result in
// s_cdDriveLetter:
//   1. read the saved drive letter from HKLM\Software\Monolith Productions\Gruntz\1.0
//      value "CdRom Drive"; if it is a real letter, a CD-ROM, and holds
//      "<L>:\GAME\GRUNTZ.EXE", use it;
//   2. otherwise scan drives A..Z for a CD-ROM holding that marker file.
// Returns the letter (0 if none). The local RegistryHelper's destructor (Close) runs
// at scope exit -> the C++ EH frame.
// @early-stop
// 98.4%: body byte-exact; residual is the `push 0xb` EH-scope-table value vs the
// delinked `push 0x0` (same accepted EH-frame artifact as CheckCdRomRegistry).
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
                reinterpret_cast<HKEY>(0x80000002) /*HKEY_LOCAL_MACHINE*/,
                0
            )) {
            valueSize = 0x1e;
            value[0] = 0;
            if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
                && static_cast<i8>(value[0]) > 0x14) {
                letter = value[0];
                sprintf(drivePath, "%c:\\", letter);
                if (GetDriveTypeA(drivePath) == 5 /*DRIVE_CDROM*/) {
                    sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                    if (FileExistsCopy(exePath)) {
                        goto found;
                    }
                }
            }
        }

        for (letter = 'A'; letter <= 'Z'; letter++) {
            sprintf(drivePathScan, "%c:\\", letter);
            if (GetDriveTypeA(drivePathScan) == 5 /*DRIVE_CDROM*/) {
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
