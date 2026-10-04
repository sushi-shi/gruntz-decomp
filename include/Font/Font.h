#ifndef SRC_FONT_FONT_H
#define SRC_FONT_FONT_H

#include <string>

#include <Ints.h>

class Font {
public:
    Font();
    ~Font();
    i32 AllocateMemory(i32 count);
    void FreeMemory();
    i32 LoadFont(const std::string& szFileName);
    i32 SaveFont(const std::string& szFileName);

    u8** GetSurface(u8 c);
    CSize& GetGlyph(CSize& out, u8 c);
    void SetGlyph(u8 c, CSize glyph);
    i32 GetMaxHeight();

    b32 m_ready;
    i32 m_count;
    u8** m_surfaces;
    CSize* m_glyphs;
    i32 m_maxHeight;
};

extern Font g_largeFont;
extern Font g_mediumFont;
extern Font g_smallFont;
extern Font g_tinyFont;

class CDDSurface;

class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();
    void SetFont(Font* f);
    void SetColor(i32 color);

    CSize MeasureText(const std::string& text);

    void DrawGlyphRun(const std::string& text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend);

    void DrawLine(const std::string& text, CDDSurface* surf, i32 x, i32 y, i32 z);
    void DrawLineClipped(const std::string& text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 z);

    CSize MeasureWrapped(std::string text, CRect rc);

    void DrawWrapped(std::string text, CDDSurface* surf, CRect rc, i32 z, i32 hcenter, i32 spacing);

    CSize LayoutWrapped(std::string text, CRect rc, i32* outLen);

    Font* m_font;
    COLORREF m_color;

    i32 m_surface;
    i32 m_clip;
};

#define SET_FONT_GLYPH(c, glyph) m_glyphs[c] = glyph

#endif
