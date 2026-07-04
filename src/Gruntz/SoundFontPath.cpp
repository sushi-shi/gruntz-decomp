// SoundFontPath.cpp - the SFMAN32.DLL soundfont-device teardown (0xf8e20) and
// BuildSoundFontPath (0xf8f30).
//
// CloseSoundFontDevice tears the selected SFMAN32 device down (re-seed keys,
// deselect the chosen index, free the DLL, clear the init/handle globals).
// BuildSoundFontPath builds the four candidate Gruntz SF2 soundfont paths (the
// current dir's Gruntz4.SF2 / Gruntz.SF2 and the CD's <drive>:\MUSIC\Gruntz(4).SF2),
// then asks the music device (g_sfDevice->+0x2c loader) to load the first that
// exists, falling back to the MUSIC-dir copy. The newer build (g_sfVer >= 0x37115c)
// prefers the "Gruntz4" variant. All state lives in module globals; frameless.
#include <rva.h>

#include <Win32.h> // GetCurrentDirectoryA / FreeLibrary

#include <Gruntz/SfManagerDevice.h> // the SFMAN32 device interface (*0x64e0b0); Deselect (+0x14)
#include <Globals.h>                // g_sfDevice, g_sfReady, g_sfDeviceCount, g_sfDll
#include <stdio.h>                  // sprintf (0x11f890, _sprintf)
#include <string.h>                 // strlen (inline repne scas)

// The config blocks A (0x64dacc) and the load token (0x64dd28) are the same WORD
// globals SfDeviceInitKeys (ReconBatch2.cpp) seeds; reference its names so the
// reloc symbols line up (their DATA() binding lives there).
extern WORD g_sfCfgA0;    // 0x64dacc  config block A +0 (=1)
extern WORD g_sfCfgA2;    // 0x64dace  config block A +2 (=0)
extern WORD g_sfDeviceId; // 0x64dd28  soundfont load token

// The soundfont path/state globals.

// 0xf8ec0 (via ILT thunk 0x3382): re-seed the music device key table.
int SfDeviceInitKeys();
// 0xf90f0 (via ILT thunk 0x3d32): OpenFile(OF_EXIST) probe. The real function is
// Utils::WinAPI::FileExistsCopyF90F0 (src/Utils/WinAPI.cpp); reloc-masked callee.
namespace Utils {
    namespace WinAPI {
        i32 FileExistsCopyF90F0(char* szPath);
    }
} // namespace Utils

// CloseSoundFontDevice (0xf8e20): if a device is selected (init flag set, receiver
// present, at least one device counted), re-seed the key table, deselect the chosen
// index (the load token), free SFMAN32.DLL, and clear the DLL handle + init flag.
// Reached through the ILT thunk 0x129e; frameless __cdecl.
RVA(0x000f8e20, 0x56)
void CloseSoundFontDevice() {
    if (g_sfReady != 0 && g_sfDevice != 0 && g_sfDeviceCount != 0) {
        SfDeviceInitKeys();
        g_sfDevice->Deselect(g_sfDeviceId);
        FreeLibrary(g_sfDll);
        g_sfDll = 0;
        g_sfReady = 0;
    }
}

RVA(0x000f8f30, 0x160)
i32 BuildSoundFontPath(char drive) {
    if (g_sfReady == 0) {
        return 0;
    }

    GetCurrentDirectoryA(0xff, g_sfDir);
    int len = (int)strlen(g_sfDir);
    if (len > 0 && g_sfDir[len - 1] == '\\') {
        g_sfDir[len - 1] = '\0';
    }
    sprintf(g_sfLocal4, "%s\\Gruntz4.SF2", g_sfDir);
    sprintf(g_sfLocal, "%s\\Gruntz.SF2", g_sfDir);
    sprintf(g_sfMusic4, "%c:\\MUSIC\\Gruntz4.SF2", drive);
    sprintf(g_sfMusic, "%c:\\MUSIC\\Gruntz.SF2", drive);
    SfDeviceInitKeys();

    g_sfCfgA0 = 1;
    g_sfCfgA2 = 0;
    g_sfCfgB0 = 0x80;
    g_sfCfgB12 = 0;
    int hiVer = 0;
    i32 res = 0x6b;
    if (g_sfVer >= 0x37115c) {
        hiVer = 1;
    }
    g_sfCurPath = hiVer ? g_sfLocal4 : g_sfLocal;
    if (Utils::WinAPI::FileExistsCopyF90F0(g_sfCurPath)) {
        res = g_sfDevice->Load(g_sfDeviceId, &g_sfCfgA0, &g_sfCfgB0);
    }
    if (res != 0) {
        g_sfCurPath = hiVer ? g_sfMusic4 : g_sfMusic;
        res = g_sfDevice->Load(g_sfDeviceId, &g_sfCfgA0, &g_sfCfgB0);
    }
    return res == 0;
}
