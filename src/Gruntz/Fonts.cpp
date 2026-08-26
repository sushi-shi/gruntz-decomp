#include <rva.h>

#include <Gruntz/Fonts.h>

#include <Font/Font.h>
#include <Gruntz/GruntDirStatics.h>

RVA_DYNINIT(0x00115840, 0xa, g_tinyFont)
RVA_DYNINIT(0x00115860, 0xa, g_tinyFont)
RVA_DYNINIT(0x00115880, 0xe, g_tinyFont)
RVA_DYNINIT(0x001158a0, 0xa, g_tinyFont)
DATA(0x0024f9b0)
Font g_tinyFont;
RVA_DYNINIT(0x001156c0, 0xa, g_largeFont)
RVA_DYNINIT(0x001156e0, 0xa, g_largeFont)
RVA_DYNINIT(0x00115700, 0xe, g_largeFont)
RVA_DYNINIT(0x00115720, 0xa, g_largeFont)
DATA(0x0024fa18)
Font g_largeFont;
RVA_DYNINIT(0x001158c0, 0xa, g_textObj)
RVA_DYNINIT(0x001158e0, 0xa, g_textObj)
RVA_DYNINIT(0x00115900, 0xe, g_textObj)
RVA_DYNINIT(0x00115920, 0xa, g_textObj)
DATA(0x0024fa30)
FontRenderer g_textObj;
RVA_DYNINIT(0x00115740, 0xa, g_mediumFont)
RVA_DYNINIT(0x00115760, 0xa, g_mediumFont)
RVA_DYNINIT(0x00115780, 0xe, g_mediumFont)
RVA_DYNINIT(0x001157a0, 0xa, g_mediumFont)
DATA(0x0024fa40)
Font g_mediumFont;
RVA_DYNINIT(0x001157c0, 0xa, g_smallFont)
RVA_DYNINIT(0x001157e0, 0xa, g_smallFont)
RVA_DYNINIT(0x00115800, 0xe, g_smallFont)
RVA_DYNINIT(0x00115820, 0xa, g_smallFont)
DATA(0x0024fa58)
Font g_smallFont;
DATA(0x0024fa6c)
b32 g_loadedFlag = false;

RVA(0x00115940, 0xa3)
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

RVA(0x00115a20, 0x2e)
i32 FreeFontsMemory() {
    g_largeFont.FreeMemory();
    g_mediumFont.FreeMemory();
    g_smallFont.FreeMemory();
    g_tinyFont.FreeMemory();
    return 1;
}
