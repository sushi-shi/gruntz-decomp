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
#include <Gruntz/SoundFont.h> // shared decls (CGruntzMgr::Run boot audio path)

#include <Win32.h> // GetCurrentDirectoryA / FreeLibrary

#include <Dsndmgr/SfManager.h> // real SFMANL101API device + the SFManager factory
#include <Globals.h>           // g_sfDevice, g_sfReady, g_sfDeviceCount, g_sfDll
#include <stdio.h>             // sprintf (0x11f890, _sprintf)
#include <string.h>            // strlen (inline repne scas)

// Config block A (0x64dacc) - the same WORD globals SfDeviceInitKeys (below) seeds;
// DEFINED here (soundfontpath.obj's .bss), zero-init, DATA()-pinned. The load token
// g_sfDeviceId (0x64dd28) is shared with sfselectdevice, so its binding stays extern.
DATA(0x0024dacc)
WORD g_sfCfgA0 = 0; // config block A +0 (runtime =1)
DATA(0x0024dace)
WORD g_sfCfgA2 = 0; // config block A +2 (runtime =0)
DATA(0x0024dd28)
WORD g_sfDeviceId;

// The soundfont path/state globals - config block B + the four candidate SF2 path
// buffers + the current-directory scratch. Owned by this TU; DEFINED here (zero-init
// .bss), DATA()-pinned, reference externs kept in <Globals.h>. Ascending retail RVA.
DATA(0x0024dad0)
u32 g_sfCfgB0 = 0; // config block B +0 (runtime =0x80)
DATA(0x0024dad4)
char* g_sfCurPath = 0; // the path currently being tried
DATA(0x0024dadc)
u16 g_sfCfgB12 = 0; // config block B +0xc
// The four candidate SF2 path buffers + the current-directory scratch. These are
// NOT string literals (an earlier @undefined-data note here claimed they were, which
// is why they stayed unbound): all five are in the retail .data loader-zero TAIL, i.e.
// zero-init .bss BUFFERS that get WRITTEN - GetCurrentDirectoryA fills g_sfDir, and
// the four sprintf calls below fill the rest. Each address is read straight off the
// retail body of BuildSoundFontPath (0xf8f30): `push 0x64dfa0` into
// GetCurrentDirectoryA + `[ecx+0x64df9f]` for dir[len-1], and each sprintf's last
// push before the call is its destination. Ascending retail RVA.
//
// Binding-only: the extent is deliberately NOT declared. The gaps here are not proven
// extents (g_sfDeviceId at 0x64dd28 sits between two of them), and a guessed size
// would be enrolled as a delinker data extent - see docs/data-attribution.md. The
// definitions stay in <Globals.h>; these DATA() decls only pin name -> retail RVA so
// the delinker names the references instead of falling back to the nearest symbol.
DATA(0x0024dae0)
extern char g_sfMusic4[]; // "<drive>:\MUSIC\Gruntz4.SF2"
DATA(0x0024dc28)
extern char g_sfLocal4[]; // "<cwd>\Gruntz4.SF2"
DATA(0x0024dd30)
extern char g_sfMusic[]; // "<drive>:\MUSIC\Gruntz.SF2"
DATA(0x0024de30)
extern char g_sfLocal[]; // "<cwd>\Gruntz.SF2"
DATA(0x0024dfa0)
extern char g_sfDir[]; // GetCurrentDirectoryA(0xff, ...) scratch

// 0xf8ec0: re-seed the music device key table (defined below, in RVA order between
// CloseSoundFontDevice and BuildSoundFontPath; forward-declared here for Close).
i32 SfDeviceInitKeys();
// 0xf90f0: OpenFile(OF_EXIST) probe. The inline FileExists helper BuildSoundFontPath
// calls; MSVC emits this TU's out-of-line COMDAT copy at 0xf90f0 (defined below, in
// RVA order after BuildSoundFontPath). Byte-identical to the sibling copies in
// WinAPICdRom (FileExistsCopy1FD70) / HeapDiag (FileExists); the per-copy name is
// load-bearing (one-name-one-RVA delinker model forbids three symbols named FileExists).
i32 FileExistsCopyF90F0(char* szPath);

// CloseSoundFontDevice (0xf8e20): if a device is selected (init flag set, receiver
// present, at least one device counted), re-seed the key table, deselect the chosen
// index (the load token), free SFMAN32.DLL, and clear the DLL handle + init flag.
// Reached through the ILT thunk 0x129e; frameless __cdecl.
RVA(0x000f8e20, 0x56)
void CloseSoundFontDevice() {
    if (g_sfReady != 0 && g_sfDevice != 0 && g_sfDeviceCount != 0) {
        SfDeviceInitKeys();
        g_sfDevice->SF_Close(g_sfDeviceId);
        FreeLibrary(g_sfDll);
        g_sfDll = 0;
        g_sfReady = 0;
    }
}

// SfDeviceInitKeys (0xf8ec0): if the device is up, seed the music device's key table
// by walking key codes 1..0x7f through its SF_GetLoadedBankPathname slot (invoked
// 2-arg via the SfGetLoadedBankPathname2 cast), leaving the config word at 1.
// __cdecl; returns 0 when the device is down else 1. Re-homed from
// src/Stub/ReconBatch2.cpp (xref-proven: only Close + BuildSoundFontPath call it).
RVA(0x000f8ec0, 0x50)
i32 SfDeviceInitKeys() {
    if (g_sfReady == 0) {
        return 0;
    }
    g_sfCfgA2 = 0;
    for (i32 i = 1; i <= 0x7f; i++) {
        g_sfCfgA0 = (WORD)i;
        ((SfGetLoadedBankPathname2)
             g_sfDevice->SF_GetLoadedBankPathname)(g_sfDeviceId, (PSFMIDILOCATION)&g_sfCfgA0);
    }
    g_sfCfgA0 = 1;
    return 1;
}

RVA(0x000f8f30, 0x160)
i32 BuildSoundFontPath(char drive) {
    if (g_sfReady == 0) {
        return 0;
    }

    GetCurrentDirectoryA(0xff, g_sfDir);
    int len = static_cast<int>(strlen(g_sfDir));
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
    if (FileExistsCopyF90F0(g_sfCurPath)) {
        res = g_sfDevice->SF_LoadBank(
            g_sfDeviceId,
            (PSFMIDILOCATION)&g_sfCfgA0,
            (PSFBUFFEROBJECT)&g_sfCfgB0
        );
    }
    if (res != 0) {
        g_sfCurPath = hiVer ? g_sfMusic4 : g_sfMusic;
        res = g_sfDevice->SF_LoadBank(
            g_sfDeviceId,
            (PSFMIDILOCATION)&g_sfCfgA0,
            (PSFBUFFEROBJECT)&g_sfCfgB0
        );
    }
    return res == 0;
}

// FileExistsCopyF90F0 (0xf90f0): OpenFile(OF_EXIST) probe; false for a null/empty path.
// The out-of-line COMDAT copy of the inline helper BuildSoundFontPath calls (see fwd
// decl above). Re-homed from the deleted src/Utils/WinAPI.cpp.
RVA(0x000f90f0, 0x45)
i32 FileExistsCopyF90F0(char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
}
