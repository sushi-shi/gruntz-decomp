#ifndef GRUNTZ_GRUNTZ_COLORTINTREF_H
#define GRUNTZ_GRUNTZ_COLORTINTREF_H

#include <Mfc.h>

#include <Gruntz/ColorTint.h>

inline COLORREF TintColorRef(ColorTint tint) {
    switch (tint) {
        case TINT_DKBLUE:
            return 0x800000;
        case TINT_DKGREEN:
            return 0x008000;
        case TINT_TURQ:
            return 0x808000;
        case TINT_DKRED:
            return 0x000080;
        case TINT_PURPLE:
            return 0x800080;
        case TINT_DKYELLOW:
            return 0x008080;
        case TINT_GREY:
            return 0x808080;
        case TINT_BLUE:
            return 0xff0000;
        case TINT_GREEN:
            return 0x00ff00;
        case TINT_CYAN:
            return 0xffff00;
        case TINT_RED:
            return 0x0000ff;
        case TINT_PINK:
            return 0xff00ff;
        case TINT_YELLOW:
            return 0x00ffff;
        case TINT_WHITE:
            return 0xffffff;
        case TINT_ORANGE:
            return 0x0080ff;
        case TINT_HOTPINK:
            return 0x8000ff;
        case TINT_BLACK:
        default:
            return 0;
    }
}

#endif // GRUNTZ_GRUNTZ_COLORTINTREF_H
