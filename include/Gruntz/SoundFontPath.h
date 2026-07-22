// SoundFontPath.h - the SoundFontPath.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_SOUNDFONTPATH_H
#define GRUNTZ_GRUNTZ_SOUNDFONTPATH_H

#include <Ints.h>

extern u16 g_sfDeviceId; // 0x0024dd28 (WORD; u16 here - the header precedes windows.h)


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern char g_sfMusic4[]; // "<drive>:\MUSIC\Gruntz4.SF2"
extern char g_sfLocal4[]; // "<cwd>\Gruntz4.SF2"
extern char g_sfMusic[]; // "<drive>:\MUSIC\Gruntz.SF2"
extern char g_sfLocal[]; // "<cwd>\Gruntz.SF2"
extern char g_sfDir[]; // GetCurrentDirectoryA(0xff, ...) scratch

extern "C" char g_id0_613dff; // (def in SFSelectDevice.cpp)
extern "C" char g_id1_613e00; // (def in SFSelectDevice.cpp)
extern "C" char g_id2_613e01; // (def in SFSelectDevice.cpp)
extern "C" char g_id3_613e02; // (def in SFSelectDevice.cpp)

extern "C" char g_id0_613dff;
extern "C" char g_id1_613e00;
extern "C" char g_id2_613e01;
extern "C" char g_id3_613e02;
extern u32 g_sfCfgB0;
extern char* g_sfCurPath;
extern u16 g_sfCfgB12;
extern char g_sfMusic4[];
extern char g_sfLocal4[];
extern char g_sfMusic[];
extern char g_sfLocal[];
extern char g_sfDir[];
#endif // GRUNTZ_GRUNTZ_SOUNDFONTPATH_H
