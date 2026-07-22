#include <Font/Font.h>
#include <Gruntz/Fonts.h> // the shared InitializeFonts decl (Run boot-calls it)
#include <rva.h>

DATA(0x0024ea58)
Font g_tinyFont;
DATA(0x0024eac0)
Font g_largeFont;
DATA(0x0024eae8)
Font g_mediumFont;
DATA(0x0024eb00)
Font g_smallFont;
DATA(0x0024eb14)
i32 g_loadedFlag = 0; // 0x24eb14  Fonts::EnsureLoaded once-gate

RVA(0x00115630, 0xa)
void Forward_115630() {
    g_mediumFont.Font::Font();
}

RVA(0x00115730, 0xa)
void Forward_115730() {
    g_tinyFont.Font::Font();
}

DATA(0x0024ead8)
FontRenderer g_textObj;
RVA(0x001157b0, 0xa)
void Forward_1157b0() {
    g_textObj.FontRenderer::FontRenderer();
}

RVA(0x00115810, 0xa3)
i32 InitializeFonts() {
    // The already-loaded case + the all-loaded success case share the single
    // `return 1` tail (the target's `jne <tail>` + the immediate flag store, not
    // an eax-routed one): the flag-set lives INSIDE the not-yet-loaded block so it
    // emits `mov dword[flag],1` (eax not yet live) before the shared `mov eax,1`.
    if (!g_loadedFlag) {
        if (!g_largeFont.LoadFont("large.fnt")) {
            return 0;
        }
        if (!g_mediumFont.LoadFont("medium.fnt")) {
            return 0;
        }
        if (!g_smallFont.LoadFont("small.fnt")) {
            return 0;
        }
        if (!g_tinyFont.LoadFont("tiny.fnt")) {
            return 0;
        }

        g_loadedFlag = 1;
    }
    return 1;
}

RVA(0x001158f0, 0x2e)
i32 FreeFontsMemory() {
    g_largeFont.FreeMemory();
    g_mediumFont.FreeMemory();
    g_smallFont.FreeMemory();
    g_tinyFont.FreeMemory();
    return 1;
}
