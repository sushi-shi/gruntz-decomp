// Fonts.cpp - InitializeFonts, the one-shot bitmap-font loader.
//
//   InitializeFonts @0x115810 (163 B) - a free function that loads the four
//     global bitmap Font instances (large/medium/small/tiny .fnt) via the matched
//     Font::LoadFont (@0x179830, font unit), each from a by-value CString temp
//     built from the .fnt literal. Gated by a load-once flag @0x64eb14 (returns 1
//     immediately if set); a 0 LoadFont result aborts the chain (returns the 0).
//     Once all four load, the flag is latched and 1 returned.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Built /O2 /MT (NO /GX): the target carries no
// fs:0 EH frame - the by-value CString argument's ownership transfers to the
// callee (Font::LoadFont takes it by value), so InitializeFonts has no temp to
// unwind and emits no exception frame.
// ---------------------------------------------------------------------------
#include "../Font/Font.h"

// The four global Font instances + the load-once flag + the four .fnt file-name
// literals, addressed by fixed VA so the loads reloc-mask against the matched
// Font::LoadFont (@0x179830) and the CString literal-ctor (@0x1b9d4c).
#define g_loadedFlag (*(int *)0x64eb14)
#define g_largeFont  (*(Font *)0x64eac0)
#define g_mediumFont (*(Font *)0x64eae8)
#define g_smallFont  (*(Font *)0x64eb00)
#define g_tinyFont   (*(Font *)0x64ea58)

#define s_large_fnt  ((const char *)0x6152c4)   // "large.fnt"
#define s_medium_fnt ((const char *)0x6152b4)   // "medium.fnt"
#define s_small_fnt  ((const char *)0x6152a8)   // "small.fnt"
#define s_tiny_fnt   ((const char *)0x61529c)   // "tiny.fnt"

// ---------------------------------------------------------------------------
// InitializeFonts @0x115810
// One-shot load of the four bitmap fonts. Each Font::LoadFont takes a CString by
// value (a stack temp constructed from the literal); a 0 return aborts the load
// (returns the 0 the failed LoadFont left in eax). Once all four load, the flag
// is set + 1 returned.
// ---------------------------------------------------------------------------
// @address: 0x115810
// @size:    0xa3
int InitializeFonts()
{
    // The already-loaded case + the all-loaded success case share the single
    // `return 1` tail (the target's `jne <tail>` + the immediate flag store, not
    // an eax-routed one): the flag-set lives INSIDE the not-yet-loaded block so it
    // emits `mov dword[flag],1` (eax not yet live) before the shared `mov eax,1`.
    if (!g_loadedFlag) {
        if (!g_largeFont.LoadFont(s_large_fnt))
            return 0;
        if (!g_mediumFont.LoadFont(s_medium_fnt))
            return 0;
        if (!g_smallFont.LoadFont(s_small_fnt))
            return 0;
        if (!g_tinyFont.LoadFont(s_tiny_fnt))
            return 0;

        g_loadedFlag = 1;
    }
    return 1;
}
