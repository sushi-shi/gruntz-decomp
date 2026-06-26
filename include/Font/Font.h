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

// ---------------------------------------------------------------------------
// Global operator new / delete (the NAFXCW heap).
// External / no-body so their `call rel32` displacements reloc-mask in objdiff.
//   operator new(unsigned int)
//   operator delete(void*)
// ---------------------------------------------------------------------------
void* operator new(u32 n);
void operator delete(void* p);

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
#include <Mfc.h> // real MFC CString / CFile / CArchive / CFileException

// ---------------------------------------------------------------------------
// A single glyph's metric record (the m_glyphs[] element, 8 B). The pixel
// surface for the glyph is width*height bytes (one byte per pixel).
// ---------------------------------------------------------------------------
struct Glyph {
    Glyph() {}  // user-declared (drives the array-new shape)
    i32 width;  // +0x00
    i32 height; // +0x04
};

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

    // Accessors matched in this module cluster.
    void** GetSurface(u8 c);
    void GetGlyph(u8 c, Glyph& out);
    i32 GetMaxHeight();

    i32 m_ready;       // +0x00
    i32 m_count;       // +0x04
    void** m_surfaces; // +0x08
    Glyph* m_glyphs;   // +0x0c
    i32 m_maxHeight;   // +0x10
};

// ---------------------------------------------------------------------------
// The pixel extent of a measured run of text: {total advance width, line
// height}. Returned by value (sret) from FontRenderer::MeasureText - a plain
// 8-byte pair so the two int stores reproduce exactly.
// ---------------------------------------------------------------------------
struct TextExtent {
    i32 width;  // +0x00 (sum of per-glyph advance widths)
    i32 height; // +0x04 (the font line-height, Font::GetMaxHeight)
};

// ---------------------------------------------------------------------------
// FontRenderer - a stateful rendering shim wrapping a Font*. The leaf methods
// (ctor / SetColor / GetChar) plus the text geometry/word-wrap/draw entry
// points below are matched here; the lower-level blit/glyph callees stay
// external (no body) so their calls reloc-mask.
// ---------------------------------------------------------------------------
// A 16-byte rectangle bundle passed by value to the inner glyph-blit
// (DrawGlyphRun) - four ints carried as one block on the stack. Built by the
// out-of-line Rect(i32,i32,i32,i32) ctor at 0x29ac0 (external/no-body here).
struct Rect {
    Rect(i32 a, i32 b, i32 c, i32 d); // 0x29ac0
    i32 a;                            // +0x00
    i32 b;                            // +0x04
    i32 c;                            // +0x08
    i32 d;                            // +0x0c
};

// The layout/draw rectangle the public draw entry points receive a pointer to:
// a destination box plus alignment/limit fields. Only the offsets the matched
// methods read are load-bearing.
struct DrawRect {
    i32 left;     // +0x00
    i32 top;      // +0x04
    i32 right;    // +0x08
    i32 bottom;   // +0x0c
    i32 m_10;     // +0x10
    i32 m_14;     // +0x14
    i32 m_bottom; // +0x18  (vertical limit, clip test in DrawLine)
};

class FontRenderer {
public:
    FontRenderer();
    void SetColor(i32 color);
    u8 GetChar(i32 i);

    // Text geometry + drawing (the ClassUnknown_52 cluster).
    TextExtent MeasureText(CString text); // 0x17ac50

    // The inner glyph-run blit (0x179e70). External (no body here) so its
    // call reloc-masks; the 9-dword arg shape (CString + 1 int + Rect{4} + 3
    // ints) reproduces the retail `ret 0x24` and the caller's pushes.
    void DrawGlyphRun(CString text, i32 a1, Rect rc, i32 x, i32 y, i32 z); // 0x179e70

    void DrawLine(DrawRect* p, i32 x, i32 y, CString text, i32 a4);           // 0x179c30
    void DrawLineClipped(CString text, i32 a1, Rect rc, i32 x, i32 y, i32 z); // 0x179d10

    // Word-wrap entry points (the two big methods) deferred to the final sweep:
    //   Stub_17ad10 = MeasureWrapped (bounding {w,h} of greedily wrapped text)
    //   Stub_17a460 = DrawWrapped    (lay out + draw each line via DrawLine)
    // CString-temp-heavy /GX bodies (~1-2 KB); kept as backlog stubs, RVA-tracked
    // under their real class - see src/Font/Font.cpp.
    void Stub_17ad10(); // 0x17ad10 (FontRenderer::MeasureWrapped)
    void Stub_17a460(); // 0x17a460 (FontRenderer::DrawWrapped)

    // 0x17b120 - a third word-wrap entry: greedily lays out `text` from `begin`
    // down to `bottom`, returns the final cursor {x, y + lineHeight + 1} and writes
    // the total laid-out character count to *outLen. Same CString-temp /GX family
    // as the two wrap stubs above; deferred to the final sweep.
    TextExtent LayoutWrapped(CString text, i32 x0, i32 begin, i32 right, i32 bottom,
                             i32* outLen); // 0x17b120

    Font* m_font;    // +0x00  (Font* to render with)
    i32 m_color;     // +0x04  (packed colour, default 0x00ffffff)
    void* m_surface; // +0x08  (optional dest surface pointer)
    void* m_clip;    // +0x0c  (optional clip rect pointer)
};

// ---------------------------------------------------------------------------
// TextRange - a {begin..end} view over a measured run of text, built on the
// FontRenderer word-wrap stack (three adjacent CString locals: the line text and
// its head/tail markers). Span() returns the byte distance end - begin (a signed
// length fed to the per-line divide in the wrap loop). Only the two char* fields
// it reads (+0x00 begin, +0x08 end) are load-bearing.
// ---------------------------------------------------------------------------
struct TextRange {
    char* m_begin;     // +0x00
    char* m_pad04;     // +0x04
    char* m_end;       // +0x08
    i32 Span(); // 0x17b500
};

// ---------------------------------------------------------------------------
// CWapNodeB - a WAP node carrying packed data + two owned string buffers.
// Only FreeStrings is matched here.
// ---------------------------------------------------------------------------
struct CWapNodeBase {
    virtual ~CWapNodeBase(); // only the vtable matters
};

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

// InterfaceObject - a minimal COM-style object that carries a GUID pointer at
// +0x04. The IsInterfaceX methods check whether that GUID matches a known iid.
struct InterfaceObject {
    const void* iid; // +0x04 (after vtable/first field)
    i32 IsInterface1();
    i32 IsInterface2();
    i32 IsInterface3();
    i32 IsInterface4();
    i32 IsInterface5();
};

#endif // SRC_FONT_FONT_H
