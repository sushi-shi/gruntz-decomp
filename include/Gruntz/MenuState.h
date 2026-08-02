#ifndef GRUNTZ_GRUNTZ_MENUSTATE_H_H
#define GRUNTZ_GRUNTZ_MENUSTATE_H_H

#include <Gruntz/GameMode.h>
#include <Ints.h>

extern "C" tagRECT g_versionRect;

void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
);

#endif // GRUNTZ_GRUNTZ_MENUSTATE_H_H
