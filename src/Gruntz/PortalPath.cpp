// PortalPath.cpp - LaunchPortalExe (0x90550). Despite the label it does not spawn
// a process: it resolves the installed "Portal" companion's executable path from
// the registry. __stdcall(outPath): opens HKLM\Software\Monolith Productions\
// Portal\1.0, reads the "filedir" value, ensures a trailing backslash, appends
// "portal.exe", and (iff the file exists) copies the full path into outPath.
// Returns 1 on success, 0 otherwise.
//
// Sibling of Utils::WinAPI::CheckCdRomRegistry: the local Utils::RegistryHelper's
// destructor (Close) runs on every early return, so the body carries the /GX C++
// EH frame (push -1 / push handler / mov fs:0,esp). Built on the `eh` profile.
// Only offsets / strings / code bytes are load-bearing; the inline CRT
// strlen/strcat/strcpy come from the intrinsics.
#include <Mfc.h> // HKEY / windows.h via afx
#include <Utils/RegistryHelper.h>
#include <rva.h>
#include <string.h> // inline strlen/strcat/strcpy

// FUN_00004282 __cdecl: probe whether a path exists (OpenFile OF_EXIST). Reloc-
// masked external (same helper LoadCustomWorldInfo uses).
extern i32 PathFileExists(char* path);

// @source: decomp-xref
RVA(0x00090550, 0x1e6)
i32 __stdcall LaunchPortalExe(char* outPath) {
    u32 bufSize;
    char regBuf[0x100];
    Utils::RegistryHelper reg;

    if (!reg.Open(
            "Monolith Productions",
            "Portal",
            "1.0",
            0,
            (HKEY)0x80000002 /*HKEY_LOCAL_MACHINE*/,
            0
        )) {
        return 0;
    }
    regBuf[0] = 0;
    bufSize = 0xde;
    if (!reg.GetValueString("filedir", regBuf, &bufSize, 0)) {
        return 0;
    }
    i32 len = strlen(regBuf);
    if (len < 1) {
        return 0;
    }
    if (regBuf[len - 1] != '\\') {
        strcat(regBuf, "\\");
    }
    strcat(regBuf, "portal.exe");
    if (!PathFileExists(regBuf)) {
        return 0;
    }
    if (outPath != 0) {
        strcpy(outPath, regBuf);
    }
    return 1;
}
