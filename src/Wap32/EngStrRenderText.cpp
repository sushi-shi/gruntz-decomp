#include <StdAfx.h>

#include <rva.h>

#include <Font/Font.h>
#include <Font/FontSel.h>
#include <Ints.h>
#include <Wap32/EngStr.h>

RVA(0x00115930, 0x18f)
i32 EngStr_RenderText(
    void* worldContext,
    CString* text,
    RECT* bounds,
    CDDSurface* drawSurface,
    i32 fontSelection,
    i32 drawShadow,
    i32 red,
    i32 green,
    i32 blue,
    i32 centerText
) {
    if (worldContext == NULL) {
        return 0;
    }
    if (text == NULL) {
        return 0;
    }
    if (bounds == NULL) {
        return 0;
    }
    if (drawSurface == NULL) {
        return 0;
    }
    FontSel selectedFont = static_cast<FontSel>(fontSelection);
    switch (selectedFont) {
        case FONTSEL_LARGE:
            g_textRenderer.SetFont(&g_largeFont);
            break;
        case FONTSEL_MEDIUM:
            g_textRenderer.SetFont(&g_mediumFont);
            break;
        case FONTSEL_SMALL:
            g_textRenderer.SetFont(&g_smallFont);
            break;
        case FONTSEL_TINY:
            g_textRenderer.SetFont(&g_tinyFont);
            break;
    }
    CString* textToDraw = text;
    RECT* textBounds = bounds;
    CRect shadowBounds;
    if (drawShadow) {
        shadowBounds.CopyRect(textBounds);
        shadowBounds.OffsetRect(ENGSTR_SHADOW_OFFSET_X_PX, ENGSTR_SHADOW_OFFSET_Y_PX);
        g_textRenderer.SetColor(ENGSTR_SHADOW_COLOR);

        g_textRenderer.DrawWrapped(*textToDraw, drawSurface, shadowBounds, 1, centerText, 0);
    }
    g_textRenderer.SetColor(RGB(red, green, blue));
    g_textRenderer.DrawWrapped(*textToDraw, drawSurface, *textBounds, 1, centerText, 0);
    return 1;
}

RVA_COMPGEN(0x00115b30, 0x15, ??0CRect@@QAE@ABUtagRECT@@@Z)
