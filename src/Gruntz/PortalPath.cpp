#include <Mfc.h> // HKEY / windows.h via afx
#include <Utils/RegistryHelper.h>
#include <rva.h>
#include <string.h> // inline strlen/strcat/strcpy

extern i32 FileExists(char* path);

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
            reinterpret_cast<HKEY>(0x80000002) /*HKEY_LOCAL_MACHINE*/,
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
    if (!FileExists(regBuf)) {
        return 0;
    }
    if (outPath != 0) {
        strcpy(outPath, regBuf);
    }
    return 1;
}

RVA(0x00090860, 0xd3)
i32 __stdcall LaunchProcessInDir(char* exe, char* dir) {
    char cmdline[256];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (dir && *dir) {
        i32 len = strlen(dir);
        if (len > 0 && dir[len - 1] == '\\') {
            wsprintfA(cmdline, "%s%s", dir, exe);
        } else {
            wsprintfA(cmdline, "%s\\%s", dir, exe);
        }
    } else {
        wsprintfA(cmdline, "%s", exe);
    }
    if (dir && *dir == 0) {
        dir = 0;
    }
    return CreateProcessA(0, cmdline, 0, 0, 0, 0, 0, dir, &si, &pi);
}
