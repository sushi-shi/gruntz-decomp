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
#include <Font/Font.h>
#include <rva.h>
#include <string.h> // memcmp (InterfaceObject::IsInterfaceX)

// GUIDs for DirectDraw-style interface checks (InterfaceObject::IsInterfaceX).
// clang-format off
const u8 g_guid1[16] = {0x00, 0xc4, 0x5b, 0x68, 0x2c, 0x9d, 0xcf, 0x11,
                                   0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3};
const u8 g_guid2[16] = {0xe0, 0x5e, 0xe9, 0x36, 0x77, 0x85, 0xcf, 0x11,
                                   0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82};
const u8 g_guid3[16] = {0x60, 0xa7, 0xea, 0x44, 0x68, 0xcb, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
const u8 g_guid4[16] = {0x60, 0x68, 0x1d, 0x0f, 0xd9, 0x88, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
const u8 g_guid5[16] = {0x00, 0xb4, 0x23, 0xd2, 0x7d, 0x0a, 0xd1, 0x11,
                                   0x90, 0xc3, 0x00, 0x60, 0x97, 0x72, 0x58, 0x40};
// clang-format on

// ---------------------------------------------------------------------------
// Font::Font
// The empty constructor: zero both table pointers and the {ready,count} pair.
// The store order (m_surfaces, m_glyphs, m_ready, m_count) reproduces MSVC's
// schedule (it writes +0x8/+0xc before +0x0/+0x4).
RVA(0x00179700, 0x10)
Font::Font() {
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
RVA(0x00179720, 0x87)
i32 Font::AllocateMemory(i32 count) {
    FreeMemory();

    m_count = count;
    if (count < 1) {
        return 0;
    }

    m_surfaces = (void**)operator new(m_count * sizeof(void*));
    m_glyphs = new Glyph[m_count];

    for (i32 i = 0; i < m_count; i++) {
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
RVA(0x001797b0, 0x71)
void Font::FreeMemory() {
    if (m_ready) {
        for (i32 i = 0; i < m_count; i++) {
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
RVA(0x00179830, 0x1b1)
i32 Font::LoadFont(CString szFileName) {
    FreeMemory();

    CFile file;
    if (!file.Open((const char*)szFileName, 0, 0)) {
        return 0;
    }

    CArchive ar(&file, 1 /*CArchive::load*/, 0x1000, 0);

    ar >> m_count;
    AllocateMemory(m_count);

    for (i32 i = 0; i < m_count; i++) {
        ar.Read(&m_glyphs[i], sizeof(Glyph));
        m_surfaces[i] = operator new(m_glyphs[i].width * m_glyphs[i].height);
        ar.Read(m_surfaces[i], m_glyphs[i].width * m_glyphs[i].height);
    }

    ar.Close();
    file.Close();

    i32 maxHeight = 0;
    for (i32 j = 0; j < m_count; j++) {
        if (maxHeight <= m_glyphs[j].height) {
            maxHeight = m_glyphs[j].height;
        }
    }
    m_maxHeight = maxHeight;

    return 1;
}

// =========================================================================
// IsInterface1
//
RVA(0x001794b0, 0x21)
i32 InterfaceObject::IsInterface1() {
    if (!iid) {
        return 0;
    }
    return memcmp(iid, g_guid1, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface2
//
RVA(0x001794e0, 0x21)
i32 InterfaceObject::IsInterface2() {
    if (!iid) {
        return 0;
    }
    return memcmp(iid, g_guid2, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface3
//
RVA(0x00179510, 0x21)
i32 InterfaceObject::IsInterface3() {
    if (!iid) {
        return 0;
    }
    return memcmp(iid, g_guid3, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface4
//
RVA(0x00179540, 0x21)
i32 InterfaceObject::IsInterface4() {
    if (!iid) {
        return 0;
    }
    return memcmp(iid, g_guid4, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface5
//
RVA(0x00179570, 0x21)
i32 InterfaceObject::IsInterface5() {
    if (!iid) {
        return 0;
    }
    return memcmp(iid, g_guid5, 16) == 0 ? 1 : 0;
}

// =========================================================================
// CWapNodeB::FreeStrings
//
// Frees two allocated buffers at +0x34 and +0x38 and clears m_type.
//
RVA(0x00179680, 0x3a)
void CWapNodeB::FreeStrings() {
    if (m_buf34) {
        operator delete(m_buf34);
        m_buf34 = 0;
    }
    if (m_buf38) {
        operator delete(m_buf38);
        m_buf38 = 0;
    }
    m_type = 0;
}

// =========================================================================
// Font::GetSurface
//
RVA(0x00179b60, 0x12)
void** Font::GetSurface(u8 c) {
    return &m_surfaces[c];
}

// =========================================================================
// Font::GetGlyph
//
RVA(0x00179b80, 0x22)
void Font::GetGlyph(u8 c, Glyph& out) {
    out = m_glyphs[c];
}

// =========================================================================
// Font::GetMaxHeight
//
RVA(0x00179bd0, 0x4)
i32 Font::GetMaxHeight() {
    return m_maxHeight;
}

// =========================================================================
// FontRenderer::FontRenderer
//
RVA(0x00179be0, 0x14)
FontRenderer::FontRenderer() {
    m_font = 0;
    m_color = 0x00ffffff;
    m_clip = 0;
    m_surface = 0;
}

// =========================================================================
// FontRenderer::SetColor
// Internal helper: stores the color argument into m_color.
//
RVA(0x00179c20, 0xa)
void FontRenderer::SetColor(i32 color) {
    m_color = color;
}

// =========================================================================
// FontRenderer::GetChar
//
RVA(0x0017b4f0, 0xc)
u8 FontRenderer::GetChar(i32 i) {
    return ((u8*)m_font)[i];
}

// =========================================================================
// FontRenderer::MeasureText
// Sum the advance widths of every glyph in `text` and pair it with the font's
// line-height. With no font loaded the extent is {0,0}. The CString arg is
// taken by value (the EH frame destroys it); the result is returned by value.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): retail
// pins esi=0 and reuses it for the null-branch result stores, the EH-state
// writes and the length compares (+ one dead `mov [esp+0x10],esi` spill); cl
// allocates ecx for the zero, cascading a 1-instr regalloc shift. Body/offsets
// byte-exact; logic complete.
RVA(0x0017ac50, 0xbd)
TextExtent FontRenderer::MeasureText(CString text) {
    TextExtent ext;
    i32 i = 0;
    i32 width = 0;
    if (m_font == 0) {
        ext.width = 0;
        ext.height = 0;
        return ext;
    }
    for (i = 0; i < text.GetLength(); i++) {
        Glyph g;
        u8 c = ((const u8*)(const char*)text)[i];
        m_font->GetGlyph(c, g);
        width += g.width;
    }
    ext.width = width;
    ext.height = m_font->GetMaxHeight();
    return ext;
}

// =========================================================================
// FontRenderer::DrawLineClipped
// Draw one text run with up to two shadow passes: a white pass offset by
// (+1,+1) when m_clip is set, a black pass offset by (.,+2) when m_surface is
// set, then the main pass in m_color. Each pass forwards the same rect+text to
// the inner glyph-blit (DrawGlyphRun, external). The CString is taken by value
// (destroyed by the EH frame).
RVA(0x00179d10, 0x15c)
void FontRenderer::DrawLineClipped(CString text, i32 a1, Rect rc, i32 x, i32 y, i32 z) {
    i32 savedColor = m_color;
    if (m_clip) {
        SetColor(0xffffff);
        DrawGlyphRun(text, a1, rc, x, y, z);
        x++;
        y++;
    }
    if (m_surface) {
        SetColor(0);
        x += 2;
        DrawGlyphRun(text, a1, rc, x, y, z);
        x -= 2;
    }
    SetColor(savedColor);
    DrawGlyphRun(text, a1, rc, x, y, z);
}

// =========================================================================
// FontRenderer::Stub_17a460  ==  DrawWrapped (~2 KB), the cluster's largest.
// Word-wrap layout + draw: greedily breaks the run into lines (measuring with
// MeasureText) and draws each via DrawLine. Deferred to the final sweep - a
// CString-temp-heavy /GX body (Left/Mid/fill, ~6 nested temps with cycling EH
// states) that needs a leaf-first redo to converge past the eh-state-numbering
// wall. Backlog stub retained, RVA-tracked under its real class (FontRenderer).
// @confidence: high
// @source: this-trace
// @stub
RVA(0x0017a460, 0x7ec)
void FontRenderer::Stub_17a460() {}

// =========================================================================
// FontRenderer::Stub_17ad10  ==  MeasureWrapped (~1 KB).
// Greedy word-wrap bounding-box measurer: returns {maxLineWidth, totalHeight}.
// Same CString-temp / EH-state density as DrawWrapped; deferred to the final
// sweep for a leaf-first redo. Backlog stub retained, RVA-tracked.
// @confidence: high
// @source: this-trace
// @stub
RVA(0x0017ad10, 0x402)
void FontRenderer::Stub_17ad10() {}

// =========================================================================
// FontRenderer::DrawLine
// Public single-line entry: measure the run, reject it if it would overflow
// the box's vertical limit (p->m_bottom), otherwise build the destination Rect
// and hand off to DrawLineClipped. No-op when no font is loaded.
// @early-stop
// arg-bundle/frame wall: the 9-dword DrawLineClipped call is assembled from a
// heterogeneous bundle (the 029ac0 Rect result + p + a Rect field + the CString
// temp) whose exact field grouping isn't fully recovered; retail reserves 2
// extra stack dwords, skewing every [esp+N]. Control flow + measure/clip/draw
// logic are exact; residual is push scheduling + frame size (~85%).
RVA(0x00179c30, 0xdb)
void FontRenderer::DrawLine(DrawRect* p, i32 x, i32 y, CString text, i32 a4) {
    TextExtent ext = MeasureText(text);
    if (m_font == 0) {
        return;
    }
    i32 limit = p->m_bottom;
    if (m_font->GetMaxHeight() + y > limit) {
        return;
    }
    DrawLineClipped(text, a4, Rect(0, 0, x, y), x, y, p->left);
}
