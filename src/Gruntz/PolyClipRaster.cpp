// PolyClipRaster.cpp - the rotated-image polygon clip + rasterize entry (0x146550).
// __cdecl, called from CFileImage::DecodeRun24 (0x140c50) and the rotated-blit
// transform setup ImageRotateBlit (0x145f60). Sutherland-Hodgman clips the source
// quad (`verts`, n vertices of seven floats) against an axis-aligned box - four
// passes (x>=lo, x<lo+w, y>=lo, y<lo+h) ping-ponging between two global vertex
// scratch buffers (g_clipA / g_clipB) - then, if the clipped polygon is non-empty,
// hands it to the span rasterizer at 0x146a20. When the clip arg is -1 the box
// defaults to the source image's own w/h (img->m_18/m_1c). Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <rva.h>

// A clip vertex: x,y plus interpolated attributes (28-byte stride). The clip lerps
// indices 1..3 (y + two attrs); the clipped axis takes the boundary value exactly.
struct ClipVtx {
    float x, y, a, b, c, d, e;
};

// The source image (clip-default box): width @+0x18, height @+0x1c.
struct ClipImg {
    char p00[0x18];
    i32 m_width;  // +0x18
    i32 m_height; // +0x1c
};

// The two ping-pong clip scratch buffers. The -28 "guard" slot before each
// (g_clipA's at 0x6a16ec, g_clipB's at 0x6a21dc) is the prev-vertex base
// (&buf[count-1]); only their addresses (reloc-masked) are load-bearing.
DATA(0x002a1708)
extern ClipVtx g_clipA[]; // 0x6a1708
DATA(0x002a21f8)
extern ClipVtx g_clipB[]; // 0x6a21f8

// The span rasterizer the clipped polygon is handed to.
extern "C" void RasterSpans(ClipVtx* poly, i32 n, i32 a3, i32 a4, i32 a5, i32 a6); // 0x146a20

// @early-stop
// x87 scheduling wall (~25-35%, complete + correct): the four-plane
// Sutherland-Hodgman structure, the ping-pong buffers, the /28 vertex-count magic
// divide and the rasterizer hand-off are faithful, but the per-edge crossing
// interpolation `(bound - P)/(C - P)` lerp is dense fld/fxch/fdivp/fmulp x87 whose
// schedule MSVC does not expose to source ordering (same wall as the sibling
// ImageRotateBlit 0x145f60 and CFaderRadial::Build). Retail inlines all four clip
// passes; this reconstruction does too, to keep the body shape.
RVA(0x00146550, 0x4ca)
i32 RotateRasterize(
    ClipVtx* verts,
    i32 n,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 clipFlag,
    i32 clipB,
    i32 clipC,
    i32 clipD
) {
    float bound0, clip0, clip1, clip2;
    if (clipFlag == -1) {
        ClipImg* img = (ClipImg*)(void*)a3;
        clip1 = 0.0f;
        clip0 = (float)img->m_height;
        clip2 = (float)img->m_width;
        bound0 = 0.0f;
    } else {
        bound0 = (float)clipFlag;
        clip0 = (float)clipB;
        clip1 = (float)clipC;
        clip2 = (float)clipD;
    }

    // Pass 1: clip x >= bound0   (verts -> g_clipA)
    ClipVtx* out = g_clipA;
    if (n > 0) {
        ClipVtx* prev = &verts[n - 1];
        ClipVtx* cur = verts;
        i32 j = n;
        do {
            if (prev->x >= bound0) {
                *out++ = *prev;
            }
            if ((prev->x >= bound0) != (cur->x >= bound0)) {
                float t = (bound0 - prev->x) / (cur->x - prev->x);
                out->x = bound0;
                out->y = prev->y + (cur->y - prev->y) * t;
                out->a = prev->a + (cur->a - prev->a) * t;
                out->b = prev->b + (cur->b - prev->b) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = (i32)(out - g_clipA);
    if (n == 0) {
        return 0;
    }

    // Pass 2: clip x < clip0   (g_clipA -> g_clipB)
    out = g_clipB;
    if (n > 0) {
        ClipVtx* prev = &g_clipA[n - 1];
        ClipVtx* cur = g_clipA;
        i32 j = n;
        do {
            if (cur->x < clip0) {
                *out++ = *cur;
            }
            if ((cur->x < clip0) != (prev->x < clip0)) {
                float t = (clip0 - cur->x) / (prev->x - cur->x);
                out->x = clip0;
                out->y = cur->y + (prev->y - cur->y) * t;
                out->a = cur->a + (prev->a - cur->a) * t;
                out->b = cur->b + (prev->b - cur->b) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = (i32)(out - g_clipB);
    if (n == 0) {
        return 0;
    }

    // Pass 3: clip y >= clip1   (g_clipB -> g_clipA)
    out = g_clipA;
    if (n > 0) {
        ClipVtx* prev = &g_clipB[n - 1];
        ClipVtx* cur = g_clipB;
        i32 j = n;
        do {
            if (cur->y >= clip1) {
                *out++ = *cur;
            }
            if ((cur->y >= clip1) != (prev->y >= clip1)) {
                float t = (clip1 - cur->y) / (prev->y - cur->y);
                out->y = clip1;
                out->x = cur->x + (prev->x - cur->x) * t;
                out->a = cur->a + (prev->a - cur->a) * t;
                out->b = cur->b + (prev->b - cur->b) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = (i32)(out - g_clipA);
    if (n == 0) {
        return 0;
    }

    // Pass 4: clip y < clip2   (g_clipA -> g_clipB)
    out = g_clipB;
    if (n > 0) {
        ClipVtx* prev = &g_clipA[n - 1];
        ClipVtx* cur = g_clipA;
        i32 j = n;
        do {
            if (cur->y < clip2) {
                *out++ = *cur;
            }
            if ((cur->y < clip2) != (prev->y < clip2)) {
                float t = (clip2 - cur->y) / (prev->y - cur->y);
                out->y = clip2;
                out->x = cur->x + (prev->x - cur->x) * t;
                out->a = cur->a + (prev->a - cur->a) * t;
                out->b = cur->b + (prev->b - cur->b) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = (i32)(out - g_clipB);
    if (n == 0) {
        return 0;
    }

    RasterSpans(g_clipB, n, a4, a4, a5, a6);
    return 1;
}
