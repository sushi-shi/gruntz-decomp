// Font.cpp - the engine's bitmap Font class (the text/glyph subsystem). Each
// Font owns a per-letter table of glyph metrics (m_glyphs[], {width,height})
// and a parallel table of decoded pixel surfaces (m_surfaces[]), loaded from a
// binary font file through the MFC CFile/CArchive I/O stack.
//
// Matched leaf methods (the font-resource lifecycle):
//   Font::Font           - the empty ctor (all four members 0).
//   Font::AllocateMemory - (re)allocate the two parallel tables
//                          for `count` glyphs, zero them.
//   Font::FreeMemory     - free every glyph surface + both
//                          tables and reset to the empty state.
//   Font::LoadFont       - open the font file, read the glyph
//                          count + metric table via a CArchive,
//                          decode each glyph's pixel surface,
//                          and compute the line-height.
//
// Built /O2 /MT /GX: LoadFont holds stack CFile/CArchive/CString objects whose
// destructors run under a C++ EH frame (the target opens an fs:0 SEH/EH frame:
// `push -1; push &handler; mov fs:0`). The NAFXCW callees (operator new/delete,
// CFile::*, CArchive::*, CString::~CString) are external/no-body so their
// `call rel32` displacements reloc-mask in objdiff.
#include "Font.h"
#include "../rva.h"
#include <string.h>   // memcmp (InterfaceObject::IsInterfaceX)

// GUIDs for DirectDraw-style interface checks (InterfaceObject::IsInterfaceX).
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
// The empty constructor: zero both table pointers and the {ready,count} pair.
// The store order (m_surfaces, m_glyphs, m_ready, m_count) reproduces MSVC's
// schedule (it writes +0x8/+0xc before +0x0/+0x4).
RVA(0x179700, 0x10)
Font::Font()
{
    m_surfaces = 0;
    m_glyphs = 0;
    m_ready = 0;
    m_count = 0;
}

// ---------------------------------------------------------------------------
// Font::AllocateMemory
// Free any prior tables, then allocate the surface-pointer table (count*4) and
// the glyph-metric table (count*8) and zero every entry. Returns 1 on success,
// 0 when count < 1. (No allocation-failure path is taken - operator new throws
// / returns the buffer; the null-test on the glyph table is the source's own.)
RVA(0x179720, 0x87)
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
// Release everything the tables hold: each glyph's pixel surface, then the
// surface-pointer table, then the glyph-metric table, then reset to empty. A
// no-op when m_ready is already 0.
RVA(0x1797b0, 0x71)
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
// Open the named binary font file, read the glyph count + the metric table via
// a CArchive, decode each glyph's pixel surface into m_surfaces[i] (width*height
// bytes read straight from the archive), then compute m_maxHeight (the font
// line-height = the tallest glyph). Returns 0 if the file fails to open, 1 once
// the font is fully populated.
RVA(0x179830, 0x1b1)
int Font::LoadFont(CString szFileName)
{
    FreeMemory();

    CFile file;
    if (!file.Open(szFileName.m_pchData, 0, 0)) {
        return 0;
    }

    CArchive ar(&file, 1 /*CArchive::load*/, 0x1000, 0);

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
// IsInterface1
//
RVA(0x1794b0, 0x21)
int InterfaceObject::IsInterface1()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid1, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface2
//
RVA(0x1794e0, 0x21)
int InterfaceObject::IsInterface2()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid2, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface3
//
RVA(0x179510, 0x21)
int InterfaceObject::IsInterface3()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid3, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface4
//
RVA(0x179540, 0x21)
int InterfaceObject::IsInterface4()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid4, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface5
//
RVA(0x179570, 0x21)
int InterfaceObject::IsInterface5()
{
    if (!iid)
        return 0;
    return memcmp(iid, g_guid5, 16) == 0 ? 1 : 0;
}

// =========================================================================
// CWapNodeB::FreeStrings
//
// Frees two allocated buffers at +0x34 and +0x38 and clears m_type.
//
RVA(0x179680, 0x3a)
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
// Font::GetSurface
//
RVA(0x179b60, 0x12)
void **Font::GetSurface(unsigned char c)
{
    return &m_surfaces[c];
}

// =========================================================================
// Font::GetGlyph
//
RVA(0x179b80, 0x22)
void Font::GetGlyph(unsigned char c, Glyph &out)
{
    out = m_glyphs[c];
}

// =========================================================================
// Font::GetMaxHeight
//
RVA(0x179bd0, 0x4)
int Font::GetMaxHeight()
{
    return m_maxHeight;
}

// =========================================================================
// FontRenderer::FontRenderer
//
RVA(0x179be0, 0x14)
FontRenderer::FontRenderer()
{
    m_font = 0;
    m_color = 0x00ffffff;
    m_clip = 0;
    m_surface = 0;
}

// =========================================================================
// FontRenderer::SetColor
// Internal helper: stores the color argument into m_color.
//
RVA(0x179c20, 0xa)
void FontRenderer::SetColor(int color)
{
    m_color = color;
}

// =========================================================================
// FontRenderer::GetChar
//
RVA(0x17b4f0, 0xc)
unsigned char FontRenderer::GetChar(int i)
{
    return ((unsigned char *)m_font)[i];
}
