#include <StdAfx.h>

#include <rva.h>

#include <Font/Font.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Font/FontBlendInline.h>
#include <RectMacros.h>
#include <SafeDelete.h>
#include <Wap32/TileGeometry.h>

#include <ddraw.h>
#include <limits.h>

RVA(0x00179700, 0x10)
Font::Font() {
    m_glyphBitmaps = NULL;
    m_glyphSizes = NULL;
    m_storageAllocated = false;
    m_glyphCount = 0;
}

RVA(0x00179710, 0x5)
Font::~Font() {
    FreeMemory();
}

RVA(0x00179720, 0x87)
i32 Font::AllocateMemory(i32 glyphCount) {
    FreeMemory();

    m_glyphCount = glyphCount;
    if (glyphCount < 1) {
        return 0;
    }

    m_glyphBitmaps = new u8*[m_glyphCount];
    m_glyphSizes = new CSize[m_glyphCount];

    for (i32 i = 0; i < m_glyphCount; i++) {
        m_glyphBitmaps[i] = NULL;

        CSize glyphSize(0, 0);
        SET_FONT_GLYPH_SIZE(i, glyphSize);
    }

    m_maxHeight = 0;
    m_storageAllocated = true;
    return 1;
}

RVA(0x001797b0, 0x71)
void Font::FreeMemory() {
    if (m_storageAllocated) {
        for (i32 i = 0; i < m_glyphCount; i++) {
            if (m_glyphBitmaps[i]) {
                delete[] m_glyphBitmaps[i];
                m_glyphBitmaps[i] = NULL;
            }
        }
        delete[] m_glyphBitmaps;
        m_glyphBitmaps = NULL;
        SAFE_DELETE_ARRAY(m_glyphSizes);
        m_glyphCount = 0;
        m_storageAllocated = false;
    }
}

RVA(0x00179830, 0x1b1)
i32 Font::LoadFont(CString szFileName) {
    FreeMemory();

    CFile file;
    if (!file.Open(szFileName, CFile::modeRead, NULL)) {
        return 0;
    }

    CArchive ar(&file, CArchive::load, 0x1000, NULL);

    ar >> m_glyphCount;
    AllocateMemory(m_glyphCount);

    for (i32 i = 0; i < m_glyphCount; i++) {
        ar.Read(&m_glyphSizes[i], sizeof(CSize));
        m_glyphBitmaps[i] = new u8[m_glyphSizes[i].cx * m_glyphSizes[i].cy];
        ar.Read(m_glyphBitmaps[i], m_glyphSizes[i].cx * m_glyphSizes[i].cy);
    }

    ar.Close();
    file.Close();

    i32 maxHeight = 0;
    for (i32 j = 0; j < m_glyphCount; j++) {
        maxHeight = max(maxHeight, m_glyphSizes[j].cy);
    }
    m_maxHeight = maxHeight;

    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001799f0, 0x16d)
i32 Font::SaveFont(CString szFileName) {
    CFile file;
    if (!file.Open(szFileName, CFile::modeCreate | CFile::modeWrite, NULL)) {
        return 0;
    }

    CArchive ar(&file, CArchive::store, 0x1000, NULL);

    ar << m_glyphCount;

    for (i32 i = 0; i < m_glyphCount; i++) {
        CSize glyphSize = m_glyphSizes[i];
        ar.Write(&glyphSize, sizeof(CSize));
        ar.Write(m_glyphBitmaps[i], m_glyphSizes[i].cx * m_glyphSizes[i].cy);
    }

    ar.Close();
    file.Close();

    return 1;
}

RVA(0x00179b60, 0x12)
u8** Font::GetGlyphBitmap(u8 character) {
    return &m_glyphBitmaps[character];
}

RVA(0x00179b80, 0x22)
CSize& Font::GetGlyphSize(CSize& sizeOut, u8 character) {
    sizeOut = m_glyphSizes[character];
    return sizeOut;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179bb0, 0x1e)
void Font::SetGlyphSize(u8 character, CSize glyphSize) {
    SET_FONT_GLYPH_SIZE(character, glyphSize);
}

RVA(0x00179bd0, 0x4)
i32 Font::GetMaxHeight() {
    return m_maxHeight;
}

RVA(0x00179be0, 0x14)
FontRenderer::FontRenderer() {
    m_font = NULL;
    m_color = RGB(255, 255, 255);
    m_highlightEnabled = 0;
    m_shadowEnabled = 0;
}

RVA(0x00179c00, 0x1)
FontRenderer::~FontRenderer() {}

RVA(0x00179c10, 0x9)
void FontRenderer::SetFont(Font* font) {
    m_font = font;
}

RVA(0x00179c20, 0xa)
void FontRenderer::SetColor(i32 color) {
    m_color = color;
}

RVA(0x00179c30, 0xdb)
void FontRenderer::DrawLine(CString text, CDDSurface* surface, i32 x, i32 y, i32 blendGlyphs) {
    CSize textExtent = MeasureText(text);
    if (m_font == NULL) {
        return;
    }
    i32 surfaceHeight = surface->GetHeight();
    if (m_font->GetMaxHeight() + y > surfaceHeight) {
        return;
    }
    DrawLineClipped(text, surface, CRect(0, 0, textExtent.cx, textExtent.cy), x, y, blendGlyphs);
}

RVA(0x00179d10, 0x15c)
void FontRenderer::DrawLineClipped(
    CString text,
    CDDSurface* surface,
    CRect clipRect,
    i32 x,
    i32 y,
    i32 blendGlyphs
) {
    i32 savedColor = m_color;
    if (m_highlightEnabled) {
        SetColor(RGB(255, 255, 255));
        DrawGlyphRun(text, surface, clipRect, x, y, blendGlyphs);
        x++;
        y++;
    }
    if (m_shadowEnabled) {
        SetColor(RGB(0, 0, 0));
        x += 2;
        DrawGlyphRun(text, surface, clipRect, x, y, blendGlyphs);
        x -= 2;
    }
    SetColor(savedColor);
    DrawGlyphRun(text, surface, clipRect, x, y, blendGlyphs);
}

RVA(0x00179e70, 0x5ec)
void FontRenderer::DrawGlyphRun(
    CString text,
    CDDSurface* surface,
    CRect clipRect,
    i32 x,
    i32 y,
    i32 blendGlyphs
) {
    if (m_font == NULL) {
        return;
    }
    if (clipRect.left < 0) {
        return;
    }
    if (clipRect.top < 0) {
        return;
    }
    if (x < 0) {
        return;
    }
    if (y < 0) {
        return;
    }

    if (RunRightEdge(clipRect, x) > surface->GetWidth()) {
        clipRect.right = clipRect.right + clipRect.Width() + x - surface->GetWidth();
    }
    if (y + clipRect.Height() > surface->GetHeight()) {
        clipRect.bottom = clipRect.bottom + clipRect.Height() + y - surface->GetHeight();
    }

    CSize measuredExtent = MeasureText(text);
    {
        CRect textBounds(CPoint(0, 0), measuredExtent);
        if (!clipRect.IntersectRect(&clipRect, &textBounds)) {
            return;
        }
    }
    CLAMP_UPPER_INPLACE(clipRect.right, measuredExtent.cx);
    CLAMP_UPPER_INPLACE(clipRect.bottom, measuredExtent.cy);

    u16* pixels = static_cast<u16*>(surface->Lock(NULL));
    i32 pitchBytes = surface->m_apiDesc.lPitch;
    if (pixels == NULL) {
        return;
    }

    CPoint drawPosition(x, y);
    i32 cumulativeWidth = 0;
    i32 red = GetRValue(m_color);
    i32 green = GetGValue(m_color);
    i32 blue = GetBValue(m_color);
    u16 packedColor =
        (static_cast<u8>((static_cast<u8>(red) >> static_cast<u8>(g_rDown))) << g_rUp)
        | (static_cast<u8>((static_cast<u8>(green) >> static_cast<u8>(g_gDown))) << g_gUp)
        | (static_cast<u8>(blue) >> static_cast<u8>(g_bDown));

    i32 rightClipPixels = 0;
    i32 firstGlyphColumn = 0;
    i32 firstCharacterIndex;
    if (clipRect.left != 0) {
        i32 previousWidth = 0;
        firstCharacterIndex = 0;
        while (cumulativeWidth < clipRect.left) {
            CSize glyphExtent;
            previousWidth = cumulativeWidth;
            cumulativeWidth += m_font->GetGlyphSize(glyphExtent, text[firstCharacterIndex]).cx;
            firstCharacterIndex++;
        }
        --firstCharacterIndex;
        firstGlyphColumn = clipRect.left - previousWidth;
    } else {
        firstCharacterIndex = 0;
    }

    i32 endCharacterIndex;
    cumulativeWidth = 0;

    if (clipRect.right != measuredExtent.cx) {
        i32 scanIndex = 0;
        endCharacterIndex = 0;
        if (clipRect.right >= 0) {
            do {
                CSize glyphExtent;
                cumulativeWidth += m_font->GetGlyphSize(glyphExtent, text[scanIndex]).cx;
                scanIndex++;
            } while (cumulativeWidth <= clipRect.right);
            endCharacterIndex = scanIndex;
        }
        rightClipPixels = cumulativeWidth - clipRect.right;
    } else {
        endCharacterIndex = text.GetLength();
    }

    for (i32 characterIndex = firstCharacterIndex; characterIndex < endCharacterIndex;
         characterIndex++) {
        CSize glyphExtent;
        measuredExtent = m_font->GetGlyphSize(glyphExtent, text[characterIndex]);
        i32 row;
        i32 col;
        i32 clippedGlyphWidth;
        if (characterIndex == endCharacterIndex - 1) {
            clippedGlyphWidth = measuredExtent.cx - rightClipPixels;
        } else {
            clippedGlyphWidth = measuredExtent.cx;
        }
        u8* glyphBitmap = m_font->GetGlyphBitmap(text[characterIndex])[0];
        if (blendGlyphs) {
            for (row = clipRect.top; row < clipRect.bottom; row++) {
                u16* destination = pixels + ((row - clipRect.top + drawPosition.y) * pitchBytes) / 2
                                   + drawPosition.x;
                for (col = firstGlyphColumn; col < clippedGlyphWidth; col++) {
                    u8 coverage = glyphBitmap[row * measuredExtent.cx + col];

                    if (coverage == 0) {
                    } else if (coverage != UCHAR_MAX) {
                        *destination = BlendPixel16(*destination, coverage, red, green, blue);
                    } else {
                        *destination = packedColor;
                    }
                    destination++;
                }
            }
        } else {
            for (row = clipRect.top; row < clipRect.bottom; row++) {
                u16* destination = pixels + ((row - clipRect.top + drawPosition.y) * pitchBytes) / 2
                                   + drawPosition.x;
                for (col = firstGlyphColumn; col < clippedGlyphWidth; col++) {
                    if (glyphBitmap[row * measuredExtent.cx + col] != 0) {
                        *destination = packedColor;
                    }
                    destination++;
                }
            }
        }
        drawPosition.x += clippedGlyphWidth - firstGlyphColumn;
        firstGlyphColumn = 0;
    }

    surface->Unlock();
}

RVA(0x0017a460, 0x7ec)
void FontRenderer::DrawWrapped(
    CString remainingText,
    CDDSurface* surface,
    CRect bounds,
    i32 blendGlyphs,
    i32 centerText,
    i32 lineSpacing
) {
    i32 lineAdvance = m_font->GetMaxHeight() + lineSpacing;
    if (centerText) {
        CSize textExtent = MeasureWrapped(remainingText, bounds);
        bounds.top = bounds.top + bounds.Height() / 2 - textExtent.cy / 2;
    }

    i32 y = bounds.top;
    i32 x = bounds.left;

    CString line;
    while (y < bounds.bottom) {
        i32 textLength = remainingText.GetLength();
        if (textLength <= 0) {
            break;
        }

        i32 hasNewline = 0;
        for (i32 characterIndex = 0; characterIndex < textLength; characterIndex++) {
            if (remainingText[characterIndex] == '\n') {
                hasNewline = 1;
                break;
            }
        }

        CSize measuredExtent;
        measuredExtent = MeasureText(remainingText);
        if (measuredExtent.cx + x <= bounds.right && !hasNewline) {
            line += remainingText;
            remainingText = "";
            if (y + lineAdvance <= bounds.bottom) {
                if (centerText) {
                    CSize lineExtent = MeasureText(line);
                    DrawLine(
                        line,
                        surface,
                        bounds.left + bounds.Width() / 2 - lineExtent.cx / 2,
                        y,
                        blendGlyphs
                    );
                    x = bounds.left + bounds.Width() / 2 - lineExtent.cx;
                } else {
                    DrawLine(line, surface, bounds.left, y, blendGlyphs);
                }
            }
            line = "";
        } else {
            i32 wordLength = 0;
            i32 newlineAfterWord = 0;

            while (wordLength < remainingText.GetLength()) {
                u8 character = remainingText[wordLength];
                if (character == ' ' || character == '\n') {
                    break;
                }
                wordLength++;
            }
            if (wordLength < remainingText.GetLength() && remainingText[wordLength] == '\n') {
                newlineAfterWord = 1;
            }
            CString word;
            if (newlineAfterWord) {
                word = remainingText.Left(wordLength);
            } else {
                word = remainingText.Left(wordLength + 1);
            }
            measuredExtent = MeasureText(word);
            i32 wordWidth = measuredExtent.cx;
            remainingText = remainingText.Right(remainingText.GetLength() - wordLength - 1);
            if (wordWidth + x < bounds.right) {
                line += word;
                x = wordWidth + x;
            } else if (wordWidth < bounds.Width()) {
                if (centerText) {
                    CSize lineExtent = MeasureText(line);
                    DrawLine(
                        line,
                        surface,
                        bounds.left + bounds.Width() / 2 - lineExtent.cx / 2,
                        y,
                        blendGlyphs
                    );
                } else {
                    DrawLine(line, surface, bounds.left, y, blendGlyphs);
                }
                y = y + lineAdvance;
                x = bounds.left;
                line = "";
                if (lineAdvance + y < bounds.bottom) {
                    line += word;
                    x += wordWidth;
                }
            } else {

                // The signed length guard is required; IsEmpty emits a zero-only test.
                while (word.GetLength() > 0) {
                    if (y >= bounds.bottom) {
                        break;
                    }
                    measuredExtent = MeasureText(CString(word.GetAt(0), 1));
                    i32 glyphWidth = measuredExtent.cx;
                    if (glyphWidth + x > bounds.right) {
                        y = y + lineAdvance;
                        x = bounds.left;
                        if (centerText) {
                            CSize lineExtent = MeasureText(line);
                            DrawLine(
                                line,
                                surface,
                                bounds.left + bounds.Width() / 2 - lineExtent.cx / 2,
                                y,
                                blendGlyphs
                            );
                        } else {
                            DrawLine(line, surface, x, y, blendGlyphs);
                        }
                        line = "";
                    }
                    if (lineAdvance + y >= bounds.bottom) {
                        break;
                    }
                    line += word[0];
                    x += glyphWidth;
                }
            }
            if (newlineAfterWord) {
                if (centerText) {
                    CSize lineExtent = MeasureText(line);
                    DrawLine(
                        line,
                        surface,
                        bounds.left + bounds.Width() / 2 - lineExtent.cx / 2,
                        y,
                        blendGlyphs
                    );
                } else {
                    DrawLine(line, surface, bounds.left, y, blendGlyphs);
                }
                y = y + lineAdvance;
                x = bounds.left;
                line = "";
            }
        }
    }
    if (y + lineAdvance <= bounds.bottom && line.GetLength() > 0) {
        if (centerText) {
            CSize lineExtent = MeasureText(line);
            DrawLine(
                line,
                surface,
                bounds.left + bounds.Width() / 2 - lineExtent.cx / 2,
                y,
                blendGlyphs
            );
        } else {
            DrawLine(line, surface, bounds.left, y, blendGlyphs);
        }
    }
}

RVA(0x0017ac50, 0xbd)
CSize FontRenderer::MeasureText(CString text) {
    CSize textExtent;

    CSize glyphExtent;
    glyphExtent.cy = 0;
    i32 characterIndex = 0;
    i32 textWidth = 0;
    if (m_font == NULL) {
        return CSize(0, 0);
    }
    for (; characterIndex < text.GetLength(); characterIndex++) {
        u8 character = text[characterIndex];

        textWidth += m_font->GetGlyphSize(glyphExtent, character).cx;
    }
    textExtent = CSize(textWidth, m_font->GetMaxHeight());
    return textExtent;
}

// @early-stop
RVA(0x0017ad10, 0x402)
CSize FontRenderer::MeasureWrapped(CString remainingText, CRect bounds) {
    i32 y = bounds.top;
    CSize maxExtent;
    maxExtent.cx = 0;
    i32 x = bounds.left;

    CString line;
    while (y < bounds.bottom) {
        i32 textLength = remainingText.GetLength();
        if (textLength <= 0) {
            break;
        }

        i32 hasNewline = 0;
        for (i32 characterIndex = 0; characterIndex < textLength; characterIndex++) {
            if (remainingText[characterIndex] == '\n') {
                hasNewline = 1;
                break;
            }
        }

        CSize remainingExtent;
        remainingExtent = MeasureText(remainingText);
        if (remainingExtent.cx + x <= bounds.right && !hasNewline) {
            line += remainingText;
            remainingText = "";
            if (m_font->GetMaxHeight() + y <= bounds.bottom) {
                CSize lineExtent = MeasureText(line);
                i32 lineWidth = lineExtent.cx;
                maxExtent.cx = max(maxExtent.cx, lineWidth);
            }
        } else {
            i32 wordLength = 0;
            i32 newlineAfterWord = 0;

            while (wordLength < remainingText.GetLength()) {
                u8 character = remainingText[wordLength];
                if (character == ' ' || character == '\n') {
                    break;
                }
                wordLength++;
            }
            if (wordLength < remainingText.GetLength() && remainingText[wordLength] == '\n') {
                newlineAfterWord = 1;
            }
            CString word = remainingText.Left(wordLength + 1);

            CSize wordExtent;
            wordExtent = MeasureText(word);
            i32 wordWidth = wordExtent.cx;
            remainingText = remainingText.Right(remainingText.GetLength() - wordLength - 1);
            if (wordWidth + x < bounds.right) {
                line += word;
                x = wordWidth + x;
            } else if (wordWidth < bounds.Width()) {
                CSize lineExtent = MeasureText(line);
                i32 lineWidth = lineExtent.cx;
                maxExtent.cx = max(maxExtent.cx, lineWidth);
                y = y + m_font->GetMaxHeight();
                x = bounds.left;
                line = "";
                if (m_font->GetMaxHeight() + y < bounds.bottom) {
                    line += word;
                    x = wordWidth + bounds.left;
                }
            } else {

                for (i32 glyphIndex = 0; glyphIndex < word.GetLength(); glyphIndex++) {
                    if (y >= bounds.bottom) {
                        break;
                    }
                    CSize glyphExtent;
                    glyphExtent = MeasureText(CString(word.GetAt(glyphIndex), 1));
                    i32 glyphWidth = glyphExtent.cx;
                    if (glyphWidth + x > bounds.right) {
                        y = y + m_font->GetMaxHeight();
                        x = bounds.left;
                        CSize lineExtent = MeasureText(line);
                        i32 lineWidth = lineExtent.cx;
                        maxExtent.cx = max(maxExtent.cx, lineWidth);
                    }
                    if (m_font->GetMaxHeight() + y >= bounds.bottom) {
                        break;
                    }
                    line += word[glyphIndex];
                    x += glyphWidth;
                }
            }
            if (newlineAfterWord) {
                y = y + m_font->GetMaxHeight();
                x = bounds.left;
                line = "";
            }
        }
    }
    return CSize(maxExtent.cx - bounds.left + 1, m_font->GetMaxHeight() + (y - bounds.top) + 1);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017b120, 0x3c6)
CSize FontRenderer::LayoutWrapped(CString remainingText, CRect bounds, i32* outCharacterCount) {
    i32 y = bounds.top;
    i32 characterCount = 0;
    i32 x = bounds.left;

    CString line;
    while (y < bounds.bottom) {
        i32 textLength = remainingText.GetLength();
        if (textLength <= 0) {
            break;
        }

        i32 hasNewline = 0;
        for (i32 characterIndex = 0; characterIndex < textLength; characterIndex++) {
            if (remainingText[characterIndex] == '\n') {
                hasNewline = 1;
                break;
            }
        }

        CSize remainingExtent;
        remainingExtent = MeasureText(remainingText);
        if (remainingExtent.cx + x <= bounds.right && !hasNewline) {
            line += remainingText;
            remainingText = "";
            if (m_font->GetMaxHeight() + y <= bounds.bottom) {
                characterCount += line.GetLength();
            }
            line = "";
        } else {
            i32 wordLength = 0;
            i32 newlineAfterWord = 0;

            while (wordLength < remainingText.GetLength()) {
                u8 character = remainingText[wordLength];
                if (character == ' ' || character == '\n') {
                    break;
                }
                wordLength++;
            }
            if (wordLength < remainingText.GetLength() && remainingText[wordLength] == '\n') {
                newlineAfterWord = 1;
            }
            CString word = remainingText.Left(wordLength + 1);

            CSize wordExtent;
            wordExtent = MeasureText(word);
            i32 wordWidth = wordExtent.cx;
            remainingText = remainingText.Right(remainingText.GetLength() - wordLength - 1);
            if (wordWidth + x < bounds.right) {
                line += word;
                x = wordWidth + x;
            } else if (wordWidth < bounds.Width()) {
                characterCount += line.GetLength();
                y = y + m_font->GetMaxHeight();
                x = bounds.left;
                line = "";
                if (m_font->GetMaxHeight() + y < bounds.bottom) {
                    line += word;
                    x = wordWidth + bounds.left;
                }
            } else {

                // The signed length guard is required; IsEmpty emits a zero-only test.
                while (word.GetLength() > 0) {
                    if (y >= bounds.bottom) {
                        break;
                    }
                    CSize glyphExtent;
                    glyphExtent = MeasureText(CString(word.GetAt(0), 1));
                    i32 glyphWidth = glyphExtent.cx;
                    if (glyphWidth + x > bounds.right) {
                        y = y + m_font->GetMaxHeight();
                        x = bounds.left;
                        characterCount += line.GetLength();
                        line = "";
                    }
                    if (m_font->GetMaxHeight() + y >= bounds.bottom) {
                        break;
                    }
                    line += word[0];
                    x += glyphWidth;
                }
            }
            if (newlineAfterWord) {
                characterCount += line.GetLength();
                y = y + m_font->GetMaxHeight();
                x = bounds.left;
                line = "";
            }
        }
    }
    if (m_font->GetMaxHeight() + y <= bounds.bottom && line.GetLength() > 0) {
        characterCount += line.GetLength();
    }
    if (outCharacterCount) {
        *outCharacterCount = characterCount;
    }
    return CSize(x, m_font->GetMaxHeight() + y + 1);
}

RVA_COMPGEN(0x0017b4f0, 0xc, ?GetAt@CString@@QBEDH@Z)
