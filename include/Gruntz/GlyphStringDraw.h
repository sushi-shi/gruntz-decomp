// GlyphStringDraw.h - the GlyphStringDraw.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <Ints.h>

class CDDrawSurfaceMgr;
class CDDSurface;
class CString;
struct tagRECT;
typedef tagRECT RECT;
// (sink, text, box, fontSel, ...): HudMsgPush's body pins the order - a2 is the
// CString (null-checked then drawn), a3 the RECT (CopyRect source), a4 the
// 0x64..0x82 font selector.
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
); // 0x1154b0
void ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0x115520 (present-page twin)

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void HudMsgPush(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    CDDSurface* surf,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
); // 0x115930

#endif // GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
