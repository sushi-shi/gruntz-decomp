#ifndef GRUNTZ_DDRAWMGR_WALLPROJECT_H
#define GRUNTZ_DDRAWMGR_WALLPROJECT_H

#include <Ints.h>

extern const float g_c10;
extern const float g_c20;

extern float g_c24;

class CDDSurface;

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
