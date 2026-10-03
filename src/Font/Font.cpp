#include <StdAfx.h>

#include <Ints.h>

#include <Font/Font.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Font/FontBlendInline.h>
#include <RectMacros.h>
#include <SafeDelete.h>
#include <Wap32/TileGeometry.h>

#include <ddraw.h>
#include <limits.h>

Font::Font() {
    m_surfaces = NULL;
    m_glyphs = NULL;
    m_ready = false;
    m_count = 0;
}

Font::~Font() {
    FreeMemory();
}

i32 Font::AllocateMemory(i32 count) {
    FreeMemory();

    m_count = count;
    if (count < 1) {
        return 0;
    }

    m_surfaces = new u8*[m_count];
    m_glyphs = new CSize[m_count];

    for (i32 i = 0; i < m_count; i++) {
        m_surfaces[i] = NULL;

        CSize g(0, 0);
        SET_FONT_GLYPH(i, g);
    }

    m_maxHeight = 0;
    m_ready = true;
    return 1;
}

void Font::FreeMemory() {
    if (m_ready) {
        for (i32 i = 0; i < m_count; i++) {
            if (m_surfaces[i]) {
                delete[] m_surfaces[i];
                m_surfaces[i] = NULL;
            }
        }
        delete[] m_surfaces;
        m_surfaces = NULL;
        SAFE_DELETE_ARRAY(m_glyphs);
        m_count = 0;
        m_ready = false;
    }
}

i32 Font::LoadFont(std::string szFileName) {
    FreeMemory();

    CFile file;
    if (!file.Open((szFileName).c_str(), CFile::modeRead, NULL)) {
        return 0;
    }

    CArchive ar(&file, CArchive::load, 0x1000, NULL);

    ar >> m_count;
    AllocateMemory(m_count);

    for (i32 i = 0; i < m_count; i++) {
        ar.Read(&m_glyphs[i], sizeof(CSize));
        m_surfaces[i] = new u8[m_glyphs[i].cx * m_glyphs[i].cy];
        ar.Read(m_surfaces[i], m_glyphs[i].cx * m_glyphs[i].cy);
    }

    ar.Close();
    file.Close();

    i32 maxHeight = 0;
    for (i32 j = 0; j < m_count; j++) {
        maxHeight = max(maxHeight, m_glyphs[j].cy);
    }
    m_maxHeight = maxHeight;

    return 1;
}

i32 Font::SaveFont(std::string szFileName) {
    CFile file;
    if (!file.Open((szFileName).c_str(), CFile::modeCreate | CFile::modeWrite, NULL)) {
        return 0;
    }

    CArchive ar(&file, CArchive::store, 0x1000, NULL);

    ar << m_count;

    for (i32 i = 0; i < m_count; i++) {
        CSize g = m_glyphs[i];
        ar.Write(&g, sizeof(CSize));
        ar.Write(m_surfaces[i], m_glyphs[i].cx * m_glyphs[i].cy);
    }

    ar.Close();
    file.Close();

    return 1;
}

u8** Font::GetSurface(u8 c) {
    return &m_surfaces[c];
}

CSize& Font::GetGlyph(CSize& out, u8 c) {
    out = m_glyphs[c];
    return out;
}

void Font::SetGlyph(u8 c, CSize glyph) {
    SET_FONT_GLYPH(c, glyph);
}

i32 Font::GetMaxHeight() {
    return m_maxHeight;
}

FontRenderer::FontRenderer() {
    m_font = NULL;
    m_color = RGB(255, 255, 255);
    m_clip = 0;
    m_surface = 0;
}

FontRenderer::~FontRenderer() {}

void FontRenderer::SetFont(Font* f) {
    m_font = f;
}

void FontRenderer::SetColor(i32 color) {
    m_color = color;
}

void FontRenderer::DrawLine(std::string text, CDDSurface* surf, i32 x, i32 y, i32 z) {
    CSize ext = MeasureText(text);
    if (m_font == NULL) {
        return;
    }
    i32 limit = surf->GetHeight();
    if (m_font->GetMaxHeight() + y > limit) {
        return;
    }
    DrawLineClipped(text, surf, CRect(0, 0, ext.cx, ext.cy), x, y, z);
}

void FontRenderer::DrawLineClipped(std::string text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 z) {
    i32 savedColor = m_color;
    if (m_clip) {
        SetColor(RGB(255, 255, 255));
        DrawGlyphRun(text, surf, rc, x, y, z);
        x++;
        y++;
    }
    if (m_surface) {
        SetColor(RGB(0, 0, 0));
        x += 2;
        DrawGlyphRun(text, surf, rc, x, y, z);
        x -= 2;
    }
    SetColor(savedColor);
    DrawGlyphRun(text, surf, rc, x, y, z);
}

void FontRenderer::DrawGlyphRun(std::string text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend) {
    if (m_font == NULL) {
        return;
    }
    if (rc.left < 0) {
        return;
    }
    if (rc.top < 0) {
        return;
    }
    if (x < 0) {
        return;
    }
    if (y < 0) {
        return;
    }

    if (RunRightEdge(rc, x) > surf->GetWidth()) {
        rc.right = rc.right + rc.Width() + x - surf->GetWidth();
    }
    if (y + rc.Height() > surf->GetHeight()) {
        rc.bottom = rc.bottom + rc.Height() + y - surf->GetHeight();
    }

    CSize m = MeasureText(text);
    {
        CRect extent(CPoint(0, 0), m);
        if (!rc.IntersectRect(&rc, &extent)) {
            return;
        }
    }
    CLAMP_UPPER_INPLACE(rc.right, m.cx);
    CLAMP_UPPER_INPLACE(rc.bottom, m.cy);

    u16* bits = static_cast<u16*>(surf->Lock(NULL));
    i32 pitch = surf->m_apiDesc.lPitch;
    if (bits == NULL) {
        return;
    }

    CPoint dest(x, y);
    i32 acc = 0;
    i32 red = GetRValue(m_color);
    i32 green = GetGValue(m_color);
    i32 blue = GetBValue(m_color);
    u16 packedColor =
        (static_cast<u8>((static_cast<u8>(red) >> static_cast<u8>(g_rDown))) << g_rUp)
        | (static_cast<u8>((static_cast<u8>(green) >> static_cast<u8>(g_gDown))) << g_gUp)
        | (static_cast<u8>(blue) >> static_cast<u8>(g_bDown));

    i32 rightPartial = 0;
    i32 firstCol = 0;
    i32 startChar;
    if (rc.left != 0) {
        i32 prev = 0;
        startChar = 0;
        while (acc < rc.left) {
            CSize g;
            prev = acc;
            acc += m_font->GetGlyph(g, text[startChar]).cx;
            startChar++;
        }
        --startChar;
        firstCol = rc.left - prev;
    } else {
        startChar = 0;
    }

    i32 endChar;
    acc = 0;

    if (rc.right != m.cx) {
        i32 j = 0;
        endChar = 0;
        if (rc.right >= 0) {
            do {
                CSize g;
                acc += m_font->GetGlyph(g, text[j]).cx;
                j++;
            } while (acc <= rc.right);
            endChar = j;
        }
        rightPartial = acc - rc.right;
    } else {
        endChar = static_cast<i32>((text).size());
    }

    for (i32 ci = startChar; ci < endChar; ci++) {
        CSize g;
        m = m_font->GetGlyph(g, text[ci]);
        i32 row;
        i32 col;
        i32 clippedW;
        if (ci == endChar - 1) {
            clippedW = m.cx - rightPartial;
        } else {
            clippedW = m.cx;
        }
        u8* glyphBuf = m_font->GetSurface(text[ci])[0];
        if (blend) {
            for (row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + dest.y) * pitch) / 2 + dest.x;
                for (col = firstCol; col < clippedW; col++) {
                    u8 cover = glyphBuf[row * m.cx + col];

                    if (cover == 0) {
                    } else if (cover != UCHAR_MAX) {
                        *dst = BlendPixel16(*dst, cover, red, green, blue);
                    } else {
                        *dst = packedColor;
                    }
                    dst++;
                }
            }
        } else {
            for (row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + dest.y) * pitch) / 2 + dest.x;
                for (col = firstCol; col < clippedW; col++) {
                    if (glyphBuf[row * m.cx + col] != 0) {
                        *dst = packedColor;
                    }
                    dst++;
                }
            }
        }
        dest.x += clippedW - firstCol;
        firstCol = 0;
    }

    surf->Unlock();
}

void FontRenderer::DrawWrapped(
    std::string text,
    CDDSurface* surf,
    CRect rc,
    i32 z,
    i32 hcenter,
    i32 spacing
) {
    i32 lineAdvance = m_font->GetMaxHeight() + spacing;
    if (hcenter) {
        CSize m = MeasureWrapped(text, rc);
        rc.top = rc.top + rc.Height() / 2 - m.cy / 2;
    }

    i32 y = rc.top;
    i32 x = rc.left;

    std::string line;
    while (y < rc.bottom) {
        i32 len = static_cast<i32>((text).size());
        if (len <= 0) {
            break;
        }

        i32 nl = 0;
        for (i32 k = 0; k < len; k++) {
            if (text[k] == '\n') {
                nl = 1;
                break;
            }
        }

        CSize size;
        size = MeasureText(text);
        if (size.cx + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (y + lineAdvance <= rc.bottom) {
                if (hcenter) {
                    CSize le = MeasureText(line);
                    DrawLine(line, surf, rc.left + rc.Width() / 2 - le.cx / 2, y, z);
                    x = rc.left + rc.Width() / 2 - le.cx;
                } else {
                    DrawLine(line, surf, rc.left, y, z);
                }
            }
            line = "";
        } else {
            i32 i = 0;
            i32 breakNL = 0;

            while (i < static_cast<i32>((text).size())) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < static_cast<i32>((text).size()) && text[i] == '\n') {
                breakNL = 1;
            }
            std::string head;
            if (breakNL) {
                head = sliceText(text, 0, i);
            } else {
                head = sliceText(text, 0, i + 1);
            }
            size = MeasureText(head);
            i32 headW = size.cx;
            text = rightText(text, static_cast<i32>((text).size()) - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.Width()) {
                if (hcenter) {
                    CSize le = MeasureText(line);
                    DrawLine(line, surf, rc.left + rc.Width() / 2 - le.cx / 2, y, z);
                } else {
                    DrawLine(line, surf, rc.left, y, z);
                }
                y = y + lineAdvance;
                x = rc.left;
                line = "";
                if (lineAdvance + y < rc.bottom) {
                    line += head;
                    x += headW;
                }
            } else {

                while (static_cast<i32>((head).size()) > 0) {
                    if (y >= rc.bottom) {
                        break;
                    }
                    size = MeasureText(std::string(1, head[0]));
                    i32 chW = size.cx;
                    if (chW + x > rc.right) {
                        y = y + lineAdvance;
                        x = rc.left;
                        if (hcenter) {
                            CSize le = MeasureText(line);
                            DrawLine(line, surf, rc.left + rc.Width() / 2 - le.cx / 2, y, z);
                        } else {
                            DrawLine(line, surf, x, y, z);
                        }
                        line = "";
                    }
                    if (lineAdvance + y >= rc.bottom) {
                        break;
                    }
                    line += head[0];
                    x += chW;
                }
            }
            if (breakNL) {
                if (hcenter) {
                    CSize le = MeasureText(line);
                    DrawLine(line, surf, rc.left + rc.Width() / 2 - le.cx / 2, y, z);
                } else {
                    DrawLine(line, surf, rc.left, y, z);
                }
                y = y + lineAdvance;
                x = rc.left;
                line = "";
            }
        }
    }
    if (y + lineAdvance <= rc.bottom && static_cast<i32>((line).size()) > 0) {
        if (hcenter) {
            CSize le = MeasureText(line);
            DrawLine(line, surf, rc.left + rc.Width() / 2 - le.cx / 2, y, z);
        } else {
            DrawLine(line, surf, rc.left, y, z);
        }
    }
}

CSize FontRenderer::MeasureText(std::string text) {
    CSize ext;

    CSize g;
    g.cy = 0;
    i32 i = 0;
    i32 width = 0;
    if (m_font == NULL) {
        return CSize(0, 0);
    }
    for (; i < static_cast<i32>((text).size()); i++) {
        u8 c = text[i];

        width += m_font->GetGlyph(g, c).cx;
    }
    ext = CSize(width, m_font->GetMaxHeight());
    return ext;
}

CSize FontRenderer::MeasureWrapped(std::string text, CRect rc) {
    i32 y = rc.top;
    CSize maxExtent;
    maxExtent.cx = 0;
    i32 x = rc.left;

    std::string line;
    while (y < rc.bottom) {
        i32 len = static_cast<i32>((text).size());
        if (len <= 0) {
            break;
        }

        i32 nl = 0;
        for (i32 k = 0; k < len; k++) {
            if (text[k] == '\n') {
                nl = 1;
                break;
            }
        }

        CSize e;
        e = MeasureText(text);
        if (e.cx + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= rc.bottom) {
                CSize lw = MeasureText(line);
                i32 w = lw.cx;
                maxExtent.cx = max(maxExtent.cx, w);
            }
        } else {
            i32 i = 0;
            i32 breakNL = 0;

            while (i < static_cast<i32>((text).size())) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < static_cast<i32>((text).size()) && text[i] == '\n') {
                breakNL = 1;
            }
            std::string head = sliceText(text, 0, i + 1);

            CSize he;
            he = MeasureText(head);
            i32 headW = he.cx;
            text = rightText(text, static_cast<i32>((text).size()) - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.Width()) {
                CSize lw = MeasureText(line);
                i32 w = lw.cx;
                maxExtent.cx = max(maxExtent.cx, w);
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
                if (m_font->GetMaxHeight() + y < rc.bottom) {
                    line += head;
                    x = headW + rc.left;
                }
            } else {

                for (i32 j = 0; j < static_cast<i32>((head).size()); j++) {
                    if (y >= rc.bottom) {
                        break;
                    }
                    CSize ce;
                    ce = MeasureText(std::string(1, head[j]));
                    i32 chW = ce.cx;
                    if (chW + x > rc.right) {
                        y = y + m_font->GetMaxHeight();
                        x = rc.left;
                        CSize lw = MeasureText(line);
                        i32 w = lw.cx;
                        maxExtent.cx = max(maxExtent.cx, w);
                    }
                    if (m_font->GetMaxHeight() + y >= rc.bottom) {
                        break;
                    }
                    line += head[j];
                    x += chW;
                }
            }
            if (breakNL) {
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
            }
        }
    }
    return CSize(maxExtent.cx - rc.left + 1, m_font->GetMaxHeight() + (y - rc.top) + 1);
}

CSize FontRenderer::LayoutWrapped(std::string text, CRect rc, i32* outLen) {
    i32 y = rc.top;
    i32 totalChars = 0;
    i32 x = rc.left;

    std::string line;
    while (y < rc.bottom) {
        i32 len = static_cast<i32>((text).size());
        if (len <= 0) {
            break;
        }

        i32 nl = 0;
        for (i32 k = 0; k < len; k++) {
            if (text[k] == '\n') {
                nl = 1;
                break;
            }
        }

        CSize e;
        e = MeasureText(text);
        if (e.cx + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= rc.bottom) {
                totalChars += static_cast<i32>((line).size());
            }
            line = "";
        } else {
            i32 i = 0;
            i32 breakNL = 0;

            while (i < static_cast<i32>((text).size())) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < static_cast<i32>((text).size()) && text[i] == '\n') {
                breakNL = 1;
            }
            std::string head = sliceText(text, 0, i + 1);

            CSize he;
            he = MeasureText(head);
            i32 headW = he.cx;
            text = rightText(text, static_cast<i32>((text).size()) - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.Width()) {
                totalChars += static_cast<i32>((line).size());
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
                if (m_font->GetMaxHeight() + y < rc.bottom) {
                    line += head;
                    x = headW + rc.left;
                }
            } else {

                while (static_cast<i32>((head).size()) > 0) {
                    if (y >= rc.bottom) {
                        break;
                    }
                    CSize ce;
                    ce = MeasureText(std::string(1, head[0]));
                    i32 chW = ce.cx;
                    if (chW + x > rc.right) {
                        y = y + m_font->GetMaxHeight();
                        x = rc.left;
                        totalChars += static_cast<i32>((line).size());
                        line = "";
                    }
                    if (m_font->GetMaxHeight() + y >= rc.bottom) {
                        break;
                    }
                    line += head[0];
                    x += chW;
                }
            }
            if (breakNL) {
                totalChars += static_cast<i32>((line).size());
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
            }
        }
    }
    if (m_font->GetMaxHeight() + y <= rc.bottom && static_cast<i32>((line).size()) > 0) {
        totalChars += static_cast<i32>((line).size());
    }
    if (outLen) {
        *outLen = totalChars;
    }
    return CSize(x, m_font->GetMaxHeight() + y + 1);
}
