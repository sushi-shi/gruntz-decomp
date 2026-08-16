#ifndef GRUNTZ_WAP32_ENGSTR_H
#define GRUNTZ_WAP32_ENGSTR_H

#include <Ints.h>

class CDDrawSurfaceMgr;

i32 EngStr_RenderText(
    void* self,
    class CString* text,
    struct tagRECT* dst,
    class CDDSurface* drawSurface,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

i32 EngStr_DrawText(
    CDDrawSurfaceMgr* obj,
    class CString* text,
    struct tagRECT* dst,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

class FontRenderer;
extern FontRenderer g_textObj;

#endif // GRUNTZ_WAP32_ENGSTR_H
