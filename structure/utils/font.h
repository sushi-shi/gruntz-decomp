#ifndef UTILS_FONT_H
#define UTILS_FONT_H

/*
 * Font — the bitmap-font loader for the four UI fonts (large/medium/small/tiny).
 * CGruntzMgr holds the four as statics (font_large/medium/small/tiny; see
 * ../game/cgruntzmgr.h) and builds them in CGruntzMgr::InitializeFonts().
 *
 * Layout + the .fnt on-disk format ported from tomalla (@approx tomalla 1.0.1.77;
 * offsets version-independent). Not in RTTI; "Font" is a tomalla-invented name for
 * a real binary class with matched fields and a fully reconstructed loader.
 *
 * .fnt on-disk format (little-endian, read via MFC CArchive):
 *   FntFile { uint32 lettersCount; Letter letters[lettersCount]; }
 *   Letter  { uint32 width; uint32 height; uint8 pixelData[width*height]; }
 *
 * @bug (faithfully noted by tomalla): the loader does no bounds-checking on the
 * archive reads (a corrupt/short file will crash); the leading `>>` may throw.
 *
 * Size 0x14.  (CString in LoadFont's real signature is an MFC type, modeled here
 * as an opaque pointer so the header parses without <afxwin.h>; it carries no
 * layout weight.)
 */

class Font
{
public:
    Font();
    ~Font();

    void FreeMemory();
    bool LoadFont(void *szFileName /* MFC CString by value in the binary */);

private:
    bool AllocateMemory(int lettersCount);

    // Per-letter glyph dimensions (parsed straight from the .fnt Letter header).
    struct Size
    {
        int width;
        int height;
    };

    bool   m_isMemoryAllocated; // +0x00  (1 byte; 3 bytes pad to the next dword)
    int    m_lettersCount;      // +0x04
    char **m_pPixelData;        // +0x08  [m_lettersCount] glyph pixel rows
    Size  *m_pLettersSize;      // +0x0c  [m_lettersCount] glyph {width,height}
    int    m_fontHeight;        // +0x10  max glyph height across all letters
};                              // 0x14 bytes

#endif /* UTILS_FONT_H */
