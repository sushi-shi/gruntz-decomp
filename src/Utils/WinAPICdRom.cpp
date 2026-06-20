// WinAPICdRom.cpp - Utils::WinAPI::CheckCdRomRegistry, the sibling of
// WinAPI::GetGruntzDriveLetter (C:\Proj). Both find the drive letter of the CD
// holding the Gruntz disc by reading the saved letter from the registry, then
// falling back to a drive scan; this one is marker-checked via
// GetCurrentDirectoryA instead of a probe file and does NOT memoise.
//
// Built /GX (the `eh` flag profile): the local Utils::RegistryHelper's
// destructor (Close) must run on every early return, so the body carries the
// C++ EH frame (push -1 / push handler / mov fs:0,esp) that unwinds it - exactly
// like GetGruntzDriveLetter. Only offsets / strings / code bytes are load-bearing.
//
// Body is instruction-for-instruction correct (Open/GetValueString/the >0x14
// check/sprintf/GetDriveTypeA==5/GetCurrentDirectoryA/the 26-drive scan, with the
// destructor's four Close calls). Sits at the EH-codegen plateau (~76% fuzzy): the
// only residuals are (1) the `push 0xb` EH-scope-table value vs the delinked
// `push 0x0` - the SAME accepted artifact the matched GetGruntzDriveLetter carries
// at 98% - and (2) MSVC duplicating the bare epilogue inline at each return where
// retail outlined it (one shared `jmp` tail): identical instructions, a tail-
// sharing-vs-duplication coin-flip, not a source bug. Kept in the truest shape
// (multiple returns -> four Close) rather than collapsing to one Close for a
// higher score.
// <Mfc.h> brings <windows.h> KERNEL32 (GetDriveTypeA / GetCurrentDirectoryA; UINT /
// LPCSTR / DWORD / LPSTR).
#include <Mfc.h>
#include <Utils/RegistryHelper.h>
#include <rva.h>
#include <stdio.h>

namespace Utils {
namespace WinAPI {

// ---------------------------------------------------------------------------
// CheckCdRomRegistry
// Returns the drive letter of the CD-ROM holding the Gruntz disc, or 0:
//   1. read HKLM\Software\Monolith Productions\Gruntz\1.0 value "CdRom Drive";
//      if it names a real CD-ROM drive, use it;
//   2. else if the current working directory is on a CD-ROM, use that drive;
//   3. else scan drives A..Z for any CD-ROM.
RVA(0x1fde0, 0x189)
char CheckCdRomRegistry()
{
    unsigned int valueSize;
    char value[32];
    char drivePath[32];
    char cwdPath[256];
    RegistryHelper reg;
    char letter;
    int i;

    if (reg.Open("Monolith Productions", "Gruntz", "1.0", 0,
                 (HKEY)0x80000002 /*HKEY_LOCAL_MACHINE*/, 0)) {
        valueSize = 0x1e;
        value[0] = 0;
        if (reg.GetValueString("CdRom Drive", value, &valueSize, 0)
            && (signed char)value[0] > 0x14) {
            letter = value[0];
            sprintf(drivePath, "%c:\\", letter);
            if (GetDriveTypeA(drivePath) == 5 /*DRIVE_CDROM*/)
                return letter;
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
        if (GetDriveTypeA(cwdPath) == 5 /*DRIVE_CDROM*/)
            return letter;
        letter++;
    }
    letter = 0;
    return letter;
}

} // namespace WinAPI
} // namespace Utils
