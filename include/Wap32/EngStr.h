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
    void* worldContext,
    class CString* text,
    struct tagRECT* bounds,
    class CDDSurface* drawSurface,
    i32 fontSelection,
    i32 drawShadow,
    i32 red,
    i32 green,
    i32 blue,
    i32 centerText
);

i32 DrawTextToFrontSurface(
    CDDrawSurfaceMgr* surfaceMgr,
    class CString* text,
    struct tagRECT* bounds,
    i32 fontSelection,
    i32 drawShadow,
    i32 red,
    i32 green,
    i32 blue,
    i32 centerText
);

class FontRenderer;
extern FontRenderer g_textRenderer;

#endif // GRUNTZ_WAP32_ENGSTR_H
