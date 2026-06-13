#ifndef UTILS_FONT_H
#define UTILS_FONT_H

/*
 * Font — the bitmap-font loader for the four UI fonts (large/medium/small/tiny).
 * CGruntzMgr holds the four as statics (font_large/medium/small/tiny; see
 * ../game/cgruntzmgr.h) and builds them in CGruntzMgr::InitializeFonts().
 *
 * LAYOUT + .fnt FILE FORMAT PORTED FROM tomalla (refs/tomalla-gruntz/gruntz/font.h
 * + font.cpp). @approx tomalla 1.0.1.77 — field OFFSETS and the on-disk .fnt
 * layout are version-independent. Function addresses (Font::Font/AllocateMemory/
 * FreeMemory/LoadFont) are 1.0.1.77 and are deferred to the re-anchor (not here).
 *
 * Provenance: not in RTTI; "Font" is a tomalla-invented name for a real binary
 * class with matched fields and a fully reconstructed loader.
 *
 * .fnt on-disk format (little-endian, read via MFC CArchive):
 *
 *   struct FntFile {           // the whole file
 *     uint32_t lettersCount;   // @0x0
 *     Letter   letters[lettersCount];
 *   };
 *   struct Letter {
 *     uint32_t width;          // @0x0
 *     uint32_t height;         // @0x4
 *     uint8_t  pixelData[width*height];  // @0x8  (1 byte/pixel, palette index)
 *   };
 *
 * @bug (faithfully noted by tomalla): the loader does no bounds-checking on the
 * archive reads (a corrupt/short file will crash); the leading `>>` may throw.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>   /* CString, CArchive, CFile */

class Font
{
public:
    //@size: 0x14   @approx tomalla 1.0.1.77 (offsets version-independent)
    Font();
    ~Font();

    void FreeMemory();
    bool LoadFont(CString szFileName);

private:
    bool AllocateMemory(int lettersCount);

    // Per-letter glyph dimensions (parsed straight from the .fnt Letter header).
    struct Size
    {
        int width;
        int height;
    };

    //@offset: 0
    bool m_isMemoryAllocated;
    //@offset: 4
    int m_lettersCount;
    //@offset: 8
    char** m_pPixelData;     // [m_lettersCount] glyph pixel rows (1 byte/pixel)
    //@offset: c
    Size* m_pLettersSize;    // [m_lettersCount] glyph {width,height}
    //@offset: 10
    int m_fontHeight;        // max glyph height across all letters
    // (total 0x14)
};

#endif /* UTILS_FONT_H */
