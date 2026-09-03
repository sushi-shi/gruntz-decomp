#ifndef DDRAWMGR_PIXELFORMATMACROS_H
#define DDRAWMGR_PIXELFORMATMACROS_H

#include <DDrawMgr/PixelShift.h>

inline b32 PixelFormatIsRgb555() {
    return g_rDown == PIXEL16_RED_DOWN && g_gDown == RGB555_GREEN_DOWN
           && g_bDown == PIXEL16_BLUE_DOWN && g_rUp == RGB555_RED_UP && g_gUp == PIXEL16_GREEN_UP;
}

#endif // DDRAWMGR_PIXELFORMATMACROS_H
