// SoundFontPath.cpp - BuildSoundFontPath (0xf8f30).
//
// Builds the four candidate Gruntz SF2 soundfont paths (the current dir's
// Gruntz4.SF2 / Gruntz.SF2 and the CD's <drive>:\MUSIC\Gruntz(4).SF2), then asks
// the music device (g_keyRecv_64e0b0->+0x2c loader) to load the first that exists,
// falling back to the MUSIC-dir copy. The newer build (g_sfVer >= 0x37115c)
// prefers the "Gruntz4" variant. All state lives in module globals; frameless.
#include <rva.h>

#include <Win32.h> // GetCurrentDirectoryA

#include <Globals.h> // g_keyRecv_64e0b0, g_initFlag_64e0b8
#include <stdio.h>   // sprintf (0x11f890, _sprintf)
#include <string.h>  // strlen (inline repne scas)

// The music device object (reached through g_keyRecv_64e0b0): a __cdecl loader
// function pointer at +0x2c that loads a soundfont (token + the two config blocks).
struct SfMusicMgr {
    char m_pad00[0x2c];
    i32(__cdecl* Load)(u16 token, void* cfgA, void* cfgB); // +0x2c
};

// The config blocks A (0x64dacc) and the load token (0x64dd28) are the same WORD
// globals InitKeys_f8ec0 (ReconBatch2.cpp) seeds; reference its names so the
// reloc symbols line up (their DATA() binding lives there).
extern WORD g_word_64dacc; // 0x64dacc  config block A +0 (=1)
extern WORD g_word_64dace; // 0x64dace  config block A +2 (=0)
extern WORD g_word_64dd28; // 0x64dd28  soundfont load token

// The soundfont path/state globals.
DATA(0x0024dad0)
extern u32 g_sfCfgB0; // 0x64dad0  config block B +0 (=0x80)
DATA(0x0024dad4)
extern char* g_sfCurPath; // 0x64dad4  the path currently being tried
DATA(0x0024dadc)
extern u16 g_sfCfgB12; // 0x64dadc  config block B +0xc (=0)
DATA(0x0024dc28)
extern char g_sfLocal4[]; // 0x64dc28  "<dir>\Gruntz4.SF2"
DATA(0x0024dd30)
extern char g_sfMusic[]; // 0x64dd30  "<drive>:\MUSIC\Gruntz.SF2"
DATA(0x0024dae0)
extern char g_sfMusic4[]; // 0x64dae0  "<drive>:\MUSIC\Gruntz4.SF2"
DATA(0x0024de30)
extern char g_sfLocal[]; // 0x64de30  "<dir>\Gruntz.SF2"
DATA(0x0024dfa0)
extern char g_sfDir[]; // 0x64dfa0  current-directory scratch (0xff)
DATA(0x0024e0a0)
extern u32 g_sfVer; // 0x64e0a0  build/version selector

// 0xf8ec0 (via ILT thunk 0x3382): re-seed the music device key table.
int InitKeys_f8ec0();
namespace EngineLabelBacklog {
    // 0xf90f0 (via ILT thunk 0x3d32): OpenFile(OF_EXIST) probe.
    int Stub_0f90f0(char* szPath);
} // namespace EngineLabelBacklog

RVA(0x000f8f30, 0x160)
i32 BuildSoundFontPath(char drive) {
    if (g_initFlag_64e0b8 == 0) {
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
    InitKeys_f8ec0();

    g_word_64dacc = 1;
    g_word_64dace = 0;
    g_sfCfgB0 = 0x80;
    g_sfCfgB12 = 0;
    int hiVer = 0;
    i32 res = 0x6b;
    if (g_sfVer >= 0x37115c) {
        hiVer = 1;
    }
    g_sfCurPath = hiVer ? g_sfLocal4 : g_sfLocal;
    if (EngineLabelBacklog::Stub_0f90f0(g_sfCurPath)) {
        res = ((SfMusicMgr*)g_keyRecv_64e0b0)->Load(g_word_64dd28, &g_word_64dacc, &g_sfCfgB0);
    }
    if (res != 0) {
        g_sfCurPath = hiVer ? g_sfMusic4 : g_sfMusic;
        res = ((SfMusicMgr*)g_keyRecv_64e0b0)->Load(g_word_64dd28, &g_word_64dacc, &g_sfCfgB0);
    }
    return res == 0;
}
SIZE_UNKNOWN(SfMusicMgr);
