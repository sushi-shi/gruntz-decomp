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
    void* m_paletteArg; // +0x04
    void* m_surfaceArg; // +0x08
    char p0c[0x10 - 0xc];
    void* m_imageSrc;        // +0x10  (-> m_0c surface)
    void* m_providedSurface; // +0x14
};

// The source surface (this->m_srcSurface): dims at +0x18/+0x1c, GetRowBase @0x13e6d0,
// pixel buffer at +0x08, row pitch at +0x20, post-plot notifier @[+8] slot 0x80.
struct FrSurface {
    i32 GetRowBase(i32 row); // 0x13e6d0  __thiscall
    char p0[0x8];
    void* m_pixels; // +0x08  pixel buffer
    char p0c[0x18 - 0xc];
    i32 m_width;  // +0x18  width
    i32 m_height; // +0x1c  height
    char p20[0x24 - 0x20];
    i32 m_pitch; // +0x20  row pitch
    char p24[0xb0 - 0x24];
    i32 m_colStride; // +0xb0  column stride
};

// The 16-byte fade cell.
struct FrCell {
    i32 vx, vy, fade, pixel;
};

struct CFaderRadial {
    i32 Build(FrConfig* cfg);              // 0x17fa40
    i32 BuildSurface(i32 a, i32 b, i32 c); // 0x14e830  (this+0x4 helper)
    char p0[0x1c];
    void* m_workSurface; // +0x1c  resolved surface handle
    char p20[0x24 - 0x20];
    void* m_defaultPalette; // +0x24  default palette
    void* m_defaultSurface; // +0x28  default surface
    char p2c[0x30 - 0x2c];
    i32 m_ownsSurface; // +0x30  owns-surface flag
    char p34[0x38 - 0x34];
    FrSurface* m_srcSurface; // +0x38  source surface
    void* m_palette;         // +0x3c  resolved palette
    char p40[0x44 - 0x40];
    i32 m_maxRadius; // +0x44  max-radius scale
    char p48[0x4c - 0x48];
    float m_fadeDivisor; // +0x4c  radius->fade divisor
    FrCell* m_cells;     // +0x50  cell buffer
    i32 m_centerX;       // +0x54  center x
    i32 m_centerY;       // +0x58  center y
};

// @early-stop
// x87 scheduling wall: the surface/palette resolution + the radial cell loop are
// faithful, but MSVC's fild/fxch/fsqrt interleave over the per-cell distance math
// is not source-steerable (~30-40%).
RVA(0x0017fa40, 0x1f3)
i32 CFaderRadial::Build(FrConfig* cfg) {
    CFaderRadial* self = this;
    if (cfg->m_paletteArg == 0) {
        self->m_palette = self->m_defaultPalette;
    } else {
        self->m_palette = cfg->m_paletteArg;
    }

    if (cfg->m_surfaceArg == 0) {
        self->m_srcSurface = (FrSurface*)self->m_defaultSurface;
    } else {
        self->m_srcSurface = (FrSurface*)cfg->m_surfaceArg;
    }

    if (cfg->m_providedSurface == 0) {
        self->m_workSurface =
            (void*)self->BuildSurface(*(i32*)((char*)cfg->m_imageSrc + 0xc), 0x10, 0);
        self->m_ownsSurface = 1;
    } else {
        self->m_workSurface = cfg->m_providedSurface;
        self->m_ownsSurface = 0;
    }
    if (self->m_workSurface == 0) {
        return 0;
    }

    FrSurface* s = self->m_srcSurface;
    self->m_fadeDivisor = (float)s->m_height * 1.0; // K(0x5f0828)
    self->m_centerX = s->m_height / 2;
    self->m_centerY = s->m_width / 2;
    self->m_cells = (FrCell*)::operator new(s->m_width * s->m_height * 16);

    i32 cx = self->m_centerX;
    i32 cy = self->m_centerY;
    self->m_maxRadius = FrSqrtFn((double)(cx * cx + cy * cy));

    for (i32 y = 0; y < s->m_width; y++) {
        for (i32 x = 0; x < s->m_height; x++) {
            i32 dx = x - self->m_centerX;
            i32 dy = 0 - self->m_centerY; // (y term folded into the per-row base)
            float r = (float)self->m_maxRadius - (float)FrSqrtFn((double)(dx * dx + dy * dy));
            float fade = r / self->m_fadeDivisor;
            i32 vx = (i32)((float)dx * fade);
            i32 vy = (i32)((float)dy * fade);
            u8 pix;
            i32 rb = self->m_srcSurface->GetRowBase(y);
            if (rb != 0) {
                FrSurface* ss = self->m_srcSurface;
                i32 col = ss->m_colStride * x;
                i32 rowbase = ss->m_pitch * y;
                pix = ((u8*)ss->m_pixels)[rowbase + rb + col];
                (*(void (**)(void*, i32))(*(void***)ss->m_pixels + 0x80 / 4))(ss->m_pixels, 0);
            } else {
                pix = 0;
            }
            FrCell* cell = &self->m_cells[y * self->m_srcSurface->m_height + x];
            cell->vx = vx;
            cell->vy = vy;
            cell->fade = (i32)fade;
            cell->pixel = pix;
        }
    }
    return 1;
}

SIZE_UNKNOWN(FrConfig);
SIZE_UNKNOWN(FrSurface);
SIZE_UNKNOWN(FrCell);
SIZE_UNKNOWN(CFaderRadial);
