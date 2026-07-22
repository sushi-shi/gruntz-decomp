#ifndef GRUNTZ_WAP32_ENGSTR_H
#define GRUNTZ_WAP32_ENGSTR_H

#include <Ints.h>

class CDDrawSurfaceMgr;

extern "C" i32 EngStr_RenderText(
    void* self,
    i32 a1,
    i32 a2,
    class CDDSurface* drawSurface,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

void EngStr_DrawText(
    CDDrawSurfaceMgr* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
class FontRenderer; // <Font/Font.h>
extern FontRenderer g_textObj;

#endif // GRUNTZ_WAP32_ENGSTR_H
