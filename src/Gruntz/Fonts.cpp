// Fonts.cpp - InitializeFonts, the one-shot bitmap-font loader.
//
//   InitializeFonts - a free function that loads the four
//     global bitmap Font instances (large/medium/small/tiny .fnt) via the matched
//     Font::LoadFont (font unit), each from a by-value CString temp
//     built from the .fnt literal. Gated by a load-once flag (returns 1
//     immediately if set); a 0 LoadFont result aborts the chain (returns the 0).
//     Once all four load, the flag is latched and 1 returned.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Built /O2 /MT (NO /GX): the target carries no
// fs:0 EH frame - the by-value CString argument's ownership transfers to the
// callee (Font::LoadFont takes it by value), so InitializeFonts has no temp to
// unwind and emits no exception frame.
// ---------------------------------------------------------------------------
#include <Font/Font.h>
#include <rva.h>

// The four global Font instances + the load-once flag + the four .fnt file-name
// literals, addressed by fixed VA so the loads reloc-mask against the matched
// Font::LoadFont and the CString literal-ctor.
DATA(0x24eb14)
extern int g_loadedFlag;
DATA(0x24eac0)
extern Font g_largeFont;
DATA(0x24eae8)
extern Font g_mediumFont;
DATA(0x24eb00)
extern Font g_smallFont;
DATA(0x24ea58)
extern Font g_tinyFont;

#define s_large_fnt "large.fnt"
#define s_medium_fnt "medium.fnt"
#define s_small_fnt "small.fnt"
#define s_tiny_fnt "tiny.fnt"

// ---------------------------------------------------------------------------
// One-shot load of the four bitmap fonts. Each Font::LoadFont takes a CString by
// value (a stack temp constructed from the literal); a 0 return aborts the load
// (returns the 0 the failed LoadFont left in eax). Once all four load, the flag
// is set + 1 returned.
RVA(0x115810, 0xa3)
int InitializeFonts() {
    // The already-loaded case + the all-loaded success case share the single
    // `return 1` tail (the target's `jne <tail>` + the immediate flag store, not
    // an eax-routed one): the flag-set lives INSIDE the not-yet-loaded block so it
    // emits `mov dword[flag],1` (eax not yet live) before the shared `mov eax,1`.
    if (!g_loadedFlag) {
        if (!g_largeFont.LoadFont(s_large_fnt)) {
            return 0;
        }
        if (!g_mediumFont.LoadFont(s_medium_fnt)) {
            return 0;
        }
        if (!g_smallFont.LoadFont(s_small_fnt)) {
            return 0;
        }
        if (!g_tinyFont.LoadFont(s_tiny_fnt)) {
            return 0;
        }

        g_loadedFlag = 1;
    }
    return 1;
}

RVA(0x1158f0, 0x2e)
int FreeFontsMemory() {
    g_largeFont.FreeMemory();
    g_mediumFont.FreeMemory();
    g_smallFont.FreeMemory();
    g_tinyFont.FreeMemory();
    return 1;
}
