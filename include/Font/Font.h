#ifndef SRC_FONT_FONT_H
#define SRC_FONT_FONT_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Mfc.h>        // real MFC CString / CFile / CArchive / CFileException (kept first)
#include <Wap32/Rect.h> // canonical CRect (the 16-B by-value rect bundle; ctor 0x29ac0)

struct Glyph {
    Glyph() {}  // user-declared (drives the array-new shape)
    i32 width;  // +0x00
    i32 height; // +0x04
};
SIZE(0x8); // m_glyphs[] element stride (8-byte metric record)

class Font {
public:
    Font();
    i32 AllocateMemory(i32 count);
    void FreeMemory();
    i32 LoadFont(CString szFileName);
    i32 SaveFont(CString szFileName); // 0x1799f0

    // Accessors matched in this module cluster. GetGlyph returns the out
    // reference (retail callers read the metric pair back through eax:
    // DrawGlyphRun does `call GetGlyph; mov ebp,[eax]`).
    void** GetSurface(u8 c);           // 0x179b60
    Glyph& GetGlyph(Glyph& out, u8 c); // 0x179b80
    i32 GetMaxHeight();                // 0x179bd0

    i32 m_ready;       // +0x00
    i32 m_count;       // +0x04
    void** m_surfaces; // +0x08
    Glyph* m_glyphs;   // +0x0c
    i32 m_maxHeight;   // +0x10
    i32 m_reserved14;  // +0x14  (unread here; present in the retail object)
};
SIZE(0x18); // the four global Font instances are laid out 0x18 apart

extern Font g_largeFont;  // 0x24eac0
extern Font g_mediumFont; // 0x24eae8
extern Font g_smallFont;  // 0x24eb00
extern Font g_tinyFont;   // 0x24ea58

struct TextExtent {
    i32 width;  // +0x00 (sum of per-glyph advance widths)
    i32 height; // +0x04 (the font line-height, Font::GetMaxHeight)
};
SIZE(0x8); // sret 8-byte {w,h} pair

class CDDSurface; // <DDrawMgr/DDSurface.h> in the dereferencing TUs

class FontRenderer {
public:
    FontRenderer();
    void SetFont(Font* f);    // 0x179c10  (m_font = f)
    void SetColor(i32 color); // 0x179c20

    // Text geometry + drawing (the ClassUnknown_52 cluster).
    TextExtent MeasureText(CString text); // 0x17ac50

    // The inner glyph-run blit (0x179e70, src/Stub/ApiCallers.cpp until its
    // re-home): clip `rc` against the surface + the measured text extent, Lock
    // the surface, then blit each glyph's 8bpp coverage bitmap as the packed
    // 16bpp m_color - alpha-blended per pixel when `blend` is set, opaque
    // threshold otherwise.
    void DrawGlyphRun(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend);

    void DrawLine(CString text, CDDSurface* surf, i32 x, i32 y, i32 z);                  // 0x179c30
    void DrawLineClipped(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 z); // 0x179d10

    // Word-wrap entry points (the two big greedy-wrap methods).
    //   MeasureWrapped - bounding {w,h} of greedily wrapped text (0x17ad10)
    //   DrawWrapped    - lay out + draw each line via DrawLine     (0x17a460)
    // CString-temp-heavy /GX bodies (~1-2 KB) - see src/Font/Font.cpp.
    TextExtent MeasureWrapped(CString text, i32 x0, i32 top, i32 right, i32 bottom); // 0x17ad10
    // The layout rect {left=x0, top, right, bottom} is a by-value CRect (the retail
    // caller EngStr_RenderText builds it through the 0x115b30 CRect operator=): the
    // four rect words land contiguously in the arg area exactly as four adjacent ints,
    // so &rc reinterpreted as a TextRange reads left/right at +0/+8 (the Span helper).
    void DrawWrapped(
        CString text,
        CDDSurface* surf,
        CRect rc,
        i32 z,
        i32 hcenter,
        i32 spacing
    ); // 0x17a460 (== EngStr's FontRenderer::RenderText)

    // 0x17b120 - a third word-wrap entry: greedily lays out `text` from `begin`
    // down to `bottom`, returns the final cursor {x, y + lineHeight + 1} and writes
    // the total laid-out character count to *outLen. Same CString-temp /GX family
    // as the two wrap stubs above; deferred to the final sweep.
    TextExtent LayoutWrapped(
        CString text,
        i32 x0,
        i32 begin,
        i32 right,
        i32 bottom,
        i32* outLen
    ); // 0x17b120

    Font* m_font; // +0x00  (Font* to render with)
    i32 m_color;  // +0x04  (packed colour, default 0x00ffffff)
    // m_surface / m_clip are optional engine drawing handles (a destination surface
    // and a clip rect) used here only as present/absent flags in DrawLineClipped;
    // their concrete engine types are set by an unmatched TU, so they stay void*
    // (documented opaque-handle keep).
    void* m_surface; // +0x08  (optional dest surface handle)
    void* m_clip;    // +0x0c  (optional clip-rect handle)
};
SIZE(0x10); // stateful render shim; the ctor inits exactly the four


#endif // SRC_FONT_FONT_H
