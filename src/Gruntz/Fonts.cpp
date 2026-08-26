#include <rva.h>

#include <Gruntz/Fonts.h>

#include <Font/Font.h>
#include <Gruntz/GruntDirStatics.h>

RVA_DYNINIT(0x00115710, 0xa, g_tinyFont)
RVA_DYNINIT(0x00115730, 0xa, g_tinyFont)
RVA_DYNINIT(0x00115750, 0xe, g_tinyFont)
RVA_DYNINIT(0x00115770, 0xa, g_tinyFont)
DATA(0x0024ea58)
Font g_tinyFont;
RVA_DYNINIT(0x00115590, 0xa, g_largeFont)
RVA_DYNINIT(0x001155b0, 0xa, g_largeFont)
RVA_DYNINIT(0x001155d0, 0xe, g_largeFont)
RVA_DYNINIT(0x001155f0, 0xa, g_largeFont)
DATA(0x0024eac0)
Font g_largeFont;
RVA_DYNINIT(0x00115790, 0xa, g_textObj)
RVA_DYNINIT(0x001157b0, 0xa, g_textObj)
RVA_DYNINIT(0x001157d0, 0xe, g_textObj)
RVA_DYNINIT(0x001157f0, 0xa, g_textObj)
DATA(0x0024ead8)
FontRenderer g_textObj;
RVA_DYNINIT(0x00115610, 0xa, g_mediumFont)
RVA_DYNINIT(0x00115630, 0xa, g_mediumFont)
RVA_DYNINIT(0x00115650, 0xe, g_mediumFont)
RVA_DYNINIT(0x00115670, 0xa, g_mediumFont)
DATA(0x0024eae8)
Font g_mediumFont;
RVA_DYNINIT(0x00115690, 0xa, g_smallFont)
RVA_DYNINIT(0x001156b0, 0xa, g_smallFont)
RVA_DYNINIT(0x001156d0, 0xe, g_smallFont)
RVA_DYNINIT(0x001156f0, 0xa, g_smallFont)
DATA(0x0024eb00)
Font g_smallFont;
DATA(0x0024eb14)
b32 g_loadedFlag = false;

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

        g_loadedFlag = true;
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
