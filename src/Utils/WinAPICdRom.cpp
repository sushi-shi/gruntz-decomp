// WinAPICdRom.cpp - the Gruntz CD-ROM detection TU (RVA 0x1fd50..0x20172): the
// disc-present probe (IsGruntzCDInAnyDrive), an OpenFile(OF_EXIST) helper
// (FileExistsCopy1FD70, the inline COMDAT copy GetGruntzDriveLetter calls), and the two
// registry+drive-scan letter finders (CheckCdRomRegistry, GetGruntzDriveLetter).
// These four interleave in retail .text - 0x1fd50/0x1fd70 (formerly claimed by the
// fake grab-bag src/Utils/WinAPI.cpp), 0x1fde0 (this file), 0x1ffe0 (also WinAPI.cpp)
// - which can only happen inside ONE .obj: they are one real TU. The fake
// Utils::WinAPI namespace has been dropped; these are plain file-scope free functions.
//
// Built /GX (the `eh` flag profile): the local Utils::RegistryHelper's destructor
// (Close) must run on every early return in CheckCdRomRegistry / GetGruntzDriveLetter,
// so those bodies carry the C++ EH frame (push -1 / push handler / mov fs:0,esp).
// Only offsets / strings / code bytes are load-bearing.
#include <Mfc.h>
#include <Utils/RegistryHelper.h>
#include <rva.h>
#include <stdio.h>

// GetGruntzDriveLetter is defined last (RVA order) but called by IsGruntzCDInAnyDrive.
char GetGruntzDriveLetter();

// -------------------------------------------------------------------------
// IsGruntzCDInAnyDrive (0x1fd50)
// True iff a CD drive holding the Gruntz disc was found.
RVA(0x0001fd50, 0xf)
i32 IsGruntzCDInAnyDrive() {
    char letter = GetGruntzDriveLetter();
    return letter != 0;
}

// -------------------------------------------------------------------------
// FileExistsCopy1FD70 (0x1fd70)
// Tests a path via OpenFile(OF_EXIST). Returns false for a null/empty path. This is
// the CD-ROM TU's out-of-line COMDAT copy of the inline FileExists helper that
// GetGruntzDriveLetter calls; byte-identical to the copies emitted in SoundFontPath
// (FileExistsCopyF90F0) / HeapDiag (FileExists). The per-copy name is load-bearing:
// the one-name-one-RVA delinker model forbids three symbols named `FileExists`.
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

// ---------------------------------------------------------------------------
// CheckCdRomRegistry
// Returns the drive letter of the CD-ROM holding the Gruntz disc, or 0:
//   1. read HKLM\Software\Monolith Productions\Gruntz\1.0 value "CdRom Drive";
//      if it names a real CD-ROM drive, use it;
//   2. else if the current working directory is on a CD-ROM, use that drive;
//   3. else scan drives A..Z for any CD-ROM.
// objdiff 100.0 (reloc-masked ceiling). Reassembling the real CD-ROM TU (this file +
// the three functions re-homed out of the fake WinAPI.cpp) resolved the former ~76%
// residual: with the sibling EH functions present, MSVC now shares one outlined
// epilogue tail (retail's shape) instead of duplicating the bare epilogue per return.
// The lone raw-diff left is the EH-scope-table push (`push -1` vs retail's relocated
// `push <scopetable>`) - a masked reloc operand, not a code-byte difference.
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
            (HKEY)0x80000002 /*HKEY_LOCAL_MACHINE*/,
            0
        )) {
        valueSize = 0x1e;
        value[0] = 0;
        if (reg.GetValueString("CdRom Drive", value, &valueSize, 0) && static_cast<i8>(value[0]) > 0x14) {
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

// File-scope cache of the discovered Gruntz CD drive letter. Zero = not yet probed /
// not found. Once a drive is found the letter is memoised so later calls return it
// without re-scanning.
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
                (HKEY)0x80000002 /*HKEY_LOCAL_MACHINE*/,
                0
            )) {
            valueSize = 0x1e;
            value[0] = 0;
            if (reg.GetValueString("CdRom Drive", value, &valueSize, 0) && static_cast<i8>(value[0]) > 0x14) {
                letter = value[0];
                sprintf(drivePath, "%c:\\", letter);
                if (GetDriveTypeA(drivePath) == 5 /*DRIVE_CDROM*/) {
                    sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                    if (FileExistsCopy1FD70(exePath)) {
                        goto found;
                    }
                }
            }
        }

        for (letter = 'A'; letter <= 'Z'; letter++) {
            sprintf(drivePathScan, "%c:\\", letter);
            if (GetDriveTypeA(drivePathScan) == 5 /*DRIVE_CDROM*/) {
                sprintf(exePath, "%c:\\GAME\\GRUNTZ.EXE", letter);
                if (FileExistsCopy1FD70(exePath)) {
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
