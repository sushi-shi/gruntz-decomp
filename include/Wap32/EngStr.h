#ifndef GRUNTZ_WAP32_ENGSTR_H
#define GRUNTZ_WAP32_ENGSTR_H

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(EngStrLayout)
    ENGSTR_SHADOW_COLOR = 0,
    ENGSTR_SHADOW_OFFSET_X_PX = 2,
    ENGSTR_SHADOW_OFFSET_Y_PX = 3
GZ_ENUM_CONST_END(EngStrLayout)

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
