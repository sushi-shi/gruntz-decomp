#include <rva.h>

#include <DDrawMgr/ColorHsv.h>

#include <Win32.h>

#include <Ints.h>

#define HSV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HSV_MIN(a, b) ((a) < (b) ? (a) : (b))

// @early-stop
RVA(0x0014fcc0, 0x16d)
ColorHSV* RgbToHsv(ColorHSV* out, u32 color) {
    ColorHSV hsv;
    u8 b0 = GetRValue(color);
    u8 b1 = GetGValue(color);
    u8 b2 = GetBValue(color);

    float v = static_cast<float>(HSV_MAX(HSV_MAX(b0, b1), b2));
    int mn = HSV_MIN(HSV_MIN(b0, b1), b2);
    float h;

    hsv.v = v;
    if (v == 0.0) {
        hsv.s = 0.0;
        hsv.h = 0.0;
    } else {
        float delta = v - static_cast<float>(mn);
        hsv.s = delta / v;
        if (delta == 0.0) {
            h = 0.0f;
        } else if (b0 == v) {
            h = (b1 - b2) / delta;
        } else if (b1 == v) {
            h = (b2 - b0) / delta - -2.0f;
        } else {
            h = (b0 - b1) / delta - -4.0f;
        }
        h = h * 60.0f;
        if (h < 0.0) {
            h = h - -360.0f;
        }
        hsv.h = h;
    }
    *out = hsv;
    return out;
}
