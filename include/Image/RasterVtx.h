#ifndef GRUNTZ_IMAGE_RASTERVTX_H
#define GRUNTZ_IMAGE_RASTERVTX_H

#include <Ints.h>
#include <rva.h>

class CDDSurface;

struct ClipVtx {
    float x, y, u, v; // the clip passes interpolate these as floats
    // +0x10..+0x1b: the 14-bit FIXED-POINT (x,u,v) the edge tables interpolate per
    // scanline (the warp/fill rasters read/write ONLY these as i32; no float use of
    // the tail exists anywhere - grep-proven 2026-07-19; ex `float c,d,e`).
    i32 fx, fu, fv;
};
SIZE(0x1c);

// Extents proven by the gap to the next data symbol, each an exact multiple of
// sizeof(ClipVtx)==0x1c: the A/B clip buffers are 0xaf0 B = 100 each, and the
// L/R per-scanline edge tables are 0x1c000 B = 4096 each.
extern "C" ClipVtx g_rasterVtxA[100];   // 0x6a1708
extern "C" ClipVtx g_rasterVtxB[100];   // 0x6a21f8
extern "C" ClipVtx g_rasterEdgeR[4096]; // 0x6856f8  ascending-edge table (fill reads +0x10)
extern "C" ClipVtx g_rasterEdgeL[4096]; // 0x6a2cf0  descending-edge table
extern "C" i32 g_rasterVtxCount;   // 0x6becf8 (published by ImagePolyClipRect)
extern "C" u8* g_rasterDestRow;    // 0x6a2ce8  current scanline base (engine scratch)
extern "C" i16* g_rasterDestPtr;    // 0x6becf4  current span start (engine scratch)

// SETTLED 2026-07-27: both surface args are CDDSurface, and they are DISTINCT.
//  * `dest` - all three retail callers of ImageRotateBlit (CDDSurface::RotateBlit
//    0x141040 / ScaleBlit 0x141200 / RotateScaleBlit 0x141240) pass `this` into the
//    arg that lands here, and the clipFlag==-1 arm reads its +0x18/+0x1c, which ARE
//    CDDSurface::m_height/m_width (the embedded DDSURFACEDESC's dwHeight/dwWidth).
//  * `src`  - forwarded verbatim into WarpTextureBlit's `CDDSurface* src`, whose
//    WarpIsPow2 gate reads the same +0x1c (0x146a37).
// Retail's tail (0x1469ee) pushes [entry+0x0c] then [entry+0x10]: dest, then src -
// they are not the same pointer. The ex `RotateSrcImage` pad-view is dissolved.
i32 RotateRasterize(
    ClipVtx* verts,
    i32 n,
    CDDSurface* dest,
    CDDSurface* src,
    i32 a5,
    i32 a6,
    i32 clipFlag,
    i32 clipB,
    i32 clipC,
    i32 clipD
); // 0x146550

i32 WarpTextureBlit(
    ClipVtx* va,
    i32 n,
    CDDSurface* dst,
    CDDSurface* src,
    i32 mode,
    i32 colorkey
); // 0x146a20

i32 ImagePolyClipRect(ClipVtx* poly, i32 n, i32 a2, i32 a3, i32 a4, i32 a5); // 0x1461b0

i32 FillPolygon(ClipVtx* verts, i32 count, CDDSurface* surf, i16 color); // 0x146fe0

#endif
