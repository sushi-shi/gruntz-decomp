#ifndef GRUNTZ_DDRAWMGR_COLORHSV_H
#define GRUNTZ_DDRAWMGR_COLORHSV_H

#include <Ints.h>

struct ColorHSV {
    float h;
    float s;
    float v;
};

ColorHSV* RgbToHsv(ColorHSV* out, u32 color);

#endif // GRUNTZ_DDRAWMGR_COLORHSV_H
