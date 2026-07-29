#ifndef GRUNTZ_WAP32_ENGSTR_H
#define GRUNTZ_WAP32_ENGSTR_H

#include <Ints.h>

class CDDrawSurfaceMgr;

extern "C" i32 EngStr_RenderText(
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

// The trailing six are EngStr_RenderText's own (fontSel, shadow, r, g, b, flag) slots.
void EngStr_DrawText(
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

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
class FontRenderer; // <Font/Font.h>
extern FontRenderer g_textObj;

#endif // GRUNTZ_WAP32_ENGSTR_H
