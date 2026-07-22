#include <Ints.h>
#include <DDrawMgr/ColorHsv.h> // the shared ColorHSV record + RgbToHsv decl
#include <rva.h>

#define HSV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HSV_MIN(a, b) ((a) < (b) ? (a) : (b))

// @early-stop
// regalloc + x87-schedule wall (~46.7%): body logic + min/max macro + channel
// extraction are faithful. The u16 `lo` local steers b1 toward the word-sized
// extraction retail uses, but MSVC5 still picks `mov dl,ch` where retail emits
// `mov cx,bx; shr cx,8`, pins `color` in ecx (retail: ebx), and lays a 0xc-byte
// frame (retail 0x10 - a 4-byte gap for retail's early `(float)mn` fild/fstp
// materialization). None is source-steerable; verified base-vs-target with
// `gruntz sema disasm 0x0014fcc0 --diff`. docs/patterns/zero-register-pinning.md.
RVA(0x0014fcc0, 0x16d)
ColorHSV* RgbToHsv(ColorHSV* out, u32 color) {
    u16 lo = static_cast<u16>(color);
    u8 b0 = static_cast<u8>(lo);
    u8 b1 = static_cast<u8>((lo >> 8));
    u8 b2 = static_cast<u8>((color >> 16));

    int mx = HSV_MAX(HSV_MAX(b0, b1), b2);
    int mn = HSV_MIN(HSV_MIN(b0, b1), b2);

    float v = static_cast<float>(mx);
    float h, s;
    if (v == 0.0) {
        s = 0.0;
        h = 0.0;
    } else {
        float delta = v - static_cast<float>(mn);
        s = delta / v;
        if (delta == 0.0) {
            h = 0.0f;
        } else if (b0 == mx) {
            h = static_cast<float>((b1 - b2)) / delta;
        } else if (b1 == mx) {
            h = static_cast<float>((b2 - b0)) / delta - 2.0f;
        } else {
            h = static_cast<float>((b0 - b1)) / delta - 4.0f;
        }
        h = h * 60.0f;
        if (h < 0.0) {
            h = h - -360.0f;
        }
    }
    out->h = h;
    out->s = s;
    out->v = v;
    return out;
}

