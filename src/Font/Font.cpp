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
// Walk a linked list whose head lives at [this+0x20]; each node has layout
// { void *next; void *data; }.  Returns the data pointer of the first node
// whose IsInterface<which> returns true, or 0.
//
// @address: 0x179270
// @size:    0x89
// =========================================================================
int NodeList::FindObject(int which, void *listHead)
{
    (void)this;
    (void)listHead;
    return 0;
}

// =========================================================================
// CStringFromField  (0x179300, 32 B)
// Helper: copy-construct src into the CString at (this+8).
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
// @address: 0x179680
// @size:    0x3a
// =========================================================================
void CWapNodeB::FreeStrings()
{
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
    return 0;
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
    return 0;
}

// =========================================================================
// FontRenderer::Render  (0x179e70, 1516 B)
//
// @address: 0x179e70
// @size:    0x5ec
// =========================================================================
int FontRenderer::Render(CString &text, int a1, int a2, int a3, int a4,
                         int a5, int a6, int a7, int a8)
{
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
    return (int)m_surface - (int)m_font;
}

// =========================================================================
// FECFile::Init  (0x17b510, 85 B)
//
// @address: 0x17b510
// @size:    0x55
// =========================================================================
int FECFile::Init()
{
    return 0;
}

// =========================================================================
// FECFile::Close  (0x17b570, 36 B)
//
// @address: 0x17b570
// @size:    0x24
// =========================================================================
void FECFile::Close()
{
}

// =========================================================================
// FECFile::Flush  (0x17b5a0, 72 B)
//
// @address: 0x17b5a0
// @size:    0x48
// =========================================================================
int FECFile::Flush()
{
    return 0;
}

// =========================================================================
// FECFile::Open  (0x17b5f0, 585 B)
//
// @address: 0x17b5f0
// @size:    0x249
// =========================================================================
int FECFile::Open(CString const &fileName)
{
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
    return 0;
}
