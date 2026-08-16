#ifndef GRUNTZ_IMAGE_RASTERVTX_H
#define GRUNTZ_IMAGE_RASTERVTX_H

#include <rva.h>

#include <Ints.h>

class CDDSurface;

struct ClipVtx {
    float x, y, u, v;

    i32 fx, fu, fv;
};

extern ClipVtx g_rasterVtxA[100];
extern ClipVtx g_rasterVtxB[100];
extern ClipVtx g_rasterEdgeR[4096];
extern ClipVtx g_rasterEdgeL[4096];
extern i32 g_rasterVtxCount;
extern u8* g_rasterDestRow;
extern i16* g_rasterDestPtr;

i32 RotateRasterize(
    ClipVtx* verts,
    i32 n,
    CDDSurface* dst,
    CDDSurface* src,

    i32 mode,
    i32 colorkey,
    i32 clipFlag,
    i32 clipB,
    i32 clipC,
    i32 clipD
);

i32 WarpTextureBlit(ClipVtx* va, i32 n, CDDSurface* dst, CDDSurface* src, i32 mode, i32 colorkey);

i32 ImagePolyClipRect(
    ClipVtx* poly,
    i32 n,
    i32 clipLeft,
    i32 clipTop,
    i32 clipRight,
    i32 clipBottom
);

i32 FillPolygon(ClipVtx* verts, i32 count, CDDSurface* surf, i16 color);

#endif
