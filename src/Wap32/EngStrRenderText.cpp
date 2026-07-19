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
#include <Font/Font.h> // canonical FontRenderer + CRect (RenderText IS DrawWrapped @0x17a460)

// The size-selected fonts (g_largeFont..g_tinyFont = VFont in retail). Defined in
// Fonts.cpp; declared in <Font/Font.h> (included above).

// The former WapRect view is DISSOLVED onto the canonical CRect (<Wap32/Rect.h>, pulled in
// by <Font/Font.h>): 16 bytes {left,top,right,bottom}. Its "0x115b30 converting ctor" is
// ??4CRect@@QAEAAU0@ABUtagRECT@@@Z == CRect::operator=(const tagRECT&) (Rect.h binds it), so
// the main pass builds a CRect then assigns (`CRect rect; rect = *rc;` -> the 0x115b30
// operator=) and the shadow pass reinterprets the local RECT as a CRect lvalue so the
// compiler-generated TRIVIAL COPY ctor inlines. The former per-TU FontRenderer re-signature
// is dissolved too: g_textObj is the canonical FontRenderer and RenderText IS DrawWrapped.

// The global text renderer g_textObj (0x24ead8) is THE canonical FontRenderer (<Font/Font.h>),
// DEFINED (with its DATA pin) in src/Gruntz/Fonts.cpp (the TU that holds its dynamic
// initializer). `class` FontRenderer -> the reference mangles ?g_textObj@@3VFontRenderer@@A,
// the one name bound at 0x24ead8.
extern FontRenderer g_textObj;

// @early-stop
// WapRect-by-value wall CRACKED (53 -> 60): the two render-arg builds match retail exactly.
// Retail SPLITS the two by-value rect builds - the shadow pass INLINES a 4-mov copy of the
// local sh, the main pass CALLs the operator= 0x115b30 (the "Copy" reloc). The dissolve keeps
// that split: the shadow's rect is passed as a CRect lvalue (*(CRect*)&sh) so the trivial
// copy ctor inlines, while the main pass's `CRect rect; rect = *rc;` keeps the EXTERNAL
// operator= -> a call. The rect-slot collapse drops sub esp,0x20 -> 0x10 so every [esp+N]
// realigns; all 10 callees (SetFont/CopyRect/OffsetRect/SetColor x2/CString x2/DrawWrapped x2
// + the CRect Copy) pair. Residual: the font-size sparse switch byte-index-table + jump-table
// are separate $L COMDATs (delinker-inline artifact, docs/patterns/
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
    CString* str = reinterpret_cast<CString*>(a1);
    RECT* rc = reinterpret_cast<RECT*>(a2);
    if (shadow) {
        RECT sh;
        CopyRect(&sh, rc);
        OffsetRect(&sh, 2, 3);
        g_textObj.SetColor(0);
        // the shadow pass reinterprets the local RECT as a CRect lvalue so the trivial
        // copy ctor inlines (4-mov copy of sh); the main pass below CALLs the 0x115b30
        // operator= to build its rect.
        g_textObj.DrawWrapped(*str, static_cast<CDDSurface*>(drawFn), *static_cast<CRect*>(&sh), 1, flag, 0);
    }
    g_textObj.SetColor(((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff));
    CRect rect;
    rect = *rc; // 0x115b30 CRect::operator=(const tagRECT&) (the "Copy" reloc)
    g_textObj.DrawWrapped(*str, static_cast<CDDSurface*>(drawFn), rect, 1, flag, 0);
    return 1;
}
