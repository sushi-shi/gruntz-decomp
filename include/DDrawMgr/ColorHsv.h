// ColorHsv.h - the shared RGB<->HSV color record + the RgbToHsv converter decl.
// RgbToHsv (0x14fcc0, ColorHsv.cpp) fills a {hue,sat,value} float triple from a
// packed 3-byte color and RETURNS the out pointer (proven: the palette-hue sort in
// ShadeTableCache.cpp dereferences the result, `ha = *RgbToHsv(&tmp, ...)`). The
// ex-.cpp-local ColorHSV (ColorHsv.cpp) and Hsv (ShadeTableCache.cpp) views were the
// same 12-byte record; folded here.
#ifndef GRUNTZ_DDRAWMGR_COLORHSV_H
#define GRUNTZ_DDRAWMGR_COLORHSV_H

#include <Ints.h>

// A {hue, saturation, value} float triple (12 bytes). hue in degrees.
struct ColorHSV {
    float h;
    float s;
    float v;
};

// Convert a packed color (b<<16 | g<<8 | r, low 3 bytes of `color`) to HSV in `out`;
// returns `out`. __cdecl, reloc-masked across TUs. 0x14fcc0.
ColorHSV* RgbToHsv(ColorHSV* out, u32 color);

#endif // GRUNTZ_DDRAWMGR_COLORHSV_H
