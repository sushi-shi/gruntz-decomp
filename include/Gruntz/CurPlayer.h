// CurPlayer.h - the current local-player / world index.
//
// g_curPlayer (_g_curPlayer @0x244c54) is the index of the local player's world/side,
// read tree-wide (trigger ownership, status bar, HUD, net sync). Its ONE definition +
// DATA pin live in src/Gruntz/SBI_RectOnly.cpp (an `extern "C"` datum, so its C-linkage
// name is reloc-masked). Declared once here (a minimal owner header) so consumers stop
// re-`extern`-ing it per-TU and the headers that used to re-declare it (Grunt.h,
// InGameIcon.h, TriggerMgrViews.h) share it.
#ifndef INCLUDE_GRUNTZ_CURPLAYER_H
#define INCLUDE_GRUNTZ_CURPLAYER_H
#include <Ints.h>

extern "C" i32 g_curPlayer; // 0x244c54 (defined in SBI_RectOnly.cpp)

#endif // INCLUDE_GRUNTZ_CURPLAYER_H
