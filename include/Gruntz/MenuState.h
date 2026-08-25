#ifndef GRUNTZ_GRUNTZ_MENUSTATE_H_H
#define GRUNTZ_GRUNTZ_MENUSTATE_H_H

#include <Gruntz/GameMode.h>
#include <Ints.h>

i32 DrawTextToOverlaySurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

#endif // GRUNTZ_GRUNTZ_MENUSTATE_H_H
