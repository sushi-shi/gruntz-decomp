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

extern "C" ClipVtx g_rasterVtxA[]; // 0x6a1708
extern "C" ClipVtx g_rasterVtxB[]; // 0x6a21f8
extern "C" i32 g_rasterVtxCount;   // 0x6becf8 (published by ImagePolyClipRect)
extern "C" i32 g_rasterDestRow; // 0x6a2ce8  current scanline base (engine scratch)
extern "C" i32 g_rasterDestPtr; // 0x6becf4  current span start (engine scratch)

// The rotate-blit SOURCE image geometry: the object RotateRasterize (clipFlag==-1)
// and ImageRotateBlit read the default clip box from - width @+0x18, height @+0x1c.
// Shared by both rasterizer TUs (was the duplicated .cpp-local ClipImg / ImgRect
// views). The concrete class of the rotate source is unrecovered (a3 flows opaquely
// from ImageRotateBlit's surface arg; its w/h @0x18/0x1c do NOT match CDDSurface's
// w/h @0x08/0x0c, so it is a distinct image representation); only the two offsets
// are load-bearing. @identity-TODO: pin the source-image class.
struct RotateSrcImage {
    char p00[0x18];
    i32 m_18; // +0x18  width
    i32 m_1c; // +0x1c  height
};
SIZE_UNKNOWN();

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
