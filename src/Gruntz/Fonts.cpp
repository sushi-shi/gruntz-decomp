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
#include <Globals.h>

// The four global Font instances + the load-once flag + the four .fnt file-name
// literals, addressed by fixed VA so the loads reloc-mask against the matched
// Font::LoadFont and the CString literal-ctor.
DATA(0x0024eac0)
Font g_largeFont;
DATA(0x0024eae8)
Font g_mediumFont;
DATA(0x0024eb00)
Font g_smallFont;
DATA(0x0024ea58)
Font g_tinyFont;
// The POD load-once gate, DEFINED here (owner TU); a plain `extern` stays in Globals.h.
DATA(0x0024eb14)
i32 g_loadedFlag = 0; // 0x24eb14  Fonts::EnsureLoaded once-gate

#define s_large_fnt "large.fnt"
#define s_medium_fnt "medium.fnt"
#define s_small_fnt "small.fnt"
#define s_tiny_fnt "tiny.fnt"

// ---------------------------------------------------------------------------
// The compiler-generated dynamic initializer for the g_mediumFont global (the
// _initterm entry): construct it in place via the explicit-ctor-call tail-jmp
// (mov ecx,&g_mediumFont; jmp ??0Font@@QAE@XZ - no placement-new null-guard).
// Folded here from Stub/ReconBatch2.cpp (g_mediumFont is this TU's font global).
// @interleaver Forward_115630 (g_mediumFont dyn-init) emitted-in <boundary: WapMisc
// Unmatched_1155b0/crt ___inittime @0x1155b0 (before) + crt ___inittime @0x115650
// (after)>. A dyn-init COMDAT the /Gy linker placed in the init region, not this TU block.
RVA(0x00115630, 0xa)
void Forward_115630() {
    g_mediumFont.Font::Font();
}

// The sibling dynamic initializer for the g_tinyFont global (same explicit-ctor-call
// tail-jmp). Re-homed from src/Stub/BoundaryLowerThunks.cpp (was FontForward115730).
// @interleaver Forward_115730 (g_tinyFont dyn-init) emitted-in <boundary: crt thunk
// Init_smallFont/___inittime @0x1156b0 (before) + crt ___inittime @0x115750 (after)>. A
// dyn-init COMDAT the /Gy linker placed in the init region, not this TU block.
RVA(0x00115730, 0xa)
void Forward_115730() {
    g_tinyFont.Font::Font();
}

// The global text renderer g_textObj (0x64ead8) - a FontRenderer (the stateful render
// shim, ctor ??0FontRenderer@@QAE@XZ 0x179be0; DIFFERENT type from the bitmap Font
// globals above). DEFINED here (owner TU: this TU holds its dynamic initializer, the
// explicit-ctor-call tail-jmp below). It was the address-minted placeholder
// `g_font64ead8`; the real name comes from its only consumer, EngStr_RenderText
// (src/Wap32/EngStrRenderText.cpp), which drives it as the engine's one text-render
// object (SetFont per size class, SetColor, then the wrapped draw).
DATA(0x0024ead8)
FontRenderer g_textObj;
// @interleaver Forward_1157b0 (g_textObj FontRenderer dyn-init) emitted-in <boundary:
// crt ___inittime @0x1156d0 (before) + crt ___inittime @0x1157d0 (after)>. A dyn-init
// COMDAT the /Gy linker placed in the init region, not this TU block.
RVA(0x001157b0, 0xa)
void Forward_1157b0() {
    g_textObj.FontRenderer::FontRenderer();
}

// ---------------------------------------------------------------------------
// One-shot load of the four bitmap fonts. Each Font::LoadFont takes a CString by
// value (a stack temp constructed from the literal); a 0 return aborts the load
// (returns the 0 the failed LoadFont left in eax). Once all four load, the flag
// is set + 1 returned.
RVA(0x00115810, 0xa3)
i32 InitializeFonts() {
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

RVA(0x001158f0, 0x2e)
i32 FreeFontsMemory() {
    g_largeFont.FreeMemory();
    g_mediumFont.FreeMemory();
    g_smallFont.FreeMemory();
    g_tinyFont.FreeMemory();
    return 1;
}
