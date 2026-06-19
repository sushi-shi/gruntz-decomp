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
// Global operator new / delete (the NAFXCW heap, engine @0x1b9b46 / @0x1b9b82).
// External / no-body so their `call rel32` displacements reloc-mask in objdiff.
//   ??2@YAPAXI@Z  operator new(unsigned int)
//   ??3@YAXPAX@Z  operator delete(void*)
// ---------------------------------------------------------------------------
void *operator new(unsigned int n);
void operator delete(void *p);

// ---------------------------------------------------------------------------
// The MFC I/O stack the font file is read through. Each class is reconstructed
// only as deeply as its byte-match needs:
//
//   CString  - a single char* @+0 (the MFC string). Only its destructor is
//              touched here (the by-value szFileName arg + the throwaway temp
//              the file-name normalizer builds), the NAFXCW dtor @0x1b9cde.
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
class CFileException;            // PAVCFileException@@ - pointer only, opaque.

class CString {
public:
    CString(const char *s);      // ??0CString@@QAE@PBD@Z  @0x1b9d4c
    CString(const CString &o);   // ??0CString@@QAE@ABV0@@Z @0x1b9ba3 (refcount/COW)
    ~CString();                  // ??1CString@@QAE@XZ     @0x1b9cde
    char *m_pchData;             // +0x00
};

class CFile {
public:
    CFile();                                 // ??0CFile@@QAE@XZ            @0x1befd7
    virtual ~CFile();                        // ??1CFile@@UAE@XZ            @0x1bf121
    virtual int Open(const char *lpszFileName, unsigned int nOpenFlags,
                     CFileException *pError);// ?Open@CFile@@UAEHPBD...     @0x1bf200
    virtual void Close();                    // ?Close@CFile@@UAEXXZ        @0x1bf426

    // CFile carries a vtable (the virtuals above) + several scalar fields; no
    // field is touched inline, so a conservative padded body just reserves the
    // stack slot. (MFC CFile is 0x14 bytes incl. the vptr.)
    char m_body[0x14 - 4];       // (the implicit vptr occupies the first 4 B)
};

class CArchive {
public:
    // ??0CArchive@@QAE@PAVCFile@@IHPAX@Z  @0x1c6ee8
    CArchive(CFile *pFile, unsigned int nMode, int nBufSize, void *lpBuf);
    ~CArchive();                                       // ??1CArchive@@QAE@XZ  @0x1c6fc4

    unsigned int Read(void *lpBuf, unsigned int nMax); // ?Read@...           @0x1c705a
    void Close();                                      // ?Close@...          @0x1c704c
    void FillBuffer(unsigned int nBytesNeeded);        // ?FillBuffer@...      @0x1c7272

    // The buffered-read window. Inlined operator>> reads straight from here;
    // the offsets are load-bearing (the EXE's NAFXCW layout, not afx.h's).
    char  m_pad0[0x24];          // +0x00 (mode/file/buffer-base fields, opaque)
    char *m_lpBufCur;            // +0x24
    char *m_lpBufMax;            // +0x28
    char  m_pad2c[0x40 - 0x2c];  // trailing fields (opaque - the real CArchive
                                 // is wider; size is not load-bearing here)

    // Inlined integer extraction (MFC's _AFX_INLINE operator>>). Tops up the
    // buffer when fewer than 4 bytes remain, then reads a DWORD and advances.
    CArchive &operator>>(int &i)
    {
        if (m_lpBufCur + sizeof(int) > m_lpBufMax)
            FillBuffer(sizeof(int) - (unsigned int)(m_lpBufMax - m_lpBufCur));
        i = *(int *)m_lpBufCur;
        m_lpBufCur += sizeof(int);
        return *this;
    }
};

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
    Font();                                  // @0x179700
    int AllocateMemory(int count);           // @0x179720  (ret 4)
    void FreeMemory();                        // @0x1797b0
    int LoadFont(CString szFileName);        // @0x179830  (ret 4; by-value arg)

    // Accessors matched in this module cluster.
    void **GetSurface(unsigned char c);      // @0x179b60
    void GetGlyph(unsigned char c, Glyph &out); // @0x179b80
    int GetMaxHeight();                      // @0x179bd0

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
    FontRenderer();                          // @0x179be0
    void SetColor(int color);                // @0x179c20
    unsigned char GetChar(int i);            // @0x17b4f0 (ret 4)

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
    void FreeStrings();                      // @0x179680

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
    int IsInterface1();   // @0x1794b0
    int IsInterface2();   // @0x1794e0
    int IsInterface3();   // @0x179510
    int IsInterface4();   // @0x179540
    int IsInterface5();   // @0x179570
};

#endif // SRC_FONT_FONT_H
