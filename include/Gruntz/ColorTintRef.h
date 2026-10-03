#ifndef GRUNTZ_GRUNTZ_COLORTINTREF_H
#define GRUNTZ_GRUNTZ_COLORTINTREF_H

#include <Gruntz/ColorTint.h>

inline COLORREF TintColorRef(ColorTint tint) {
    switch (tint) {
        case TINT_DKBLUE:
            return RGB(0, 0, 128);
        case TINT_DKGREEN:
            return RGB(0, 128, 0);
        case TINT_TURQ:
            return RGB(0, 128, 128);
        case TINT_DKRED:
            return RGB(128, 0, 0);
        case TINT_PURPLE:
            return RGB(128, 0, 128);
        case TINT_DKYELLOW:
            return RGB(128, 128, 0);
        case TINT_GREY:
            return RGB(128, 128, 128);
        case TINT_BLUE:
            return RGB(0, 0, 255);
        case TINT_GREEN:
            return RGB(0, 255, 0);
        case TINT_CYAN:
            return RGB(0, 255, 255);
        case TINT_RED:
            return RGB(255, 0, 0);
        case TINT_PINK:
            return RGB(255, 0, 255);
        case TINT_YELLOW:
            return RGB(255, 255, 0);
        case TINT_WHITE:
            return RGB(255, 255, 255);
        case TINT_ORANGE:
            return RGB(255, 128, 0);
        case TINT_HOTPINK:
            return RGB(255, 0, 128);
        case TINT_BLACK:
        default:
            return RGB(0, 0, 0);
    }
}

#endif
