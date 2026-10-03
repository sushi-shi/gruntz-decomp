#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/Fonts.h>

#include <Font/Font.h>
#include <Gruntz/GruntDirStatics.h>

Font g_tinyFont;

Font g_largeFont;

FontRenderer g_textObj;

Font g_mediumFont;

Font g_smallFont;

b32 g_loadedFlag = false;

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

        g_loadedFlag = true;
    }
    return 1;
}

i32 FreeFontsMemory() {
    g_largeFont.FreeMemory();
    g_mediumFont.FreeMemory();
    g_smallFont.FreeMemory();
    g_tinyFont.FreeMemory();
    return 1;
}
