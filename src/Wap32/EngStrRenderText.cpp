// EngStrRenderText.cpp - EngStr_RenderText (0x115930), the WAP32 text-render worker
// the EngStr_DrawText forwarder (src/Wap32/EngStr.cpp) tail-calls. Its true home is
// EngStr.cpp, but it is kept in its own unit so its size (+ its font-size jump table
// and the Win32 rect imports) do not perturb the already-matched zBitVec::SetSize /
// CContainerErr neighbours in EngStr.cpp (adding it there regressed SetSize ~13%).
//
// The worker: bails unless object/string/rect/draw-method are all present; picks the
// font by the size selector (100/110/120/130 -> tiny/small/medium/large); if the
// shadow flag is set, draws a black copy offset by (2,3); then draws the RGB(r,g,b)
// main pass. Each pass hands the render worker a by-value copy of the string + rect.
// Field NAMES are placeholders; offsets + call-site bytes load-bearing. NON-EH.
#include <Mfc.h> // real MFC CString (copy ctor 0x1b9ba3) + windows.h (RECT/CopyRect/OffsetRect)
#include <Ints.h>
#include <rva.h>

// The size-selected fonts (g_largeFont..g_tinyFont = VFont in retail). Opaque.
struct Font;
extern Font g_largeFont;
extern Font g_mediumFont;
extern Font g_smallFont;
extern Font g_tinyFont;

// WapRect - the WAP32-LOCAL rect (16 bytes) the render worker takes by value, NOT
// MFC CRect: its ctor 0x115b30 (via the 0x37c4 ILT thunk) is in the Wap32/EngStr
// module range, does CopyRect(this,&src) then returns this - a Wap32 helper, not
// NAFXCW (which lives at 0x1b9xxx). Kept as a local view.
struct WapRect {
    i32 left, top, right, bottom;
    WapRect(const RECT& r); // 0x115b30 (Wap32-local, reloc-masked); the compiler-
                            // generated trivial copy ctor is what the shadow pass uses.
};

// The global text renderer g_textObj (DAT_0064ead8) is THE canonical FontRenderer
// (<Font/Font.h>); it is DEFINED (with its DATA pin) in src/Gruntz/Fonts.cpp, the TU
// that holds its dynamic initializer. `class` (not struct) so the reference mangles
// ?g_textObj@@3VFontRenderer@@A - the one name bound at 0x24ead8; the old struct-key
// spelling emitted a second name (...@@3U...) that starved Fonts.cpp's reference.
//
// @identity-TODO (view debt): this is still a per-TU RE-SIGNATURE of the canonical class,
// not a distinct type. Its `RenderText(CString, void*, WapRect, i32,i32,i32)` IS
// FontRenderer::DrawWrapped @0x17a460, whose canonical Font.h declaration spells the
// same four rect words as four separate i32 params (x0/top/right/bottom). The retail
// CALLER builds that rect BY VALUE through the converting ctor @0x115b30 (the cracked
// @early-stop below depends on it), so dissolving this view onto <Font/Font.h> requires
// first re-spelling the canonical DrawWrapped to take the rect by value (and adding the
// real SetFont @0x179c10) - a Font.cpp/Font.h change, out of this lane's scope.
class FontRenderer {
public:
    Font* m_font;          // +0x00  current font
    i32 m_color;           // +0x04  current color
    void SetFont(Font* f); // 0x179c10
    void SetColor(i32 c);  // 0x179c20
    void RenderText(CString s, void* drawFn, WapRect r, i32 a, i32 b, i32 c); // 0x17a460
};
extern FontRenderer g_textObj;

// @early-stop
// WapRect-by-value wall CRACKED (53 -> 60): the two render-arg builds now match retail
// exactly. Retail SPLITS the two by-value WapRect builds - the shadow pass INLINES a
// 4-mov copy of the local sh, the main pass CALLs the converting ctor 0x115b30 (the
// "Copy" reloc). The trick: pass the shadow's rect as a WapRect lvalue (*(WapRect*)&sh)
// so the compiler-generated TRIVIAL COPY ctor inlines, while `WapRect(*rc)` in the main
// pass keeps the EXTERNAL converting ctor -> a call. That collapses the persistent rect
// slot, so the frame drops sub esp,0x20 -> 0x10 and every [esp+N] realigns; all 10
// callees (SetFont/CopyRect/OffsetRect/SetColor x2/CString x2/RenderText x2 + the
// WapRect Copy) now pair. Residual: the font-size sparse switch byte-index-table +
// jump-table are separate $L COMDATs (delinker-inline artifact, docs/patterns/
// switch-jumptable-separate-comdat.md) plus a 2-byte `add eax,-100` imm8-vs-imm32
// encoding in the switch prologue - not source-steerable.
RVA(0x00115930, 0x15b)
SYMBOL(_EngStr_RenderText)
extern "C" i32 EngStr_RenderText(
    void* self,
    i32 a1,
    i32 a2,
    void* drawFn,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    if (self == 0) {
        return 0;
    }
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (drawFn == 0) {
        return 0;
    }
    switch (fontSel) {
        case 100:
            g_textObj.SetFont(&g_tinyFont);
            break;
        case 110:
            g_textObj.SetFont(&g_smallFont);
            break;
        case 120:
            g_textObj.SetFont(&g_mediumFont);
            break;
        case 130:
            g_textObj.SetFont(&g_largeFont);
            break;
    }
    CString* str = (CString*)a1;
    RECT* rc = (RECT*)a2;
    if (shadow) {
        RECT sh;
        CopyRect(&sh, rc);
        OffsetRect(&sh, 2, 3);
        g_textObj.SetColor(0);
        // retail inlines the shadow's by-value rect build (4-mov copy of the local sh)
        // via the trivial copy ctor; the main pass below CALLs the converting ctor.
        g_textObj.RenderText(*str, drawFn, *(WapRect*)&sh, 1, flag, 0);
    }
    g_textObj.SetColor(((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff));
    g_textObj.RenderText(*str, drawFn, WapRect(*rc), 1, flag, 0);
    return 1;
}

// Class metadata (annotate-only; the settled EngStr_RenderText body is untouched).
SIZE_UNKNOWN(WapRect);      // Wap32-local RECT view (0x115b30 ctor, not MFC)
SIZE_UNKNOWN(FontRenderer); // global FontRenderer view (current font/color)
