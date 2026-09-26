#ifndef GRUNTZ_GRUNTZ_VIDEOMODEINLINE_H
#define GRUNTZ_GRUNTZ_VIDEOMODEINLINE_H

#include <Ints.h>
#include <Wap32/ScreenGeometry.h>

inline b32 IsStandardVideoMode(const SIZE& size) {
    return size.cx == SCREEN_W_PX && size.cy == SCREEN_H_PX;
}

#endif // GRUNTZ_GRUNTZ_VIDEOMODEINLINE_H
