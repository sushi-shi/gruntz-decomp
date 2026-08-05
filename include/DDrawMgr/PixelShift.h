#ifndef INCLUDE_DDRAWMGR_PIXELSHIFT_H
#define INCLUDE_DDRAWMGR_PIXELSHIFT_H

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(PixelFormat16Constants)
    PIXEL8_BYTES_PER_PIXEL = 1,
    PIXEL16_BYTES_PER_PIXEL = 2,
    PIXEL24_BYTES_PER_PIXEL = 3,
    PIXEL32_BYTES_PER_PIXEL = 4,
    PIXEL16_RED_DOWN = 3,
    PIXEL16_BLUE_DOWN = 3,
    PIXEL16_GREEN_UP = 5,
    RGB555_GREEN_DOWN = 3,
    RGB555_RED_UP = 10,
    RGB555_CHANNEL_BITS = 5,
    RGB565_GREEN_DOWN = 2,
    RGB565_RED_UP = 11,
    RGB565_GREEN_BITS = 6
GZ_ENUM_CONST_END(PixelFormat16Constants)

extern i32 g_rUp;
extern i32 g_gUp;
extern i32 g_bUp;
extern i32 g_rDown;
extern i32 g_gDown;
extern i32 g_bDown;

#endif // INCLUDE_DDRAWMGR_PIXELSHIFT_H
