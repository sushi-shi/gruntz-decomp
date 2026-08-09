#ifndef GRUNTZ_DDRAWMGR_WALLPROJECT_H
#define GRUNTZ_DDRAWMGR_WALLPROJECT_H

#include <Mfc.h>

#include <Ints.h>

extern const float g_c10;
extern const float g_c20;

extern const float g_c24;

class CDDSurface;

i32 ProjectWallQuad(
    CDDSurface* surface,
    i32 x0,
    i32 y0,
    i32 x1,
    i32 y1,
    i32 halfWidth,
    i16 color,
    RECT clip
);
#endif // GRUNTZ_DDRAWMGR_WALLPROJECT_H
