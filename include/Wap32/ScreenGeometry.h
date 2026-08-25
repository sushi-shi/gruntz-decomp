#ifndef WAP32_SCREENGEOMETRY_H
#define WAP32_SCREENGEOMETRY_H

#include <Enums.h>

// The fixed 640x480 display the game runs at, and the 320x240 half of it used
// for save-game preview thumbnails.
//
// The full size is stated outright where the surface is created -
// `world->Init(hwnd, SCREEN_W_PX, SCREEN_H_PX, 0x10, flags)` - and every other
// site agrees with it as a bound: the attract-mode spawner puts a sprite on the
// bottom edge with `*outY = SCREEN_H_PX`, the credits scroller clips to
// `{0, 0, SCREEN_W_PX, SCREEN_H_PX}`, and the Booty screen's bars all run the
// full width.
//
// The half size is pinned by the save-game preview block the loader reads back:
// 0x3843a bytes is 320 * 240 * 3 plus a BITMAPINFOHEADER, its 4-byte tail and
// a 14-byte BITMAPFILEHEADER.
GZ_ENUM_CONST_BEGIN(ScreenGeometry)
    SCREEN_W_PX = 0x280,
    SCREEN_H_PX = 0x1e0,
    SCREEN_HALF_W_PX = 0x140,
    SCREEN_HALF_H_PX = 0xf0
GZ_ENUM_CONST_END(ScreenGeometry)

#endif // WAP32_SCREENGEOMETRY_H
