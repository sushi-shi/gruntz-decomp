#include <Ints.h>
#include <Win32.h>             // GetRValue/GetGValue/GetBValue - `color` is a COLORREF
#include <DDrawMgr/ColorHsv.h> // the shared ColorHSV record + RgbToHsv decl
#include <rva.h>

#define HSV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HSV_MIN(a, b) ((a) < (b) ? (a) : (b))

// The retail bytes disagree with an int-flavoured reading of this function in
// three places, and all three are behaviour, not codegen:
//
//  1. The sector bias is ADDED, not subtracted. `fsub ds:0x5efb6c` /
//     `fsub ds:0x5efb70` load -2.0f / -4.0f (0xc0000000 / 0xc0800000), so the
//     green and blue sectors are `+2` and `+4` - textbook HSV. Subtracting +2/+4
//     put the g-max sector at -120deg and the b-max sector at -240deg. objdiff
//     masks the constant's ADDRESS, so the wrong sign was invisible in the diff;
//     only reading the .rdata word shows it.
//  2. The "is this channel the max" tests are FLOAT compares against v
//     (`fild <chan>` then `fcomp [v]`), not integer compares against an int max.
//     The int `mx` is dead the moment `v = (float)mx` is materialized.
//  3. The hue numerator stays INTEGER and divides into delta with `fidivr`
//     (`sub ecx,edx` then `fidivr [tmp]`), so no (float) cast on `(b1 - b2)`.
//
// Two shape facts fall out with them: the channels come through the COLORREF
// macros (GetGValue's `(WORD)` cast is what makes retail's 16-bit
// `mov cx,bx; shr cx,8` instead of a `mov dl,ch` byte grab, and GetBValue is
// `mov edx,ebx; shr edx,0x10` verbatim), and the result is built in a local
// record copied out in one go - the epilogue's three consecutive dword moves out
// of three consecutive stack slots, with the v==0 arm storing integer zeros into
// two of them, is a struct assignment, not three FP stores.
//
// @early-stop
// The branch sequence now AGREES with retail (and the spurious second `ret` is
// gone - the v==0 arm joins the common store tail instead of owning an epilogue).
// Residual is a register-pressure split: retail keeps all three channels live in
// whole registers for the whole body (ebx = color, masked down to b0 in place;
// ecx = the 16-bit green shift; edx = color>>16) and masks with `and r,0xff` at
// each int use, so its frame is 0x10. cl narrows the three `u8` locals and spills
// b1/b2 to byte slots, taking the frame to 0x18, and grabs green as `mov al,ch`
// rather than GetGValue's 16-bit `mov cx,bx; shr cx,8`.
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
