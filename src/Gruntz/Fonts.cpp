#include <Font/Font.h>
#include <Gruntz/Fonts.h> // the shared InitializeFonts decl (Run boot-calls it)
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
i32 g_loadedFlag = 0; // 0x24eb14  Fonts::EnsureLoaded once-gate

RVA_COMPGEN(0x00115590, 0xa, _$E1136016)
RVA_COMPGEN(0x001155b0, 0xa, _$E1136048)
RVA_COMPGEN(0x001155d0, 0xe, _$E1136080)
RVA_COMPGEN(0x001155f0, 0xa, _$E1136112)
RVA_COMPGEN(0x00115610, 0xa, _$E1136144)
RVA_COMPGEN(0x00115630, 0xa, _$E1136176)
RVA_COMPGEN(0x00115650, 0xe, _$E1136208)
RVA_COMPGEN(0x00115670, 0xa, _$E1136240)
RVA_COMPGEN(0x00115690, 0xa, _$E1136272)
RVA_COMPGEN(0x001156b0, 0xa, _$E1136304)
RVA_COMPGEN(0x001156d0, 0xe, _$E1136336)
RVA_COMPGEN(0x001156f0, 0xa, _$E1136368)
RVA_COMPGEN(0x00115710, 0xa, _$E1136400)
RVA_COMPGEN(0x00115730, 0xa, _$E1136432)
RVA_COMPGEN(0x00115750, 0xe, _$E1136464)
RVA_COMPGEN(0x00115770, 0xa, _$E1136496)
RVA_COMPGEN(0x00115790, 0xa, _$E1136528)
RVA_COMPGEN(0x001157b0, 0xa, _$E1136560)
RVA_COMPGEN(0x001157d0, 0xe, _$E1136592)
RVA_COMPGEN(0x001157f0, 0xa, _$E1136624)

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
