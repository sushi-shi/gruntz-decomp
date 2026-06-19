// Font.cpp - the engine's bitmap Font class (the text/glyph subsystem). Each
// Font owns a per-letter table of glyph metrics (m_glyphs[], {width,height})
// and a parallel table of decoded pixel surfaces (m_surfaces[]), loaded from a
// binary font file through the MFC CFile/CArchive I/O stack.
//
// Matched leaf methods (the font-resource lifecycle):
//   Font::Font          @0x179700 (16 B)  - the empty ctor (all four members 0).
//   Font::AllocateMemory@0x179720 (135 B) - (re)allocate the two parallel tables
//                                            for `count` glyphs, zero them.
//   Font::FreeMemory    @0x1797b0 (113 B) - free every glyph surface + both
//                                            tables and reset to the empty state.
//   Font::LoadFont      @0x179830 (433 B) - open the font file, read the glyph
//                                            count + metric table via a CArchive,
//                                            decode each glyph's pixel surface,
//                                            and compute the line-height.
//
// Built /O2 /MT /GX: LoadFont holds stack CFile/CArchive/CString objects whose
// destructors run under a C++ EH frame (the target opens an fs:0 SEH/EH frame:
// `push -1; push &handler; mov fs:0`). The NAFXCW callees (operator new/delete,
// CFile::*, CArchive::*, CString::~CString) are external/no-body so their
// `call rel32` displacements reloc-mask in objdiff.
#include "Font.h"
#include "../rva.h"

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
