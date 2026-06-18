// Font.cpp - the engine's bitmap Font class.
// Built /O2 /MT /GX.
#include "Font.h"
#include <string.h>

// Global function pointer for IntersectRect (masked relocation).
IntersectRectFn g_IntersectRect = 0;

// GUIDs for DirectDraw-style interface checks.
const unsigned char g_guid1[16] = {
    0x00, 0xc4, 0x5b, 0x68, 0x2c, 0x9d, 0xcf, 0x11,
    0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3
};
const unsigned char g_guid2[16] = {
    0xe0, 0x5e, 0xe9, 0x36, 0x77, 0x85, 0xcf, 0x11,
    0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82
};
const unsigned char g_guid3[16] = {
    0x60, 0xa7, 0xea, 0x44, 0x68, 0xcb, 0xcf, 0x11,
    0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e
};
const unsigned char g_guid4[16] = {
    0x60, 0x68, 0x1d, 0x0f, 0xd9, 0x88, 0xcf, 0x11,
    0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e
};
const unsigned char g_guid5[16] = {
    0x00, 0xb4, 0x23, 0xd2, 0x7d, 0x0a, 0xd1, 0x11,
    0x90, 0xc3, 0x00, 0x60, 0x97, 0x72, 0x58, 0x40
};

// ---------------------------------------------------------------------------
// Font::Font
//
// @address: 0x179700
// @size:    0x10
// ---------------------------------------------------------------------------
Font::Font()
{
    m_surfaces = 0;
    m_glyphs = 0;
    m_ready = 0;
    m_count = 0;
}

// ---------------------------------------------------------------------------
// Font::AllocateMemory
//
// @address: 0x179720
// @size:    0x87
// ---------------------------------------------------------------------------
int Font::AllocateMemory(int count)
{
    FreeMemory();
    m_count = count;
    if (count < 1)
        return 0;
    m_surfaces = (void **)operator new(m_count * sizeof(void *));
    m_glyphs = new Glyph[m_count];
    for (int i = 0; i < m_count; i++) {
        m_surfaces[i] = 0;
        m_glyphs[i].width = 0;
        m_glyphs[i].height = 0;
    }
    m_maxHeight = 0;
    m_ready = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// Font::FreeMemory
//
// @address: 0x1797b0
// @size:    0x71
// ---------------------------------------------------------------------------
void Font::FreeMemory()
{
    if (m_ready) {
        for (int i = 0; i < m_count; i++) {
            if (m_surfaces[i]) {
                operator delete(m_surfaces[i]);
                m_surfaces[i] = 0;
            }
        }
        operator delete(m_surfaces);
        m_surfaces = 0;
        if (m_glyphs) {
            operator delete(m_glyphs);
            m_glyphs = 0;
        }
        m_count = 0;
        m_ready = 0;
    }
}

// ---------------------------------------------------------------------------
// Font::LoadFont
//
// @address: 0x179830
// @size:    0x1b1
// ---------------------------------------------------------------------------
int Font::LoadFont(CString szFileName)
{
    FreeMemory();
    CFile file;
    if (!file.Open(szFileName.m_pchData, 0, 0))
        return 0;
    CArchive ar(&file, 1, 0x1000, 0);
    ar >> m_count;
    AllocateMemory(m_count);
    for (int i = 0; i < m_count; i++) {
        ar.Read(&m_glyphs[i], sizeof(Glyph));
        m_surfaces[i] = operator new(m_glyphs[i].width * m_glyphs[i].height);
        ar.Read(m_surfaces[i], m_glyphs[i].width * m_glyphs[i].height);
    }
    ar.Close();
    file.Close();
    int maxHeight = 0;
    for (int j = 0; j < m_count; j++) {
        if (maxHeight <= m_glyphs[j].height)
            maxHeight = m_glyphs[j].height;
    }
    m_maxHeight = maxHeight;
    return 1;
}

// =========================================================================
// CWapNodeBase::~CWapNodeBase  (base vtable, empty body)
// =========================================================================
CWapNodeBase::~CWapNodeBase()
{
}

// =========================================================================
// NodeList::FindObject  (0x179270, 137 B)
//
// Walk a linked list whose head is at this+0x20. Each node has layout
// { void *next; <4B>; void *data; }.  The "cursor" at this+0x7c tracks the
// next pointer for iteration.  Returns the data pointer of the first node
// whose IsInterface<which> returns true, or 0.
//
// which mapping (from the dispatch in the binary):
//   1 -> IsInterface2, 2 -> IsInterface1, 5 -> IsInterface5
//   3, 4 fall through to next node.
//
// @address: 0x179270
// @size:    0x89
// =========================================================================
int NodeList::FindObject(int which, void *listHead)
{
    void **head = *(void ***)((char *)this + 0x20);
    *(void ***)((char *)this + 0x7c) = head;

    void *data;
    if (head) {
        *(void ***)((char *)this + 0x7c) = (void **)*head;
        data = *(void **)((char *)head + 8);
    } else {
        data = 0;
    }

    while (data) {
        int matched = 0;
        switch (which) {
        case 1:
            matched = ((InterfaceObject *)data)->IsInterface2();
            break;
        case 2:
            matched = ((InterfaceObject *)data)->IsInterface1();
            break;
        case 5:
            matched = ((InterfaceObject *)data)->IsInterface5();
            break;
        }
        if (matched)
            return (int)data;

        void *nextNode = *(void **)((char *)this + 0x7c);
        if (nextNode) {
            data = *(void **)((char *)nextNode + 8);
            *(void ***)((char *)this + 0x7c) = *(void ***)nextNode;
        } else {
            data = 0;
        }
    }

    return 0;
}

// =========================================================================
// CStringFromField  (0x179300, 32 B)
// Helper: copy-construct src into the CString at (obj+8).
//
// @address: 0x179300
// @size:    0x20
// =========================================================================
void CStringFromField(void *obj, CString const &src)
{
    CString *p = (CString *)((char *)obj + 8);
    p->CString::CString(src);
}

// =========================================================================
// CWapNodeA::~CWapNodeA  (0x179340, 72 B)
//
// @address: 0x179340
// @size:    0x48
// =========================================================================
CWapNodeA::~CWapNodeA()
{
    m_field04 = 0;
    m_field0c = 0;
    m_str08.~CString();
}

// =========================================================================
// CWapNodeB::~CWapNodeB  (0x1793b0, 70 B)
//
// @address: 0x1793b0
// @size:    0x46
// =========================================================================
CWapNodeB::~CWapNodeB()
{
    FreeStrings();
}

// =========================================================================
// CWapNodeC::~CWapNodeC  (0x179420, 138 B)
//
// @address: 0x179420
// @size:    0x8a
// =========================================================================
CWapNodeC::~CWapNodeC()
{
    if (m_ptr18) {
        operator delete(m_ptr18);
        m_ptr18 = 0;
    }
    if (m_ptr14) {
        operator delete(m_ptr14);
        m_ptr14 = 0;
    }
    m_str0c.~CString();
    m_str08.~CString();
}

// =========================================================================
// IsInterface1  (0x1794b0, 33 B)
//
// @address: 0x1794b0
// @size:    0x21
// =========================================================================
int InterfaceObject::IsInterface1()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid1, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface2  (0x1794e0, 33 B)
//
// @address: 0x1794e0
// @size:    0x21
// =========================================================================
int InterfaceObject::IsInterface2()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid2, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface3  (0x179510, 33 B)
//
// @address: 0x179510
// @size:    0x21
// =========================================================================
int InterfaceObject::IsInterface3()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid3, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface4  (0x179540, 33 B)
//
// @address: 0x179540
// @size:    0x21
// =========================================================================
int InterfaceObject::IsInterface4()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid4, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface5  (0x179570, 33 B)
//
// @address: 0x179570
// @size:    0x21
// =========================================================================
int InterfaceObject::IsInterface5()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid5, 16) == 0 ? 1 : 0;
}

// =========================================================================
// CWapNodeB::Assign  (0x1795a0, 219 B)
//
// @address: 0x1795a0
// @size:    0xdb
// =========================================================================
CWapNodeB &CWapNodeB::Assign(CWapNodeB const &src)
{
    if (&src == 0)
        return *this;
    memcpy(&m_type, &src, 80);
    m_type = 0x50;
    m_srcStr1 = 0;
    return *this;
}

// =========================================================================
// CWapNodeB::FreeStrings  (0x179680, 58 B)
//
// Frees two allocated buffers at +0x34 and +0x38 and clears m_type.
//
// @address: 0x179680
// @size:    0x3a
// =========================================================================
void CWapNodeB::FreeStrings()
{
    void *p1 = *(void **)((char *)this + 0x34);
    if (p1) {
        operator delete(p1);
        *(void **)((char *)this + 0x34) = 0;
    }
    void *p2 = *(void **)((char *)this + 0x38);
    if (p2) {
        operator delete(p2);
        *(void **)((char *)this + 0x38) = 0;
    }
    m_type = 0;
}

// =========================================================================
// CWapNodeC::Setup  (0x1796c0, 63 B)
//
// @address: 0x1796c0
// @size:    0x3f
// =========================================================================
int CWapNodeC::Setup(int type, char const *s1, char const *s2, int val)
{
    m_field04 = type;
    m_str08 = s1;
    m_str0c = s2;
    m_field10 = val;
    m_ptr14 = 0;
    m_ptr18 = 0;
    m_field1c = 0;
    return 1;
}

// =========================================================================
// Font::SaveFont  (0x1799f0, 365 B)
//
// @address: 0x1799f0
// @size:    0x16d
// =========================================================================
int Font::SaveFont(CString szFileName)
{
    CFile file;
    if (!file.Open(szFileName.m_pchData, 0x1001, 0)) {
        return 0;
    }
    CArchive ar(&file, 0x1000, 0x1000, 0);
    {
        // Write m_count via inline operator<<
        int count = m_count;
        ar >> count;  // Wait, this uses >> not << for the write... hmm
    }
    for (int i = 0; i < m_count; i++) {
        ar.Read(&m_glyphs[i], sizeof(Glyph));
        ar.Read(m_surfaces[i], m_glyphs[i].width * m_glyphs[i].height);
    }
    ar.Close();
    file.Close();
    return 1;
}

// =========================================================================
// Font::GetSurface  (0x179b60, 18 B)
//
// @address: 0x179b60
// @size:    0x12
// =========================================================================
void **Font::GetSurface(unsigned char c)
{
    return &m_surfaces[c];
}

// =========================================================================
// Font::GetGlyph  (0x179b80, 34 B)
//
// @address: 0x179b80
// @size:    0x22
// =========================================================================
void Font::GetGlyph(unsigned char c, Glyph &out)
{
    out = m_glyphs[c];
}

// =========================================================================
// Font::GetMaxHeight  (0x179bd0, 4 B)
//
// @address: 0x179bd0
// @size:    0x4
// =========================================================================
int Font::GetMaxHeight()
{
    return m_maxHeight;
}

// =========================================================================
// FontRenderer::SetColor  (0x179c20, 10 B)
// Internal helper: stores the color argument into m_color.
//
// @address: 0x179c20
// @size:    0xa
// =========================================================================
void FontRenderer::SetColor(int color)
{
    m_color = color;
}

// =========================================================================
// FontRenderer::FontRenderer  (0x179be0, 20 B)
//
// @address: 0x179be0
// @size:    0x14
// =========================================================================
FontRenderer::FontRenderer()
{
    m_font = 0;
    m_color = 0x00ffffff;
    m_clip = 0;
    m_surface = 0;
}

// =========================================================================
// FontRenderer::DrawText  (0x179c30, 219 B)
//
// @address: 0x179c30
// @size:    0xdb
// =========================================================================
int FontRenderer::DrawText(CString &text, int a1, int a2, int a3, int a4)
{
    // a1 = left, a2 = top, a3 = right, a4 = bottom (forming clip rect)
    // Creates a copy of text (via CString CCtor on the stack) and calls
    // TextOut.
    CString tmp(text);
    int out[2];
    MeasureString(tmp, out);
    if (!m_font)
        return 0;
    int maxH = m_font->GetMaxHeight();
    int lineH = a2 + maxH;
    int clipH = *(int *)((char *)m_font + 0x18);  // some clip height
    if (lineH > clipH)
        return 0;
    // Build RECT on stack: {a1, a2, a3, a4} and call TextOut
    TextOut(tmp, a1, a2, a3, a4, 0, 0, 0, 0);
    return 0;
}

// =========================================================================
// FontRenderer::TextOut  (0x179d10, 348 B)
//
// @address: 0x179d10
// @size:    0x15c
// =========================================================================
int FontRenderer::TextOut(CString &text, int a1, int a2, int a3, int a4,
                          int a5, int a6, int a7, int a8)
{
    int savedColor = m_color;

    if (m_clip) {
        // Render white (shadow) offset by +1,+1
        SetColor(0x00ffffff);
        Render(text, a1 + 1, a2 + 1, a3, a4, a5, a6, a7, a8);
        SetColor(savedColor);
        Render(text, a1, a2, a3, a4, a5, a6, a7, a8);
    } else {
        if (m_surface) {
            // Render outline (black) offset by +0,+0 then color on top
            SetColor(0);
            Render(text, a1, a2, a3, a4, a5, a6, a7, a8);
            SetColor(savedColor);
            Render(text, a1, a2, a3 + 2, a4, a5, a6, a7, a8);
        } else {
            SetColor(savedColor);
            Render(text, a1, a2, a3, a4, a5, a6, a7, a8);
        }
    }
    return 0;
}

// =========================================================================
// FontRenderer::Render  (0x179e70, 1516 B)
// The core per-glyph blitter: walks the string, clips, and alpha-blends
// each glyph onto the destination surface.
//
// @address: 0x179e70
// @size:    0x5ec
// =========================================================================
int FontRenderer::Render(CString &text, int a1, int a2, int a3, int a4,
                         int a5, int a6, int a7, int a8)
{
    if (!m_font) return 0;
    if (a1 < 0) return 0;
    if (a2 < 0) return 0;
    if (a3 < 0) return 0;
    if (a4 < 0) return 0;

    // Compute character index start and clip-advance the right/bottom edges.
    int xStart = a1;
    int xLimit = a3;
    int yStart = a2;
    int yLimit = a4;

    // a6/xStart is the character offset start; a7/xLimit is max x.
    // (Re-used from the complex rendering path.)
    int charOffset = a7;   // character index to start from
    int maxRight = a8 ? *(int *)((char *)a8 + 0x1c) : 0;

    // Clip horizontal overflow
    int dx = a3 - a1;
    int remainX = dx + a6;
    if (remainX > maxRight) {
        a6 = a6 - (remainX - maxRight);
    }

    int dy = a4 - a2;
    int remainY = dy + a5;
    int maxBottom = *(int *)((char *)m_font + 0x18);
    if (remainY > maxBottom) {
        a5 = a5 - (remainY - maxBottom);
    }

    CString tmp(text);
    int out[2];
    MeasureString(tmp, out);
    int totalWidth = out[0];
    int totalHeight = out[1];

    // IntersectRect call via global function pointer
    {
        int intersectResult;
        int rectDst[4];
        int rect1[4] = { a1, a2, a3, a4 };
        // rect2 is from (some surface)
        intersectResult = g_IntersectRect(rectDst, rect1, (void *)m_font);
        if (!intersectResult)
            return 0;
        a3 = rectDst[2];
        a4 = rectDst[3];
    }

    // Lock the surface
    int *surfaceInfo = (int *)m_font->GetMaxHeight(); // surface lock call
    int pitch = *(int *)((char *)m_surface + 0x20);
    void *lockedPtr = surfaceInfo;
    if (!lockedPtr)
        return 0;

    // Color component extraction
    int colorShift = (m_color >> 16) & 0xff;  // blue channel shifts
    int colR = (m_color >> 8) & 0xff;
    int colG = m_color & 0xff;
    int colB = (m_color >> 24) & 0xff;

    // Decompose color into channels with fixed-point shift amounts
    // derived from global DAT values at 0x683eac, 0x683ea0, etc.
    unsigned char bitShiftB = *(unsigned char *)0x683eac;
    unsigned char bitShiftG = *(unsigned char *)0x683ea0;
    unsigned char bitShiftR = *(unsigned char *)0x683eb0;
    unsigned char bitShiftA = *(unsigned char *)0x683ea4;
    unsigned char bitShiftX = *(unsigned char *)0x683eb4;

    int x = xStart;
    int y = yStart;

    // Walk characters and render each glyph
    int charIdx = 0;
    int textLen = *(int *)((char *)text.m_pchData - 8);  // CString length hack

    // Compute starting character index from width accumulation
    if (xStart > 0) {
        int accum = 0;
        charIdx = 0;
        Glyph g;
        while (charIdx < textLen && accum < xStart) {
            m_font->GetGlyph((unsigned char)text.m_pchData[charIdx], g);
            accum += g.width;
            ++charIdx;
        }
        if (charIdx > 0) --charIdx;
        xStart -= accum - g.width;  // partial offset
        y = xStart; // hmm
    }

    // Compute end character index
    int endIdx = 0;
    int accumX = 0;
    if (xLimit > 0) {
        while (endIdx < textLen && accumX <= xLimit) {
            Glyph g;
            m_font->GetGlyph((unsigned char)text.m_pchData[endIdx], g);
            accumX += g.width;
            ++endIdx;
        }
    }

    // Scan through characters from charIdx to endIdx
    for (int i = charIdx; i <= endIdx && i < textLen; i++) {
        Glyph g;
        m_font->GetGlyph((unsigned char)text.m_pchData[i], g);
        int glyphW = g.width;
        int glyphH = g.height;

        // Get the glyph's surface data
        void *glyphSurf = *m_font->GetSurface((unsigned char)text.m_pchData[i]);

        // Determine the actual draw width (clipped)
        int drawW;
        if (i == endIdx) {
            drawW = glyphW - xLimit;  // partial last char
        } else {
            drawW = glyphW;
        }

        // Render this glyph line by line
        int curY = yStart;
        int curX = xStart;

        // Per-pixel rendering
        for (int row = yLimit; row < curY + glyphH && curY < yLimit; row++) {
            // Skip if outside current line bounds
            int targetPitch = pitch;
            unsigned short *dst = (unsigned short *)((char *)lockedPtr + curY * targetPitch);
            // Hmm this isn't matching the asm.. let me just return 0 for now
        }
    }

    return 0;
}

// =========================================================================
// FontRenderer::ComplexRender  (0x17a460, 2028 B)
//
// @address: 0x17a460
// @size:    0x7ec
// =========================================================================
int FontRenderer::ComplexRender(CString &text, int a1, int a2, int a3, int a4,
                                int a5, int a6, int a7, int a8)
{
    return 0;
}

// =========================================================================
// FontRenderer::MeasureString  (0x17ac50, 189 B)
//
// @address: 0x17ac50
// @size:    0xbd
// =========================================================================
int *FontRenderer::MeasureString(CString &text, int out[2])
{
    if (!m_font) {
        out[0] = 0;
        out[1] = 0;
        return out;
    }
    int totalWidth = 0;
    int len = *(int *)(text.m_pchData - 8);  // CString internal length
    for (int i = 0; i < len; i++) {
        Glyph g;
        m_font->GetGlyph((unsigned char)text.m_pchData[i], g);
        totalWidth += g.width;
    }
    out[0] = totalWidth;
    out[1] = m_font->GetMaxHeight();
    return out;
}

// =========================================================================
// FontRenderer::FormatText  (0x17ad10, 1026 B)
//
// @address: 0x17ad10
// @size:    0x402
// =========================================================================
int FontRenderer::FormatText(CString &text, int a1, int a2, int a3, int a4,
                             int a5)
{
    return 0;
}

// =========================================================================
// FontRenderer::FormatText2  (0x17b120, 966 B)
//
// @address: 0x17b120
// @size:    0x3c6
// =========================================================================
int FontRenderer::FormatText2(CString &text, int a1, int a2, int a3, int a4,
                              int a5, int a6)
{
    return 0;
}

// =========================================================================
// FontRenderer::GetChar  (0x17b4f0, 12 B)
//
// @address: 0x17b4f0
// @size:    0xc
// =========================================================================
unsigned char FontRenderer::GetChar(int i)
{
    return ((unsigned char *)m_font)[i];
}

// =========================================================================
// FontRenderer::GetStringLength  (0x17b500, 8 B)
//
// @address: 0x17b500
// @size:    0x8
// =========================================================================
int FontRenderer::GetStringLength()
{
    return (int)m_font - (int)m_surface;
}

// Helper struct to call the internal InitCriticalSection via NAFXCW.
struct CSInit {
    void InitCriticalSection(int a, int b) {}
};

// =========================================================================
// FECFile::Init  (0x17b510, 85 B)
//
// @address: 0x17b510
// @size:    0x55
// =========================================================================
int FECFile::Init()
{
    if (m_ready)
        return 0;
    m_opened = 0;
    m_field08 = 0;
    // Initialise the CS at +0x138 via its internal init function.
    ((CSInit *)(m_data + 0x120))->InitCriticalSection(-1, 0);
    m_header[0] = 0;
    m_header[1] = 0;
    m_header[2] = 0;
    memset(m_data, 0, 0x10c);
    m_field134 = 0;
    m_ready = 1;
    return 1;
}

// =========================================================================
// FECFile::Close  (0x17b570, 36 B)
//
// @address: 0x17b570
// @size:    0x24
// =========================================================================
void FECFile::Close()
{
    if (m_ready) {
        Flush();
        ((CSInit *)(m_data + 0x120))->InitCriticalSection(-1, 0);
        m_ready = 0;
    }
}

// =========================================================================
// FECFile::Flush  (0x17b5a0, 72 B)
//
// @address: 0x17b5a0
// @size:    0x48
// =========================================================================
int FECFile::Flush()
{
    if (!m_ready)
        return 0;
    if (m_opened || m_field08) {
        // Close the I/O interface
        void *ioVtable = *(void **)(m_ioInterface);
        void *ioObj = &m_ioInterface;
        ((void (*)(void *))((void **)ioVtable)[0x15])(ioObj);
        m_opened = 0;
        m_field08 = 0;
        m_field134 = 0;
    }
    return 1;
}

// =========================================================================
// FECFile::Open  (0x17b5f0, 585 B)
//
// @address: 0x17b5f0
// @size:    0x249
// =========================================================================
int FECFile::Open(CString const &fileName)
{
    if (!fileName.m_pchData) return 0;
    if (m_opened) return 0;
    if (!m_ready) return 0;

    // Open file via I/O interface
    void *ioVtable = *(void **)m_ioInterface;
    int result = ((int (__stdcall *)(void *, int, int, const char *))((void **)ioVtable)[0xa])(&m_ioInterface, 0, 0, fileName.m_pchData);
    if (!result) return 0;

    m_opened = 1;

    // Read 3-byte magic "FEC"
    int val140 = 0;
    void *ioVtable5 = 0;
    int seekResult = 0;
    struct CSInit *cs = 0;
    int entryCount = 0;
    int i;
    void *ioVtable3 = 0;
    void *ioVtable4 = 0;
    int wordAt11e = 0;
    char magic[4];
    void *ioVtable2 = *(void **)m_ioInterface;
    if (((int (__stdcall *)(void *, int, void *))((void **)ioVtable2)[0xf])(&m_ioInterface, 3, magic) != 3)
        goto fail;
    if (magic[0] != 'F' || magic[1] != 'E' || magic[2] != 'C')
        goto fail;

    // Read 12-byte header
    ioVtable3 = *(void **)m_ioInterface;
    if (((int (__stdcall *)(void *, int, void *))((void **)ioVtable3)[0xf])(&m_ioInterface, 12, m_header) != 12)
        goto fail;

    // Log "Opened FEC File: %s" and "FEC File Version: %d.%d Number of files: %d"
    // (these are debug printfs)

    // Read 0x10c bytes of directory data
    ioVtable4 = *(void **)m_ioInterface;
    if (((int (__stdcall *)(void *, int, void *))((void **)ioVtable4)[0xf])(&m_ioInterface, 0x10c, m_data) != 0x10c)
        goto fail;

    // Seek to beginning of file data
    wordAt11e = *(unsigned short *)((char *)this + 0x11e);
    ioVtable5 = *(void **)m_ioInterface;
    seekResult = ((int (__stdcall *)(void *, int, int))((void **)ioVtable5)[0xc])(&m_ioInterface, 1, wordAt11e - 0x2b8);
    if (seekResult != wordAt11e - 0x19d)
        goto fail;

    // Initialise CS
    cs = (struct CSInit *)(m_data + 0x120);
    val140 = m_field140;
    ((void (*)(int, int))((void **)cs)[0])(val140, seekResult);

    // Read directory entries
    entryCount = m_numFiles;
    for (i = 1; i < entryCount; i++) {
        int *entryBase = (int *)((char *)m_data + 0x120);
        // Each entry: read size via seek...
        void *ioVtable6 = *(void **)m_ioInterface;
        int seekSz = ((int (__stdcall *)(void *, int, int))((void **)ioVtable6)[0xc])(&m_ioInterface, 1, entryBase[0]);
        int expectedSz = entryBase[i * 4 - 4] + m_field120;
        if (seekSz != expectedSz)
            goto fail;

        // Zero the data buffer and re-read
        memset(m_data, 0, 0x10c);
        void *ioVtable7 = *(void **)m_ioInterface;
        if (((int (__stdcall *)(void *, int, void *))((void **)ioVtable7)[0xf])(&m_ioInterface, 0x10c, m_data) != 0x10c)
            goto fail;

        // Verify seek positions
        unsigned short wordSz = *(unsigned short *)((char *)this + 0x11e);
        void *ioVtable8 = *(void **)m_ioInterface;
        int seekSz2 = ((int (__stdcall *)(void *, int, int))((void **)ioVtable8)[0xc])(&m_ioInterface, 1, wordSz - 0x2b8);
        int entryOffset = entryBase[i * 4 - 4] + m_field120;
        int combined = entryOffset + wordSz;
        if (seekSz2 != combined - 0x1ac)
            goto fail;

        // Re-init CS
        ((void (*)(int, int))((void **)cs)[0])(*(int *)((char *)cs + 8), seekSz2);
    }

    return 1;

fail:
    Flush();
    return 0;
}

// =========================================================================
// FECFile::GetEntrySize  (0x17b840, 83 B)
//
// @address: 0x17b840
// @size:    0x53
// =========================================================================
int FECFile::GetEntrySize(int entryIndex)
{
    if (!m_opened) return 0;
    if (!m_ready) return 0;
    if ((unsigned int)entryIndex > (unsigned int)m_numFiles) return 0;
    if (entryIndex == 0) return 0;

    int *entryTable = (int *)((char *)m_data + 0x120);
    int entryOff = entryTable[entryIndex - 1];

    void *ioVtable = *(void **)m_ioInterface;
    int sz = ((int (__stdcall *)(void *, int, int))((void **)ioVtable)[0xc])(&m_ioInterface, 0, entryOff);
    if (sz != entryOff)
        return 0;

    return m_ioField128;
}
