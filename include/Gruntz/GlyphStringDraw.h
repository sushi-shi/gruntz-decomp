#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <string>

#include <Ints.h>

class CDDrawSurfaceMgr;
class CDDSurface;

struct tagRECT;
typedef tagRECT RECT;

i32 DrawTextToOverlaySurface(
    CDDrawSurfaceMgr* surfaceMgr,
    std::string* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);
i32 DrawTextToBackSurface(
    CDDrawSurfaceMgr* surfaceMgr,
    std::string* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

#endif
