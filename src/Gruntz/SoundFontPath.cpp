#include <rva.h>
#include <Gruntz/SoundFont.h> // shared decls (CGruntzMgr::Run boot audio path)

#include <Win32.h> // GetCurrentDirectoryA / FreeLibrary

#include <Dsndmgr/SfManager.h> // real SFMANL101API device + the SFManager factory
#include <Globals.h>           // g_sfDevice, g_sfReady, g_sfDeviceCount, g_sfDll
#include <stdio.h>             // sprintf (0x11f890, _sprintf)
#include <string.h>            // strlen (inline repne scas)

DATA(0x0024dacc)
WORD g_sfCfgA0 = 0; // config block A +0 (runtime =1)
DATA(0x0024dace)
WORD g_sfCfgA2 = 0; // config block A +2 (runtime =0)
DATA(0x0024dad0)
u32 g_sfCfgB0 = 0; // config block B +0 (runtime =0x80)

DATA(0x0024dad4)
char* g_sfCurPath = 0; // the path currently being tried
DATA(0x0024dadc)
u16 g_sfCfgB12 = 0; // config block B +0xc
DATA(0x0024dae0)
extern char g_sfMusic4[]; // "<drive>:\MUSIC\Gruntz4.SF2"
DATA(0x0024dc28)
extern char g_sfLocal4[]; // "<cwd>\Gruntz4.SF2"
DATA(0x0024dd28)
WORD g_sfDeviceId;
DATA(0x0024dd30)
extern char g_sfMusic[]; // "<drive>:\MUSIC\Gruntz.SF2"
DATA(0x0024de30)
extern char g_sfLocal[]; // "<cwd>\Gruntz.SF2"
DATA(0x0024dfa0)
extern char g_sfDir[]; // GetCurrentDirectoryA(0xff, ...) scratch

i32 SfDeviceInitKeys();
i32 FileExistsCopyF90F0(char* szPath);

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

RVA(0x000f8ec0, 0x50)
i32 SfDeviceInitKeys() {
    if (g_sfReady == 0) {
        return 0;
    }
    g_sfCfgA2 = 0;
    for (i32 i = 1; i <= 0x7f; i++) {
        g_sfCfgA0 = static_cast<WORD>(i);
        (reinterpret_cast<SfGetLoadedBankPathname2>(g_sfDevice->SF_GetLoadedBankPathname))(g_sfDeviceId, reinterpret_cast<PSFMIDILOCATION>(&g_sfCfgA0));
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
            reinterpret_cast<PSFMIDILOCATION>(&g_sfCfgA0),
            reinterpret_cast<PSFBUFFEROBJECT>(&g_sfCfgB0)
        );
    }
    if (res != 0) {
        g_sfCurPath = hiVer ? g_sfMusic4 : g_sfMusic;
        res = g_sfDevice->SF_LoadBank(
            g_sfDeviceId,
            reinterpret_cast<PSFMIDILOCATION>(&g_sfCfgA0),
            reinterpret_cast<PSFBUFFEROBJECT>(&g_sfCfgB0)
        );
    }
    return res == 0;
}

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
