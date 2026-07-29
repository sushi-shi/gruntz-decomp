// WallProject.h - the WallProject.cpp TU's exported constants.
#ifndef GRUNTZ_DDRAWMGR_WALLPROJECT_H
#define GRUNTZ_DDRAWMGR_WALLPROJECT_H

#include <Ints.h>

extern const float g_c10; // 0x001efb10 (0.0f)
extern const float g_c20; // 0x001efb20 (0.5f)

extern float g_c24;

class CDDSurface;
// Project a wall segment (x0,y0)-(x1,y1) of half-width `halfWidth` into the four-vertex
// raster workspace, clip it to (clipLeft,clipTop,clipRight,clipBottom) and fill it in
// `color`. Slot map read off retail 0x1471d0: [esp+0x8..0x2c] = p1..p10 in order, the
// ImagePolyClipRect call at 0x147354 pushes [+0x20]/[+0x24]/[+0x28]/[+0x2c].
i32 ProjectWallQuad(
    CDDSurface* surface,
    i32 x0,
    i32 y0,
    i32 x1,
    i32 y1,
    i32 halfWidth,
    i32 color,
    i32 clipLeft,
    i32 clipTop,
    i32 clipRight,
    i32 clipBottom
);
#endif // GRUNTZ_DDRAWMGR_WALLPROJECT_H
