#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <Ints.h>

class CGameWorld;
class CDDSurface;
class CString;
struct tagRECT;
typedef tagRECT RECT;

i32 DrawTextToOverlaySurface(
    CGameWorld* surfaceMgr,
    CString* text,
    RECT* bounds,
    i32 fontSelection,
    i32 drawShadow,
    i32 red,
    i32 green,
    i32 blue,
    i32 centerText
);
i32 DrawTextToBackSurface(
    CGameWorld* surfaceMgr,
    CString* text,
    RECT* bounds,
    i32 fontSelection,
    i32 drawShadow,
    i32 red,
    i32 green,
    i32 blue,
    i32 centerText
);

#endif // GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
