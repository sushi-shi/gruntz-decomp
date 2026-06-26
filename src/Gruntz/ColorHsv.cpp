// ColorHsv.cpp - RgbToHsv (0x14fcc0): convert a packed 3-byte color (b0/b1/b2 =
// the low three bytes of the dword arg) to a {hue, sat, value} float triple
// written through the first (output) pointer arg. __cdecl. The integer max/min
// of the three channels lower via the classic non-CSE'ing `((a)>(b)?(a):(b))`
// MACRO (max(max(b0,b1),b2) re-evaluates the inner max), and the hue/sat are
// computed in x87 with single-precision constants (60 deg/sextant, the +2/+4
// sextant biases, and a -360 wrap). Field names are placeholders; only the
// byte layout + emitted code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

#define HSV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HSV_MIN(a, b) ((a) < (b) ? (a) : (b))

struct ColorHSV {
    float h;
    float s;
    float v;
};

// @early-stop
// x87 scheduling wall: the body logic + min/max macro are faithful, but MSVC's
// exact fld/fxch interleave of the channel compares with the FP pipeline is not
// source-steerable.
RVA(0x0014fcc0, 0x16d)
void RgbToHsv(ColorHSV* out, u32 color) {
    u8 b0 = (u8)color;
    u8 b1 = (u8)(color >> 8);
    u8 b2 = (u8)(color >> 16);

    int mx = HSV_MAX(HSV_MAX(b0, b1), b2);
    int mn = HSV_MIN(HSV_MIN(b0, b1), b2);

    float v = (float)mx;
    float h, s;
    if (v == 0.0) {
        s = 0.0;
        h = 0.0;
    } else {
        float delta = v - (float)mn;
        s = delta / v;
        if (delta == 0.0) {
            h = 0.0f;
        } else if (b0 == mx) {
            h = (float)(b1 - b2) / delta;
        } else if (b1 == mx) {
            h = (float)(b2 - b0) / delta - 2.0f;
        } else {
            h = (float)(b0 - b1) / delta - 4.0f;
        }
        h = h * 60.0f;
        if (h < 0.0)
            h = h - -360.0f;
    }
    out->h = h;
    out->s = s;
    out->v = v;
}
