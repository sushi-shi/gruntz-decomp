#include <Wap32/EngStr.h> // own extern surface
#include <Mfc.h> // real MFC CString (copy ctor 0x1b9ba3) + windows.h (RECT/CopyRect/OffsetRect)
#include <Ints.h>
#include <rva.h>
#include <Font/Font.h> // canonical FontRenderer + CRect (RenderText IS DrawWrapped @0x17a460)

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
i32 EngStr_RenderText(
    void* self,
    i32 a1,
    i32 a2,
    CDDSurface* drawSurface,
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
    if (drawSurface == 0) {
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
        g_textObj.DrawWrapped(*str, drawSurface, *static_cast<CRect*>(&sh), 1, flag, 0);
    }
    g_textObj.SetColor(((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff));
    CRect rect;
    rect = *rc; // 0x115b30 CRect::operator=(const tagRECT&) (the "Copy" reloc)
    g_textObj.DrawWrapped(*str, drawSurface, rect, 1, flag, 0);
    return 1;
}
