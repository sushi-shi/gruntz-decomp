#include <Font/Font.h>
#include <Gruntz/Fonts.h>
#include <rva.h>

DATA(0x0024eac0)
Font g_largeFont;
DATA(0x0024eae8)
Font g_mediumFont;
DATA(0x0024eb00)
Font g_smallFont;
DATA(0x0024ea58)
Font g_tinyFont;
DATA(0x0024ead8)
FontRenderer g_textObj;
DATA(0x0024eb14)
i32 g_loadedFlag = 0;

RVA(0x00115810, 0xa3)
i32 InitializeFonts() {

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
