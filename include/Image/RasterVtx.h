// RasterVtx.h - the shared rotate/warp/wall rasterizer vertex + spine entry decls.
// One 28-byte raster vertex (ClipVtx) is shared by the whole poly-raster spine over
// the same two ping-pong scratch buffers (g_rasterVtxA @0x6a1708 / g_rasterVtxB
// @0x6a21f8):
//   * RotateRasterize   @0x146550 (PolyClipRaster.cpp) - rotate-blit clip+rasterize
//   * WarpTextureBlit    @0x146a20 (WarpTextureBlit.cpp) - textured span rasterizer
//   * ImagePolyClipRect  @0x1461b0 (ImagePolyClip.cpp)   - wall-quad 4-edge clipper
//   * FillPolygon        @0x146fe0 (DDrawPolyFill.cpp)   - wall-quad scanline fill
// and their transform-setup callers (ImageRotateBlit @0x145f60, ImageRotate.cpp;
// ProjectWallQuad @0x1471d0, WallProject.cpp). Field names are placeholders; the 0x1c
// stride + offsets are the load-bearing fact. (The duplicate .cpp-local PolyVtx /
// FillVert / WarpVtx views were folded onto this one struct.)
#ifndef GRUNTZ_IMAGE_RASTERVTX_H
#define GRUNTZ_IMAGE_RASTERVTX_H

#include <Ints.h>

// The held DirectDraw surface wrapper (FillPolygon/WarpTextureBlit dst/src). Full
// definition in <DDrawMgr/DDSurface.h>; a forward decl keeps the DDraw/OLE chain out
// of this widely-included header.
class CDDSurface;

// A clip/raster vertex (28-byte stride): x,y the screen position; u,v the source
// texel coordinate (both lerped through the Sutherland-Hodgman clip); c,d,e the
// carried-verbatim extra attributes. The inside-vertex copy moves the whole record
// (rep movs, 7 dwords).
struct ClipVtx {
    float x, y, u, v, c, d, e;
};

// The two ping-pong clip/raster scratch buffers + the published clipped-vertex count.
// The owner DATA() bindings live in ImagePolyClip.cpp; every raster TU shares these
// decls. The -28 "guard" slot before each buffer (g_rasterVtxA's @0x6a16ec,
// g_rasterVtxB's @0x6a21dc) is the prev-vertex base (&buf[count-1]).
extern "C" ClipVtx g_rasterVtxA[]; // 0x6a1708
extern "C" ClipVtx g_rasterVtxB[]; // 0x6a21f8
extern "C" i32 g_rasterVtxCount;   // 0x6becf8 (published by ImagePolyClipRect)

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

// The poly-raster spine entries (all __cdecl free fns, reloc-masked across TUs).

// The rotated-image polygon clip + rasterize entry (0x146550). Sutherland-Hodgman
// clips `verts` (n vertices) against a box, then hands the result to the span
// rasterizer. When clipFlag is -1 the box defaults to the source image's own w/h.
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

// The textured-polygon span rasterizer (0x146a20): scanline-fills the clipped quad
// into `dst`, sampling `src` (texel u,v walked per span). mode/colorkey select the
// per-pixel copy/skip-zero/skip-colorkey inner loop.
i32 WarpTextureBlit(ClipVtx* va, i32 n, CDDSurface* dst, CDDSurface* src, i32 mode, i32 colorkey); // 0x146a20

// The wall-quad 4-edge clipper (0x1461b0): four inlined Sutherland-Hodgman passes
// clip `poly` (n verts) to the rect [a2..a4] x [a3..a5], ping-ponging bufA<->bufB,
// publishing the surviving count to g_rasterVtxCount. Returns 0 the moment a pass
// empties the polygon, else 1.
i32 ImagePolyClipRect(ClipVtx* poly, i32 n, i32 a2, i32 a3, i32 a4, i32 a5); // 0x1461b0

// The wall-quad scanline polygon fill (0x146fe0): rasterizes the clipped polygon
// into `surf` with the flat 16-bit `color`.
i32 FillPolygon(ClipVtx* verts, i32 count, CDDSurface* surf, i16 color); // 0x146fe0

#endif
