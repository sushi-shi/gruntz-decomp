// PolyClipRaster.cpp - the rotated-image polygon clip + rasterize entry (0x146550).
// __cdecl, called from CFileImage::DecodeRun24 (0x140c50) and the rotated-blit
// transform setup ImageRotateBlit (0x145f60). Sutherland-Hodgman clips the source
// quad (`verts`, n vertices of seven floats) against an axis-aligned box - four
// passes (x>=lo, x<lo+w, y>=lo, y<lo+h) ping-ponging between two global vertex
// scratch buffers (g_rasterVtxA / g_rasterVtxB) - then, if the clipped polygon is non-empty,
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
    i32 m_18; // +0x18 width
    i32 m_1c; // +0x1c height
};

// The two ping-pong clip scratch buffers. The -28 "guard" slot before each
// (g_rasterVtxA's at 0x6a16ec, g_rasterVtxB's at 0x6a21dc) is the prev-vertex base
// (&buf[count-1]); only their addresses (reloc-masked) are load-bearing.
DATA(0x002a1708)
extern "C" ClipVtx g_rasterVtxA[]; // 0x6a1708
DATA(0x002a21f8)
extern "C" ClipVtx g_rasterVtxB[]; // 0x6a21f8

// The span rasterizer the clipped polygon is handed to: the textured-polygon
// rasterizer WarpTextureBlit (0x146a20, WarpTextureBlit.cpp). ClipVtx == WarpVtx
// (both the 28-byte {x,y,+attrs} raster vertex); a4 is the dst/src surface.
struct WarpVtx;
class CDDSurface;
i32 WarpTextureBlit(WarpVtx* va, i32 n, CDDSurface* dst, CDDSurface* src, i32 mode, i32 colorkey); // 0x146a20

// @early-stop
// x87 scheduling wall (~53%, complete + correct; RE-PROVEN 2026-07-05 - the old
// "25-35%" note was stale). Dominant residual confirmed via --diff: retail's per-edge
// crossing lerp RE-COMPUTES the division (cur->x - prev->x) and reloads cur->x/prev->x
// for EACH of the three interpolated attributes (x87 stack pressure won't hold `t`),
// emitting its own fld/fld/fsub/fxch/fsub/fdivp/fmulp/fadd interleave per attribute;
// this build computes `t` once and reuses it (fdivp once + fmul st,st(1)). MSVC5 does
// not expose that x87 recompute/fxch schedule to source ordering - same wall as the
// sibling ImageRotateBlit 0x145f60 and CFaderRadial::Build. Secondary steerable diffs
// (the clip-crossing `(A>=B)!=(C>=B)` materializes a 0/1 bool where retail re-branches
// on fcom; the clipFlag `-1` compares against a memory operand not an imm) are below
// the x87 ceiling and not worth chasing per the x87 re-prove-and-move doctrine.
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
        clip0 = (float)img->m_1c;
        clip2 = (float)img->m_18;
        bound0 = 0.0f;
    } else {
        bound0 = (float)clipFlag;
        clip0 = (float)clipB;
        clip1 = (float)clipC;
        clip2 = (float)clipD;
    }

    // Pass 1: clip x >= bound0   (verts -> g_rasterVtxA)
    ClipVtx* out = g_rasterVtxA;
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
    n = (i32)(out - g_rasterVtxA);
    if (n == 0) {
        return 0;
    }

    // Pass 2: clip x < clip0   (g_rasterVtxA -> g_rasterVtxB)
    out = g_rasterVtxB;
    if (n > 0) {
        ClipVtx* prev = &g_rasterVtxA[n - 1];
        ClipVtx* cur = g_rasterVtxA;
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
    n = (i32)(out - g_rasterVtxB);
    if (n == 0) {
        return 0;
    }

    // Pass 3: clip y >= clip1   (g_rasterVtxB -> g_rasterVtxA)
    out = g_rasterVtxA;
    if (n > 0) {
        ClipVtx* prev = &g_rasterVtxB[n - 1];
        ClipVtx* cur = g_rasterVtxB;
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
    n = (i32)(out - g_rasterVtxA);
    if (n == 0) {
        return 0;
    }

    // Pass 4: clip y < clip2   (g_rasterVtxA -> g_rasterVtxB)
    out = g_rasterVtxB;
    if (n > 0) {
        ClipVtx* prev = &g_rasterVtxA[n - 1];
        ClipVtx* cur = g_rasterVtxA;
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
    n = (i32)(out - g_rasterVtxB);
    if (n == 0) {
        return 0;
    }

    WarpTextureBlit((WarpVtx*)g_rasterVtxB, n, (CDDSurface*)a4, (CDDSurface*)a4, a5, a6);
    return 1;
}
SIZE_UNKNOWN(ClipImg);
SIZE_UNKNOWN(ClipVtx);
