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
DATA(0x0064eac0)
extern Font g_largeFont;
DATA(0x0064eae8)
extern Font g_mediumFont;
DATA(0x0064eb00)
extern Font g_smallFont;
DATA(0x0064ea58)
extern Font g_tinyFont;

// WapRect - the WAP32-LOCAL rect (16 bytes) the render worker takes by value, NOT
// MFC CRect: its ctor 0x115b30 (via the 0x37c4 ILT thunk) is in the Wap32/EngStr
// module range, does CopyRect(this,&src) then returns this - a Wap32 helper, not
// NAFXCW (which lives at 0x1b9xxx). Kept as a local view.
struct WapRect {
    i32 left, top, right, bottom;
    WapRect(const RECT& r); // 0x115b30 (Wap32-local, reloc-masked)
};

// The global text renderer g_textObj (DAT_0064ead8) is a FontRenderer (the WAP32
// font-render object; full class in <Font/Font.h>). Partial view: current font/color
// + the draw worker. SetFont/SetColor store m_font/m_color. All reloc-masked externs.
struct FontRenderer {
    Font* m_font;          // +0x00  current font
    i32 m_color;           // +0x04  current color
    void SetFont(Font* f); // 0x179c10
    void SetColor(i32 c);  // 0x179c20
    void RenderText(CString s, void* drawFn, WapRect r, i32 a, i32 b, i32 c); // 0x17a460
};
DATA(0x0064ead8)
extern FontRenderer g_textObj;

// @early-stop
// Complete + correct (~53%). Wall = by-value WapRect argument materialization: retail
// builds the render's by-value rect temp *transiently* at each call (inline 4-mov
// field copy in the shadow pass; the 0x37c4/0x115b30 CopyRect wrapper in the main
// pass) so its persistent frame is one 0x10 RECT; MSVC5 here instead calls the
// external WapRect(const RECT&) ctor and hoists a persistent 0x10 rect slot into the
// frame (sub esp,0x20 vs retail's 0x10), which shifts every stack-arg offset and
// cascades. No source spelling forces MSVC5 to inline the ctor (it is external) /
// build the temp transiently. The null-guard chain, the font-size byte-index jump
// table, the RGB(r,g,b) assembly, both SetColor/SetFont calls and the shadow
// CopyRect/OffsetRect all match; only the two by-value render-arg builds diverge.
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
        g_textObj.RenderText(*str, drawFn, WapRect(sh), 1, flag, 0);
    }
    g_textObj.SetColor(((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff));
    g_textObj.RenderText(*str, drawFn, WapRect(*rc), 1, flag, 0);
    return 1;
}

// Class metadata (annotate-only; the settled EngStr_RenderText body is untouched).
SIZE_UNKNOWN(WapRect);      // Wap32-local RECT view (0x115b30 ctor, not MFC)
SIZE_UNKNOWN(FontRenderer); // global FontRenderer view (current font/color)
