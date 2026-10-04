#ifndef SRC_FONT_FONT_H
#define SRC_FONT_FONT_H

#include <rva.h>

class Font {
public:
    Font();
    ~Font();
    i32 AllocateMemory(i32 glyphCount);
    void FreeMemory();
    i32 LoadFont(CString szFileName);
    i32 SaveFont(CString szFileName);

    u8** GetGlyphBitmap(u8 character);
    CSize& GetGlyphSize(CSize& sizeOut, u8 character);
    void SetGlyphSize(u8 character, CSize glyphSize);
    i32 GetMaxHeight();

    b32 m_storageAllocated;
    i32 m_glyphCount;
    u8** m_glyphBitmaps;
    CSize* m_glyphSizes;
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

    CSize MeasureText(CString text);

    void DrawGlyphRun(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend);

    void DrawLine(CString text, CDDSurface* surf, i32 x, i32 y, i32 z);
    void DrawLineClipped(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 z);

    CSize MeasureWrapped(CString text, CRect rc);

    void DrawWrapped(CString text, CDDSurface* surf, CRect rc, i32 z, i32 hcenter, i32 spacing);

    CSize LayoutWrapped(CString text, CRect rc, i32* outLen);

    Font* m_font;
    COLORREF m_color;

    i32 m_shadowEnabled;
    i32 m_highlightEnabled;
};

#define SET_FONT_GLYPH_SIZE(character, glyphSize) m_glyphSizes[character] = glyphSize

#endif // SRC_FONT_FONT_H
