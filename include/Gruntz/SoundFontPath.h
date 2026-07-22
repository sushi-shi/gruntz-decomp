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

#endif // GRUNTZ_GRUNTZ_SOUNDFONTPATH_H
