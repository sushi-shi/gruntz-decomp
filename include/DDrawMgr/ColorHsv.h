#ifndef GRUNTZ_DDRAWMGR_COLORHSV_H
#define GRUNTZ_DDRAWMGR_COLORHSV_H

#include <Ints.h>

#include <Ints.h>

struct ColorHSV {
    float m_h;
    float m_s;
    float m_v;
};

ColorHSV RgbToHsv(u32 color);

#endif
