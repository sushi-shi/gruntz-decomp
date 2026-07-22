// GlyphStringDraw.h - the GlyphStringDraw.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
#define GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H

#include <Ints.h>

class CDDrawSurfaceMgr;
class CString;
struct tagRECT;
typedef tagRECT RECT;
void ShowHudMessage(CDDrawSurfaceMgr* sink, RECT* box, CString* text, i32 a, i32 b, i32 c,
                    i32 d, i32 e, i32 f); // 0x1154b0

#endif // GRUNTZ_GRUNTZ_GLYPHSTRINGDRAW_H
