#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <Ints.h>

class CDDrawSurfaceMgr;
class CDDSurface;
class CString;
struct tagRECT;
typedef tagRECT RECT;

void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);
void ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

extern "C" void HudMsgPush(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    CDDSurface* surf,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

#endif // GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
