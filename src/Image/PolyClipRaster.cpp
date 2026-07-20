#include <Ints.h>
#include <Image/RasterVtx.h> // ClipVtx + RotateRasterize decl (shared with ImageRotate.cpp)
#include <rva.h>

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
        RotateSrcImage* img = static_cast<RotateSrcImage*>(reinterpret_cast<void*>(a3));
        clip1 = 0.0f;
        clip0 = static_cast<float>(img->m_1c);
        clip2 = static_cast<float>(img->m_18);
        bound0 = 0.0f;
    } else {
        bound0 = static_cast<float>(clipFlag);
        clip0 = static_cast<float>(clipB);
        clip1 = static_cast<float>(clipC);
        clip2 = static_cast<float>(clipD);
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
                out->u = prev->u + (cur->u - prev->u) * t;
                out->v = prev->v + (cur->v - prev->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxA));
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
                out->u = cur->u + (prev->u - cur->u) * t;
                out->v = cur->v + (prev->v - cur->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxB));
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
                out->u = cur->u + (prev->u - cur->u) * t;
                out->v = cur->v + (prev->v - cur->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxA));
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
                out->u = cur->u + (prev->u - cur->u) * t;
                out->v = cur->v + (prev->v - cur->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxB));
    if (n == 0) {
        return 0;
    }

    WarpTextureBlit(g_rasterVtxB, n, reinterpret_cast<CDDSurface*>(a4), reinterpret_cast<CDDSurface*>(a4), a5, a6);
    return 1;
}
SIZE(ClipVtx, 0x1c);
