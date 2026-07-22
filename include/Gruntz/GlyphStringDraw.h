// GlyphStringDraw.h - the GlyphStringDraw.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <Ints.h>

class CDDrawSurfaceMgr;
class CDDSurface;
class CString;
struct tagRECT;
typedef tagRECT RECT;
void ShowHudMessage(CDDrawSurfaceMgr* sink, RECT* box, CString* text, i32 a, i32 b, i32 c,
                    i32 d, i32 e, i32 f); // 0x1154b0


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void HudMsgPush(
    CDDrawSurfaceMgr* sink,
    i32 a2,
    i32 a3,
    CDDSurface* surf,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
); // 0x115930

#endif // GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
