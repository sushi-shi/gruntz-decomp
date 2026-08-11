#include <rva.h>

#include <Mfc.h>

#include <Font/Font.h>
#include <Font/FontSel.h>
#include <Ints.h>
#include <Wap32/EngStr.h>

RVA(0x00115930, 0x18f)
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
    if (self == NULL) {
        return 0;
    }
    if (text == NULL) {
        return 0;
    }
    if (dst == NULL) {
        return 0;
    }
    if (drawSurface == NULL) {
        return 0;
    }
    FontSel font = static_cast<FontSel>(fontSel);
    switch (font) {
        case FONTSEL_TINY:
            g_textObj.SetFont(&g_tinyFont);
            break;
        case FONTSEL_SMALL:
            g_textObj.SetFont(&g_smallFont);
            break;
        case FONTSEL_MEDIUM:
            g_textObj.SetFont(&g_mediumFont);
            break;
        case FONTSEL_LARGE:
            g_textObj.SetFont(&g_largeFont);
            break;
    }
    CString* str = text;
    RECT* rc = dst;
    CRect rect;
    if (shadow) {
        CopyRect(&rect, rc);
        OffsetRect(&rect, 2, 3);
        g_textObj.SetColor(0);

        g_textObj.DrawWrapped(*str, drawSurface, rect, 1, flag, 0);
    }
    g_textObj.SetColor(((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff));
    g_textObj.DrawWrapped(*str, drawSurface, *rc, 1, flag, 0);
    return 1;
}

// The CRect this TU's `DrawWrapped(..., *rc, ...)` by-value argument builds, and
// the COMDAT this TU wins. FLIRT called it `??4CRect@@QAEXABUtagRECT@@@Z`, but
// MFC's `operator=` returns void while these 21 bytes end `mov eax,esi` - they
// return `this`, so it is the converting CONSTRUCTOR. Byte-identical (modulo the
// __imp__CopyRect reloc) to engstrrendertext.obj's own COMDAT.
RVA_COMPGEN(0x00115b30, 0x15, ??0CRect@@QAE@ABUtagRECT@@@Z)
