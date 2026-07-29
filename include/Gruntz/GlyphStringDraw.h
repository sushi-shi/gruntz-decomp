// GlyphStringDraw.h - the GlyphStringDraw.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <Ints.h>

class CDDrawSurfaceMgr;
class CDDSurface;
class CString;
struct tagRECT;
typedef tagRECT RECT;
// (sink, text, box, fontSel, ...): HudMsgPush's body pins the order - arg2 is the
// CString (null-checked then drawn), arg3 the RECT (CopyRect source), arg4 the
// 0x64..0x82 font selector. NOTE HudMsgPush @0x115930 is the SAME RVA as
// EngStr_RenderText (<Wap32/EngStr.h>) - one retail function under two names - so the
// trailing six slots are its (fontSel, shadow, r, g, b, flag).
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
); // 0x1154b0
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
); // 0x115520 (present-page twin)

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
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
); // 0x115930 (== EngStr_RenderText - one retail body, two declared names)

#endif // GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
