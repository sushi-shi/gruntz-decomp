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
    CString();                   // ??0CString@@QAE@XZ     @0x1b9b93
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
    void Write(const void *lpBuf, unsigned int nMax);  // ?Write@...          @0x1c7168
    void Close();                                      // ?Close@...          @0x1c704c
    void FillBuffer(unsigned int nBytesNeeded);        // ?FillBuffer@...     @0x1c7272

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

    // Inlined integer insertion (MFC's _AFX_INLINE operator<<). Tops up the
    // buffer when fewer than 4 bytes remain, then writes a DWORD and advances.
    CArchive &operator<<(int i)
    {
        if (m_lpBufCur + sizeof(int) > m_lpBufMax)
            FillBuffer(sizeof(int) - (unsigned int)(m_lpBufMax - m_lpBufCur));
        *(int *)m_lpBufCur = i;
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

    // Additional Font methods matched in this module cluster
    int SaveFont(CString szFileName);        // @0x1799f0
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
// CDWordArray (MFC) - partial declaration sufficient for the methods used
// in the FECFile code.  The linker resolves against nafxcw.lib.
// ---------------------------------------------------------------------------
struct CDWordArray {
    // Methods (non-virtual; resolved via direct NAFXCW call)
    void SetSize(int nNewSize, int nGrowBy);
    void SetAtGrow(int nIndex, unsigned long newElement);

    // Fields
    void *m_pVtable;       // +0x00
    int  *m_pData;          // +0x04
    int   m_nSize;          // +0x08
    int   m_nMaxSize;       // +0x0c
    int   m_nGrowBy;        // +0x10
};

// ---------------------------------------------------------------------------
// FontRenderer - a stateful rendering shim wrapping a Font*. FontRenderer
// carries an optional destination surface pointer, clip rectangle, and a
// text colour. The matched functions below live in the same TU.
// ---------------------------------------------------------------------------
class FontRenderer {
public:
    // The default ctor zeros m_font, sets m_color to 0x00ffffff, and
    // clears the two pointer fields.
    FontRenderer();                          // @0x179be0

    // Colour setter used by TextOut.
    void SetColor(int color);                // @0x179c20

    // Measure a CString: returns {totalWidth, maxHeight} in the 2-int block.
    int *MeasureString(CString &text, int out[2]);  // @0x17ac50 (ret 8)

    // Text-out helpers, all thiscall.
    int DrawText(CString &text, int a1, int a2, int a3, int a4); // @0x179c30 (ret 0x14)
    int TextOut(CString &text, int a1, int a2, int a3, int a4,
                int a5, int a6, int a7, int a8);                  // @0x179d10 (ret 0x24)
    int Render(CString &text, int a1, int a2, int a3, int a4,
               int a5, int a6, int a7, int a8);                   // @0x179e70 (ret 0x24)
    int ComplexRender(CString &text, int a1, int a2, int a3, int a4,
                      int a5, int a6, int a7, int a8);            // @0x17a460 (ret 0x24)

    // Wrapping / formatting (ret 0x18 and 0x1c).
    int FormatText(CString &text, int a1, int a2, int a3, int a4, int a5); // @0x17ad10 (ret 0x18)
    int FormatText2(CString &text, int a1, int a2, int a3, int a4,
                    int a5, int a6);                                       // @0x17b120 (ret 0x1c)

    // Tiny accessors.
    unsigned char GetChar(int i);   // @0x17b4f0 (ret 4)
    int GetStringLength();          // @0x17b500

    Font *m_font;       // +0x00  (Font* to render with)
    int   m_color;      // +0x04  (packed colour, default 0x00ffffff)
    void *m_surface;    // +0x08  (optional dest surface pointer)
    void *m_clip;       // +0x0c  (optional clip rect pointer)
};

// ---------------------------------------------------------------------------
// FECFile — lightweight handler for Monolith's FEC archive format (opened
// via an indirect virtual I/O interface).  All 5 matched methods below live
// in the Font TU.
//
// The I/O interface is an opaque object at +0x124 with a vtable pointer.
// The CDWordArray lives at +0x138 (embedded, 0x14 bytes).
// ---------------------------------------------------------------------------

// I/O interface vtable layout for the opaque object embedded in FECFile.
// Only the methods actually called are declared; the padding-zero methods
// keep the vtable slots aligned.
struct IOFace {
    virtual int m00() = 0;  // +0x00
    virtual int m01() = 0;  // +0x04
    virtual int m02() = 0;  // +0x08
    virtual int m03() = 0;  // +0x0c
    virtual int m04() = 0;  // +0x10
    virtual int m05() = 0;  // +0x14
    virtual int m06() = 0;  // +0x18
    virtual int m07() = 0;  // +0x1c
    virtual int m08() = 0;  // +0x20
    virtual int m09() = 0;  // +0x24
    virtual int Open(const char *pszFileName, int nOpenFlags, int nMode) = 0; // +0x28 (slot 10)
    virtual int m0b() = 0;  // +0x2c
    virtual int Seek(int lOff, int nFrom) = 0; // +0x30 (slot 12)
    virtual int m0d() = 0;  // +0x34
    virtual int m0e() = 0;  // +0x38
    virtual int Read(void *lpBuf, int nCount) = 0; // +0x3c (slot 15)
    virtual int m10() = 0;  // +0x40
    virtual int m11() = 0;  // +0x44
    virtual int m12() = 0;  // +0x48
    virtual int m13() = 0;  // +0x4c
    virtual int m14() = 0;  // +0x50
    virtual int Close() = 0; // +0x54 (slot 21)
};

class FECFile {
public:
    int  Init();                             // @0x17b510
    void Close();                            // @0x17b570
    int  Flush();                            // @0x17b5a0
    int  Open(CString const &fileName);      // @0x17b5f0
    int  GetEntrySize(int entryIndex);       // @0x17b840

    // Layout (pinned from matched methods, offsets only, names are
    // placeholders):
    int    m_ready;              // +0x00
    int    m_opened;             // +0x04
    int    m_field08;            // +0x08
    int    m_header[3];          // +0x0c  (12 B)
    int    m_versionMajor;       // +0x10  (guessed)
    int    m_numFiles;           // +0x14  (guessed)
    unsigned char m_data[0x10c]; // +0x18  (268 B, read from the file)
    int    m_field120;           // +0x120
    short  m_field11e;           // +0x11e  (word, at +0x11e in the block)
    // I/O interface (8 bytes: vtable ptr + field128)
    int    m_ioVtable;           // +0x124  (vtable pointer for I/O)
    int    m_ioField128;         // +0x128
    int    m_pad12c[2];          // +0x12c (8 bytes padding)
    int    m_field134;           // +0x134
    // CDWordArray (embedded, 0x14 bytes = 20 bytes)
    CDWordArray m_nameMap;       // +0x138  (embedded CDWordArray, 20 bytes)
};

// ---------------------------------------------------------------------------
// WapNode — base class for a small COM-style object hierarchy that lives in
// the Font TU.  Derived classes have vtables that switch during destruction;
// they own zero or two CStrings and up to two raw-pointer allocations.
//
// The matched functions:
//   CWapNodeA  ~CWapNodeA      @0x179340  (vtable 0x5f0748)
//   CWapNodeB  ~CWapNodeB      @0x1793b0  (vtable 0x5f0760)
//   CWapNodeC  ~CWapNodeC      @0x179420  (vtable 0x5f0778)
//
//   CWapNodeB::Assign          @0x1795a0  (deep-copy assignment)
//   CWapNodeB::FreeStrings     @0x179680
//   CWapNodeC::Setup           @0x1796c0
// ---------------------------------------------------------------------------
struct CWapNodeBase {
    virtual ~CWapNodeBase();                // only the vtable matters
};

struct CWapNodeA : CWapNodeBase {
    virtual ~CWapNodeA();
    int     m_field04;              // +0x04
    CString m_str08;                // +0x08
    int     m_field0c;              // +0x0c
};

struct CWapNodeB : CWapNodeBase {
    virtual ~CWapNodeB();
    CWapNodeB &Assign(CWapNodeB const &src); // @0x1795a0
    void FreeStrings();                      // @0x179680

    // Fields from 80-byte-copy region (0x04..0x53):
    int     m_type;                 // +0x04  (0x50 after assign)
    char    m_pad08[0x28];          // +0x08..+0x2f  (40 B packed data)
    char   *m_srcStr1;              // +0x30  (original pointer before deep-copy)
    char    m_pad34[8];             // +0x34..+0x3b  (str1 & str2 string buffers)
    char    m_rest[0x18];           // +0x3c..+0x53  (rest of 80 B)
    // Total: 0x54 from +0x04 offset = 0x58 including vtable.
};

struct CWapNodeC : CWapNodeBase {
    virtual ~CWapNodeC();
    int Setup(int type, char const *s1, char const *s2, int val); // @0x1796c0

    int     m_field04;              // +0x04
    CString m_str08;                // +0x08
    CString m_str0c;                // +0x0c
    int     m_field10;              // +0x10
    void   *m_ptr14;                // +0x14  (operator-new'd)
    void   *m_ptr18;                // +0x18  (operator-new'd)
    int     m_field1c;              // +0x1c
    int     m_field20;              // +0x20
};

// ---------------------------------------------------------------------------
// NodeList — helper that walks a singly-linked list of WapNodeBase pointers
// and finds the first node whose data member matches a requested interface
// GUID (the 5 static IsA helpers).
// ---------------------------------------------------------------------------
struct NodeList {
    int FindObject(int which, void *listHead); // @0x179270
};

// InterfaceObject — a minimal COM-style object that carries a GUID pointer at
// +0x04.  The IsInterfaceX methods check whether that GUID matches a known
// interface identifier.  They are called as __thiscall with ecx = the object.
struct InterfaceObject {
    // +0x00: vtable (or any first field, not accessed by IsInterfaceX)
    // +0x04: const void *iid  -- pointer to 16-byte interface GUID
    const void *iid;
    int IsInterface1();   // @0x1794b0
    int IsInterface2();   // @0x1794e0
    int IsInterface3();   // @0x179510
    int IsInterface4();   // @0x179540
    int IsInterface5();   // @0x179570
};

// A CString-taking helper (creates a sub-string from this+8).
void CStringFromField(CString &dst, CString const &src); // @0x179300

// Global function pointer for IntersectRect (used by FontRenderer::Render).
typedef int (__stdcall *IntersectRectFn)(void *dst, void *src1, void *src2);
extern IntersectRectFn g_IntersectRect;

#endif // SRC_FONT_FONT_H
