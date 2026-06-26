// FaderRadial.cpp - the radial distance-field fader init (0x17fa40). __thiscall.
// Resolves the source surface + palette from the config arg, allocates a
// width*height*16 cell buffer, computes the center + max radius, then fills each
// cell with sqrt(dx^2+dy^2)-derived fade values (sampling the source pixel
// through the surface's GetRowBase + post-plot notifier when in bounds). The
// embedded radial math is heavy x87 (fild/fsqrt + the constant pool at
// 0x5f0828/30/38/40). Field names are placeholders; offsets + code bytes are the
// load-bearing fact.
#include <Ints.h>
#include <rva.h>

extern "C" int FrSqrtFn(double v); // 0x11f570  (cdecl-ish post-sqrt step)

// The source config arg.
struct FrConfig {
    char p0[0x4];
    void* m_04; // +0x04
    void* m_08; // +0x08
    char p0c[0x10 - 0xc];
    void* m_10; // +0x10  (-> m_0c surface)
    void* m_14; // +0x14
};

// The source surface (this->m_38): dims at +0x18/+0x1c, GetRowBase @0x13e6d0,
// pixel buffer at +0x08, row pitch at +0x20, post-plot notifier @[+8] slot 0x80.
struct FrSurface {
    i32 GetRowBase(i32 row); // 0x13e6d0  __thiscall
    char p0[0x8];
    void* m_08; // +0x08
    char p0c[0x18 - 0xc];
    i32 m_18;   // +0x18  width
    i32 m_1c;   // +0x1c  height
    char p20[0x24 - 0x20];
    i32 m_20;   // +0x20  row pitch
    char p24[0xb0 - 0x24];
    i32 m_b0;   // +0xb0  column stride
};

// The 16-byte fade cell.
struct FrCell {
    i32 a, b, c, d;
};

struct CFaderRadial {
    i32 Build(FrConfig* cfg);              // 0x17fa40
    i32 BuildSurface(i32 a, i32 b, i32 c); // 0x14e830  (this+0x4 helper)
    char p0[0x1c];
    void* m_1c; // +0x1c  resolved surface handle
    char p20[0x24 - 0x20];
    void* m_24; // +0x24  default surface
    void* m_28; // +0x28  default palette
    char p2c[0x30 - 0x2c];
    i32 m_30;   // +0x30  owns-surface flag
    char p34[0x38 - 0x34];
    FrSurface* m_38; // +0x38  source surface
    void* m_3c;      // +0x3c  resolved palette
    char p40[0x44 - 0x40];
    i32 m_44;   // +0x44  max-radius scale
    char p48[0x4c - 0x48];
    float m_4c;   // +0x4c  radius->fade divisor
    FrCell* m_50; // +0x50  cell buffer
    i32 m_54;   // +0x54  center x
    i32 m_58;   // +0x58  center y
};

// @early-stop
// x87 scheduling wall: the surface/palette resolution + the radial cell loop are
// faithful, but MSVC's fild/fxch/fsqrt interleave over the per-cell distance math
// is not source-steerable (~30-40%).
RVA(0x0017fa40, 0x1f3)
i32 CFaderRadial::Build(FrConfig* cfg) {
    CFaderRadial* self = this;
    if (cfg->m_04 == 0)
        self->m_3c = self->m_24;
    else
        self->m_3c = cfg->m_04;

    if (cfg->m_08 == 0)
        self->m_38 = (FrSurface*)self->m_28;
    else
        self->m_38 = (FrSurface*)cfg->m_08;

    if (cfg->m_14 == 0) {
        self->m_1c = (void*)self->BuildSurface(*(i32*)((char*)cfg->m_10 + 0xc), 0x10, 0);
        self->m_30 = 1;
    } else {
        self->m_1c = cfg->m_14;
        self->m_30 = 0;
    }
    if (self->m_1c == 0)
        return 0;

    FrSurface* s = self->m_38;
    self->m_4c = (float)s->m_1c * 1.0; // K(0x5f0828)
    self->m_54 = s->m_1c / 2;
    self->m_58 = s->m_18 / 2;
    self->m_50 = (FrCell*)::operator new(s->m_18 * s->m_1c * 16);

    i32 cx = self->m_54;
    i32 cy = self->m_58;
    self->m_44 = FrSqrtFn((double)(cx * cx + cy * cy));

    for (i32 y = 0; y < s->m_18; y++) {
        for (i32 x = 0; x < s->m_1c; x++) {
            i32 dx = x - self->m_54;
            i32 dy = 0 - self->m_58; // (y term folded into the per-row base)
            float r = (float)self->m_44 - (float)FrSqrtFn((double)(dx * dx + dy * dy));
            float fade = r / self->m_4c;
            i32 vx = (i32)((float)dx * fade);
            i32 vy = (i32)((float)dy * fade);
            u8 pix;
            i32 rb = self->m_38->GetRowBase(y);
            if (rb != 0) {
                FrSurface* ss = self->m_38;
                i32 col = ss->m_b0 * x;
                i32 rowbase = ss->m_20 * y;
                pix = ((u8*)ss->m_08)[rowbase + rb + col];
                (*(void (**)(void*, i32))(*(void***)ss->m_08 + 0x80 / 4))(ss->m_08, 0);
            } else {
                pix = 0;
            }
            FrCell* cell = &self->m_50[y * self->m_38->m_1c + x];
            cell->a = vx;
            cell->b = vy;
            cell->c = (i32)fade;
            cell->d = pix;
        }
    }
    return 1;
}
