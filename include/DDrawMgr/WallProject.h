// WallProject.h - the WallProject.cpp TU's exported constants.
#ifndef GRUNTZ_DDRAWMGR_WALLPROJECT_H
#define GRUNTZ_DDRAWMGR_WALLPROJECT_H

#include <Ints.h>

extern const float g_c10; // 0x001efb10 (0.0f)
extern const float g_c20; // 0x001efb20 (0.5f)

extern float g_c24;

class CDDSurface;
i32 ProjectWallQuad(
    CDDSurface* surface,
    i32 p1,
    i32 p2,
    i32 p3,
    i32 p4,
    i32 p5,
    i32 p6,
    i32 p7,
    i32 p8,
    i32 p9,
    i32 p10
);
#endif // GRUNTZ_DDRAWMGR_WALLPROJECT_H
