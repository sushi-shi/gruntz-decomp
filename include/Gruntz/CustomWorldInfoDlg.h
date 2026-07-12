// CustomWorldInfoDlg.h - shared types for the CUSTOM_WORLDINFO level-info popup
// (C:\Proj\Gruntz). The popup proc (CustomWorldInfoDlgProc @0x3b600) validates the
// selected .WWD through the world's level-info source and fills the four info items.
// Field names are placeholders; only the offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CUSTOMWORLDINFODLG_H
#define GRUNTZ_CUSTOMWORLDINFODLG_H

#include <rva.h>

// The world's level-info source: its IsValidWwd(path, outHeader) reads the .WWD
// header and returns 1 iff valid. The body at 0x160530 IGNORES `this` (reads only the
// two args), so it is byte-identical to a __stdcall free fn, yet the caller sets up
// ecx = the source object (thiscall-ignoring-this) - so it is a genuine __thiscall
// method. Defined in GameLevel.cpp (its retail .text neighbourhood).
struct WwdLevelInfoSrc {
    i32 IsValidWwd(const char* path, void* outHeader); // 0x160530 (def in GameLevel.cpp)
};
SIZE_UNKNOWN(WwdLevelInfoSrc);

// The world holder stashed by RunCustomWorldDialog (g_dat62c268 = the game world);
// its +0x24 is the level-info source the popup validates through.
struct WwdWorldHolder {
    char m_pad00[0x24];
    WwdLevelInfoSrc* m_24; // +0x24  level-info source
};
SIZE_UNKNOWN(WwdWorldHolder);

#endif // GRUNTZ_CUSTOMWORLDINFODLG_H
