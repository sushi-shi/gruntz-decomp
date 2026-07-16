// Fonts.h - the one-shot bitmap-font loader entry (owner: src/Gruntz/Fonts.cpp).
// CGruntzMgr::Run (RezSync.cpp) calls it during boot (via ILT thunk 0x2db0 ->
// 0x115810); returns 1 on success.
#ifndef GRUNTZ_GRUNTZ_FONTS_H
#define GRUNTZ_GRUNTZ_FONTS_H

#include <Ints.h>

i32 InitializeFonts(); // 0x115810

#endif // GRUNTZ_GRUNTZ_FONTS_H
