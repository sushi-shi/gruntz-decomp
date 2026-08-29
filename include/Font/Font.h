#ifndef SRC_FONT_FONT_H
#define SRC_FONT_FONT_H

#include <rva.h>

#include <MfcWin.h>

struct Glyph {
    Glyph() {}
    i32 width;
    i32 height;
};

class Font {
public:
    Font();
    ~Font();
    i32 AllocateMemory(i32 count);
    void FreeMemory();
    i32 LoadFont(CString szFileName);
    i32 SaveFont(CString szFileName);

    u8** GetSurface(u8 c);
    Glyph& GetGlyph(Glyph& out, u8 c);
    void SetGlyph(u8 c, Glyph glyph);
    i32 GetMaxHeight();

    b32 m_ready;
    i32 m_count;
    u8** m_surfaces;
    Glyph* m_glyphs;
    i32 m_maxHeight;
};

extern Font g_largeFont;
extern Font g_mediumFont;
extern Font g_smallFont;
extern Font g_tinyFont;

struct TextExtent {
    TextExtent() {}
    TextExtent(i32 w, i32 h) {
        width = w;
        height = h;
    }
    i32 width;
    i32 height;
};

class CDDSurface;

class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();
    void SetFont(Font* f);
    void SetColor(i32 color);

    TextExtent MeasureText(CString text);

    void DrawGlyphRun(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend);

    void DrawLine(CString text, CDDSurface* surf, i32 x, i32 y, i32 z);
    void DrawLineClipped(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 z);

    TextExtent MeasureWrapped(CString text, CRect rc);

    void DrawWrapped(CString text, CDDSurface* surf, CRect rc, i32 z, i32 hcenter, i32 spacing);

    TextExtent LayoutWrapped(CString text, CRect rc, i32* outLen);

    Font* m_font;
    COLORREF m_color;

    i32 m_surface;
    i32 m_clip;
};

#endif // SRC_FONT_FONT_H
