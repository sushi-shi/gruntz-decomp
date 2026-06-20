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

// ---------------------------------------------------------------------------
// Global operator new / delete (the NAFXCW heap).
// External / no-body so their `call rel32` displacements reloc-mask in objdiff.
//   operator new(unsigned int)
//   operator delete(void*)
// ---------------------------------------------------------------------------
void *operator new(unsigned int n);
void operator delete(void *p);

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
#include <Mfc.h>   // real MFC CString / CFile / CArchive / CFileException

// ---------------------------------------------------------------------------
// A single glyph's metric record (the m_glyphs[] element, 8 B). The pixel
// surface for the glyph is width*height bytes (one byte per pixel).
// ---------------------------------------------------------------------------
struct Glyph {
    Glyph() {}                   // user-declared (drives the array-new shape)
    int width;                   // +0x00
    int height;                  // +0x04
};

// ---------------------------------------------------------------------------
// Font - the bitmap font.
// ---------------------------------------------------------------------------
class Font {
public:
    Font();
    int AllocateMemory(int count);
    void FreeMemory();
    int LoadFont(CString szFileName);

    // Accessors matched in this module cluster.
    void **GetSurface(unsigned char c);
    void GetGlyph(unsigned char c, Glyph &out);
    int GetMaxHeight();

    int    m_ready;              // +0x00
    int    m_count;             // +0x04
    void **m_surfaces;          // +0x08
    Glyph *m_glyphs;            // +0x0c
    int    m_maxHeight;         // +0x10
};

// ---------------------------------------------------------------------------
// FontRenderer - a stateful rendering shim wrapping a Font*. Only the three
// leaves matched here (ctor / SetColor / GetChar) are reconstructed; the other
// render entry points stay external (no body) so their calls reloc-mask.
// ---------------------------------------------------------------------------
class FontRenderer {
public:
    FontRenderer();
    void SetColor(int color);
    unsigned char GetChar(int i);

    Font *m_font;       // +0x00  (Font* to render with)
    int   m_color;      // +0x04  (packed colour, default 0x00ffffff)
    void *m_surface;    // +0x08  (optional dest surface pointer)
    void *m_clip;       // +0x0c  (optional clip rect pointer)
};

// ---------------------------------------------------------------------------
// CWapNodeB - a WAP node carrying packed data + two owned string buffers.
// Only FreeStrings is matched here.
// ---------------------------------------------------------------------------
struct CWapNodeBase {
    virtual ~CWapNodeBase();                // only the vtable matters
};

struct CWapNodeB : CWapNodeBase {
    virtual ~CWapNodeB();
    void FreeStrings();

    int     m_type;                 // +0x04
    char    m_pad08[0x28];          // +0x08..+0x2f
    char   *m_srcStr1;              // +0x30
    char    m_pad34[8];             // +0x34..+0x3b
    char    m_rest[0x18];           // +0x3c..+0x53
};

// InterfaceObject - a minimal COM-style object that carries a GUID pointer at
// +0x04. The IsInterfaceX methods check whether that GUID matches a known iid.
struct InterfaceObject {
    const void *iid;                // +0x04 (after vtable/first field)
    int IsInterface1();
    int IsInterface2();
    int IsInterface3();
    int IsInterface4();
    int IsInterface5();
};

#endif // SRC_FONT_FONT_H
