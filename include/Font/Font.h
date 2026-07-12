// Font.h - the engine's bitmap Font class (the text/glyph subsystem). A Font
// owns a per-letter table of glyph metrics + a parallel table of decoded pixel
// surfaces, loaded from a binary font file through the MFC CFile/CArchive I/O
// stack. Minimal Monolith-faithful reconstruction sufficient to byte-match the
// leaf methods. Field names are placeholders (m_<hexoffset>); only the OFFSETS
// and the code bytes are load-bearing (campaign doctrine).
//
// Font instance layout (pinned from the matched methods):
//   +0x00  m_ready    : loaded/allocated flag (0 = empty, 1 = populated).
//   +0x04  m_count    : letter count (number of glyphs in the font).
//   +0x08  m_surfaces : void*[m_count] - per-glyph decoded pixel buffers
//                       (each operator-new'd to glyph.width * glyph.height
//                       bytes; freed in FreeMemory).
//   +0x0c  m_glyphs   : Glyph[m_count] - per-glyph metric records (8 B each:
//                       {int width; int height;}); operator-new'd as one block.
//   +0x10  m_maxHeight: max glyph height across the table (the font line-height,
//                       computed at the tail of LoadFont).
#ifndef SRC_FONT_FONT_H
#define SRC_FONT_FONT_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// The global operator new / delete (the NAFXCW heap) are the language's
// implicitly-declared allocation functions (also surfaced by <Mfc.h>/<new> below);
// no local re-declaration is needed - their `call rel32` displacements reloc-mask
// in objdiff exactly the same.

// ---------------------------------------------------------------------------
// The MFC I/O stack the font file is read through. Each class is reconstructed
// only as deeply as its byte-match needs:
//
//   CString  - a single char* @+0 (the MFC string). Only its destructor is
//              touched here (the by-value szFileName arg + the throwaway temp
//              the file-name normalizer builds), the NAFXCW dtor.
//   CFile    - the MFC binary file. Default-constructed on the stack, opened,
//              closed, destroyed. NO field of it is accessed inline, so only the
//              method symbols/calling-convention are load-bearing; its size just
//              has to cover the stack slot. Its Open/~CFile/Close are virtual in
//              MFC (UAE) but devirtualize to direct `call rel32` on the concrete
//              stack object.
//   CArchive - the buffered reader layered over the CFile. Its inlined
//              extraction operator reads straight out of the m_lpBufCur/
//              m_lpBufMax window (load-bearing offsets +0x24/+0x28), topping up
//              via the out-of-line FillBuffer; Read/Close/~CArchive are
//              out-of-line library calls.
//
// All of these resolve to NAFXCW (the statically linked MFC) - their bodies are
// never matched here; only the exact mangled symbol + arg shape matter.
// ---------------------------------------------------------------------------
#include <Mfc.h>        // real MFC CString / CFile / CArchive / CFileException (kept first)
#include <Wap32/Rect.h> // canonical CRect (the 16-B by-value rect bundle; ctor 0x29ac0)

// ---------------------------------------------------------------------------
// A single glyph's metric record (the m_glyphs[] element, 8 B). The pixel
// surface for the glyph is width*height bytes (one byte per pixel).
// ---------------------------------------------------------------------------
struct Glyph {
    Glyph() {}  // user-declared (drives the array-new shape)
    i32 width;  // +0x00
    i32 height; // +0x04
};
SIZE(Glyph, 0x8); // m_glyphs[] element stride (8-byte metric record)

// ---------------------------------------------------------------------------
// Font - the bitmap font.
// ---------------------------------------------------------------------------
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
SIZE(Font, 0x18); // the four global Font instances are laid out 0x18 apart
                  // (g_mediumFont 0x24eae8 -> g_smallFont 0x24eb00, adjacent)

// ---------------------------------------------------------------------------
// The pixel extent of a measured run of text: {total advance width, line
// height}. Returned by value (sret) from FontRenderer::MeasureText - a plain
// 8-byte pair so the two int stores reproduce exactly.
// ---------------------------------------------------------------------------
struct TextExtent {
    i32 width;  // +0x00 (sum of per-glyph advance widths)
    i32 height; // +0x04 (the font line-height, Font::GetMaxHeight)
};
SIZE(TextExtent, 0x8); // sret 8-byte {w,h} pair

// ---------------------------------------------------------------------------
// FontRenderer - a stateful rendering shim wrapping a Font*. The leaf methods
// (ctor / SetColor / GetChar) plus the text geometry/word-wrap/draw entry
// points below are matched here; the lower-level blit/glyph callees stay
// external (no body) so their calls reloc-mask.
// ---------------------------------------------------------------------------
// The 16-byte by-value rectangle bundle passed to the inner glyph-blit
// (DrawGlyphRun) is the canonical CRect (Wap32/Rect.h): four ints {left,top,right,
// bottom} built by the out-of-line CRect(i32,i32,i32,i32) ctor at 0x29ac0. The former
// per-TU `struct Rect` view of it is dissolved (MODEL THE CLASS, NOT THE VIEW).

// The draw target of the whole draw family is the DirectDraw surface wrapper
// (proven by DrawGlyphRun 0x179e70: arg2 is Lock()ed / m_width / m_height /
// m_pitch read / m_8->Unlock(0), and DrawLine's vertical limit [p+0x18] is its
// dwHeight). The former per-TU "DrawRect" view of it is dissolved.
class CDDSurface; // <DDrawMgr/DDSurface.h> in the dereferencing TUs

class FontRenderer {
public:
    FontRenderer();
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
    void DrawWrapped(
        CString text,
        CDDSurface* surf,
        i32 x0,
        i32 top,
        i32 right,
        i32 bottom,
        i32 z,
        i32 hcenter,
        i32 spacing
    ); // 0x17a460

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
SIZE(FontRenderer, 0x10); // stateful render shim; the ctor inits exactly the four
                          // fields m_font/m_color/m_surface/m_clip (through +0x0c)

// ---------------------------------------------------------------------------
// TextRange - a {begin..end} view over a measured run of text, built on the
// FontRenderer word-wrap stack (three adjacent CString locals: the line text and
// its head/tail markers). Span() returns the byte distance end - begin (a signed
// length fed to the per-line divide in the wrap loop). Only the two char* fields
// it reads (+0x00 begin, +0x08 end) are load-bearing.
// ---------------------------------------------------------------------------
struct TextRange {
    char* m_begin; // +0x00
    char* m_pad04; // +0x04
    char* m_end;   // +0x08
    i32 Span();    // 0x17b500
};
SIZE_UNKNOWN(TextRange); // {begin..end} view built on three adjacent stack CStrings

// ---------------------------------------------------------------------------
// CharCursor - the per-character accessor at 0x17b4f0. It reads byte `i` of the
// char* stored at its +0x00. The word-wrap loops call it on their CString line
// temps (whose m_pchData sits at +0x00) by reinterpreting the CString as this
// accessor - the retail bridge between MFC CString storage and the engine's own
// byte-indexed glyph lookup. GetChar's `this` is therefore always one of those
// CString temps, never a real FontRenderer.
// ---------------------------------------------------------------------------
struct CharCursor {
    u8* m_str; // +0x00  (aliases CString::m_pchData)
    RVA(0x0017b4f0, 0xc)
    u8 GetChar(i32 i) {
        return m_str[i];
    }
};
SIZE_UNKNOWN(CharCursor); // reinterpret view over a CString's m_pchData

// ---------------------------------------------------------------------------
// CWapNodeB - a WAP node carrying packed data + two owned string buffers.
// Only FreeStrings is matched here.
// ---------------------------------------------------------------------------
struct CWapNodeBase {
    virtual ~CWapNodeBase(); // only the vtable matters
};
SIZE_UNKNOWN(CWapNodeBase);

struct CWapNodeB : CWapNodeBase {
    virtual ~CWapNodeB() OVERRIDE;
    void FreeStrings();

    i32 m_type;         // +0x04
    char m_pad08[0x28]; // +0x08..+0x2f
    char* m_srcStr1;    // +0x30
    char* m_buf34;      // +0x34  owned buffer (FreeStrings deletes)
    char* m_buf38;      // +0x38  owned buffer (FreeStrings deletes)
    char m_rest[0x18];  // +0x3c..+0x53
};
SIZE_UNKNOWN(CWapNodeB);

// FontInterfaceObject - a minimal COM-style object that carries a GUID pointer at
// +0x04. The IsInterfaceX methods check whether that GUID matches a known iid.
struct FontInterfaceObject {
    char _pad00[4];  // +0x00  (unread by the iid checks below; iid lives at +0x04)
    const void* iid; // +0x04
    i32 IsInterface1();
    i32 IsInterface2();
    i32 IsInterface3();
    i32 IsInterface4();
    i32 IsInterface5();
};
SIZE_UNKNOWN(FontInterfaceObject); // Font's COM iid-checker (completeness-only; allocated
                                   // elsewhere - name shared with Net's polymorphic view)

// --- vtable catalog ---

#endif // SRC_FONT_FONT_H
