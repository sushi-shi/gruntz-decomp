#include <Wap32/EngStr.h> // own extern surface
#include <Mfc.h> // real MFC CString (copy ctor 0x1b9ba3) + windows.h (RECT/CopyRect/OffsetRect)
#include <Ints.h>
#include <rva.h>
#include <Font/Font.h> // canonical FontRenderer + CRect (RenderText IS DrawWrapped @0x17a460)

// @early-stop
RVA(0x00115930, 0x15b)
i32 EngStr_RenderText(
    void* self,
    CString* text,
    RECT* dst,
    CDDSurface* drawSurface,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    if (self == 0) {
        return 0;
    }
    if (text == 0) {
        return 0;
    }
    if (dst == 0) {
        return 0;
    }
    if (drawSurface == 0) {
        return 0;
    }
    switch (fontSel) {
        case 100:
            g_textObj.SetFont(&g_tinyFont);
            break;
        case 110:
            g_textObj.SetFont(&g_smallFont);
            break;
        case 120:
            g_textObj.SetFont(&g_mediumFont);
            break;
        case 130:
            g_textObj.SetFont(&g_largeFont);
            break;
    }
    CString* str = text;
    RECT* rc = dst;
    if (shadow) {
        RECT sh;
        CopyRect(&sh, rc);
        OffsetRect(&sh, 2, 3);
        g_textObj.SetColor(0);
        // the shadow pass reinterprets the local RECT as a CRect lvalue so the trivial
        // copy ctor inlines (4-mov copy of sh); the main pass below CALLs the 0x115b30
        // operator= to build its rect.
        g_textObj.DrawWrapped(*str, drawSurface, *static_cast<CRect*>(&sh), 1, flag, 0);
    }
    g_textObj.SetColor(((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff));
    CRect rect;
    rect = *rc; // 0x115b30 CRect::operator=(const tagRECT&) (the "Copy" reloc)
    g_textObj.DrawWrapped(*str, drawSurface, rect, 1, flag, 0);
    return 1;
}
