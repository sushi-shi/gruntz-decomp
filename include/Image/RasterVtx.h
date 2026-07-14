// RasterVtx.h - the shared rotated/warp rasterizer vertex + entry declarations.
// The rotate rasterizer (RotateRasterize @0x146550, PolyClipRaster.cpp) and its
// transform-setup callers (ImageRotateBlit @0x145f60, ImageRotate.cpp; the
// CDDSurface Rotate/Scale blit thunks) share this 28-byte raster vertex. Field
// names are placeholders; the 0x1c stride + offsets are the load-bearing fact.
#ifndef GRUNTZ_IMAGE_RASTERVTX_H
#define GRUNTZ_IMAGE_RASTERVTX_H

#include <Ints.h>

// A clip/raster vertex: x,y plus interpolated attributes (28-byte stride). The
// Sutherland-Hodgman clip lerps indices 1..3 (y + two attrs); x,y are the screen
// position, a,b the source texel coordinate.
struct ClipVtx {
    float x, y, a, b, c, d, e;
};

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

#endif
