#include <Mfc.h>

#undef _AFX_ENABLE_INLINES
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Font/Font.h>
#include <ddraw.h>
#include <rva.h>

RVA(0x00179700, 0x10)
Font::Font() {
    m_surfaces = 0;
    m_glyphs = 0;
    m_ready = 0;
    m_count = 0;
}

RVA(0x00179710, 0x5)
Font::~Font() {
    FreeMemory();
}

RVA(0x00179720, 0x87)
i32 Font::AllocateMemory(i32 count) {
    FreeMemory();

    m_count = count;
    if (count < 1) {
        return 0;
    }

    m_surfaces = static_cast<void**>(operator new(m_count * sizeof(void*)));
    m_glyphs = new Glyph[m_count];

    for (i32 i = 0; i < m_count; i++) {
        m_surfaces[i] = 0;

        Glyph g;
        g.width = 0;
        g.height = 0;
        m_glyphs[i] = g;
    }

    m_maxHeight = 0;
    m_ready = 1;
    return 1;
}

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

RVA(0x00179830, 0x1b1)
i32 Font::LoadFont(CString szFileName) {
    FreeMemory();

    CFile file;
    if (!file.Open(szFileName, 0, 0)) {
        return 0;
    }

    CArchive ar(&file, 1, 0x1000, 0);

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

RVA(0x001799f0, 0x16d)
i32 Font::SaveFont(CString szFileName) {
    CFile file;
    if (!file.Open(szFileName, 0x1001, 0)) {
        return 0;
    }

    CArchive ar(&file, 0, 0x1000, 0);

    ar << m_count;

    for (i32 i = 0; i < m_count; i++) {
        Glyph g = m_glyphs[i];
        ar.Write(&g, sizeof(Glyph));
        ar.Write(m_surfaces[i], m_glyphs[i].width * m_glyphs[i].height);
    }

    ar.Close();
    file.Close();

    return 1;
}

RVA(0x00179b60, 0x12)
void** Font::GetSurface(u8 c) {
    return &m_surfaces[c];
}

RVA(0x00179b80, 0x22)
Glyph& Font::GetGlyph(Glyph& out, u8 c) {
    out = m_glyphs[c];
    return out;
}

RVA(0x00179bd0, 0x4)
i32 Font::GetMaxHeight() {
    return m_maxHeight;
}

RVA(0x00179be0, 0x14)
FontRenderer::FontRenderer() {
    m_font = 0;
    m_color = 0x00ffffff;
    m_clip = 0;
    m_surface = 0;
}

RVA(0x00179c00, 0x1)
FontRenderer::~FontRenderer() {}

RVA(0x00179c10, 0x9)
void FontRenderer::SetFont(Font* f) {
    m_font = f;
}

RVA(0x00179c20, 0xa)
void FontRenderer::SetColor(i32 color) {
    m_color = color;
}

RVA(0x00179c30, 0xdb)
void FontRenderer::DrawLine(CString text, CDDSurface* surf, i32 x, i32 y, i32 z) {
    TextExtent ext = MeasureText(text);
    if (m_font == 0) {
        return;
    }
    i32 limit = surf->m_height;
    if (m_font->GetMaxHeight() + y > limit) {
        return;
    }
    DrawLineClipped(text, surf, CRect(0, 0, ext.width, ext.height), x, y, z);
}

RVA(0x00179d10, 0x15c)
void FontRenderer::DrawLineClipped(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 z) {
    i32 savedColor = m_color;
    if (m_clip) {
        SetColor(0xffffff);
        DrawGlyphRun(text, surf, rc, x, y, z);
        x++;
        y++;
    }
    if (m_surface) {
        SetColor(0);
        x += 2;
        DrawGlyphRun(text, surf, rc, x, y, z);
        x -= 2;
    }
    SetColor(savedColor);
    DrawGlyphRun(text, surf, rc, x, y, z);
}

// @early-stop
RVA(0x00179e70, 0x5ec)
void FontRenderer::DrawGlyphRun(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend) {
    if (m_font == 0) {
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

    if (x - rc.left + rc.right > surf->m_width) {
        rc.right = rc.right + rc.right - rc.left + x - surf->m_width;
    }
    if (y - rc.top + rc.bottom > surf->m_height) {
        rc.bottom = rc.bottom + rc.bottom - rc.top + y - surf->m_height;
    }

    TextExtent m = MeasureText(text);
    RECT extent;
    extent.right = m.width;
    extent.bottom = m.height;
    extent.left = 0;
    extent.top = 0;
    if (!IntersectRect(&rc, &rc, &extent)) {
        return;
    }
    if (rc.right > m.width) {
        rc.right = m.width;
    }
    if (rc.bottom > m.height) {
        rc.bottom = m.height;
    }

    u16* bits = static_cast<u16*>(surf->Lock(0));
    if (bits == 0) {
        return;
    }
    i32 pitch = surf->m_pitch;

    i32 destX = x;
    i32 red = m_color & 0xff;
    i32 green = (m_color >> 8) & 0xff;
    i32 blue = (m_color >> 16) & 0xff;
    i32 rightPartial = 0;
    i32 firstCol = 0;
    i32 packedColor =
        (static_cast<u8>((static_cast<u8>(red) >> static_cast<u8>(g_rDown))) << g_rUp)
        | (static_cast<u8>((static_cast<u8>(green) >> static_cast<u8>(g_gDown))) << g_gUp)
        | (static_cast<u8>(blue) >> static_cast<u8>(g_bDown));

    i32 startChar;
    i32 acc = 0;
    if (rc.left != 0) {
        i32 prev = 0;
        i32 i = 0;
        while (acc < rc.left) {
            Glyph g;
            prev = acc;
            acc += m_font->GetGlyph(g, text[i]).width;
            i++;
        }
        startChar = i - 1;
        firstCol = rc.left - prev;
    } else {
        startChar = 0;
    }

    i32 endChar;
    i32 w = 0;

    if (rc.right != m.width) {
        i32 j = 0;
        endChar = 0;
        if (rc.right >= 0) {
            do {
                Glyph g;
                w += m_font->GetGlyph(g, text[j]).width;
                j++;
            } while (w <= rc.right);
            endChar = j;
        }
        rightPartial = w - rc.right;
    } else {
        endChar = text.GetLength();
    }

    i32 lastChar = endChar - 1;
    for (i32 ci = startChar; ci < endChar; ci++) {
        Glyph g;
        Glyph gm = m_font->GetGlyph(g, text[ci]);
        i32 gw = gm.width;
        i32 clippedW;
        if (ci == lastChar) {
            clippedW = gw - rightPartial;
        } else {
            clippedW = gw;
        }
        u8* glyphBuf = static_cast<u8*>(m_font->GetSurface(text[ci])[0]);
        i32 startCol = firstCol;
        if (blend) {
            for (i32 row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + y) * pitch) / 2 + destX;
                for (i32 col = startCol; col < clippedW; col++) {
                    u8 cover = glyphBuf[row * gw + col];

                    if (cover == 0) {
                    } else if (cover != 0xff) {
                        i32 inv = 255 - cover;
                        u16 dp = *dst;
                        i32 dr = static_cast<u8>((static_cast<u8>((dp >> g_rUp)) << g_rDown));
                        i32 dg = static_cast<u8>((static_cast<u8>((dp >> g_gUp)) << g_gDown));
                        i32 db = static_cast<u8>((static_cast<u8>(dp) << g_bDown));
                        i32 bB = (db * inv) / 256 + (blue * cover) / 256;
                        i32 rB = (dr * inv) / 256 + (red * cover) / 256;
                        i32 gB = (dg * inv) / 256 + (green * cover) / 256;
                        *dst = static_cast<u16>(
                            (static_cast<u8>((static_cast<u8>(bB) >> static_cast<u8>(g_bDown)))
                             | (static_cast<u8>((static_cast<u8>(rB) >> static_cast<u8>(g_rDown)))
                                << g_rUp)
                             | (static_cast<u8>((static_cast<u8>(gB) >> static_cast<u8>(g_gDown)))
                                << g_gUp))
                        );
                    } else {
                        *dst = static_cast<u16>(packedColor);
                    }
                    dst++;
                }
            }
        } else {
            for (i32 row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + y) * pitch) / 2 + destX;
                for (i32 col = startCol; col < clippedW; col++) {
                    if (glyphBuf[row * gw + col] != 0) {
                        *dst = static_cast<u16>(packedColor);
                    }
                    dst++;
                }
            }
        }
        destX += clippedW - startCol;
        firstCol = 0;
    }

    surf->m_ddSurface->Unlock(0);
}

// @early-stop
RVA(0x0017a460, 0x7ec)
void FontRenderer::DrawWrapped(
    CString text,
    CDDSurface* surf,
    CRect rc,
    i32 z,
    i32 hcenter,
    i32 spacing
) {
    i32 lineAdvance = m_font->GetMaxHeight() + spacing;
    if (hcenter) {
        TextExtent m = MeasureWrapped(text, rc.left, rc.top, rc.right, rc.bottom);
        rc.top = rc.top + (rc.bottom - rc.top) / 2 - m.height / 2;
    }

    i32 y = rc.top;
    i32 x = rc.left;

    CString line;
    while (y < rc.bottom) {
        i32 len = text.GetLength();
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

        if (MeasureText(text).width + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (y + lineAdvance <= rc.bottom) {
                if (hcenter) {
                    i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
                    DrawLine(line, surf, cx, y, z);
                } else {
                    DrawLine(line, surf, rc.left, y, z);
                }
            }
            line = "";
        } else {
            i32 i = 0;
            i32 breakNL = 0;

            while (i < text.GetLength()) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < text.GetLength() && text[i] == '\n') {
                breakNL = 1;
            }
            CString head;
            if (breakNL) {
                head = text.Left(i);
            } else {
                head = text.Left(i + 1);
            }
            i32 headW = MeasureText(head).width;
            text = text.Right(len - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.right - rc.left) {
                if (hcenter) {
                    i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
                    DrawLine(line, surf, cx, y, z);
                } else {
                    DrawLine(line, surf, rc.left, y, z);
                }
                y = y + lineAdvance;
                x = rc.left;
                line = "";
                if (lineAdvance + y < rc.bottom) {
                    line += head;
                    x = headW + rc.left;
                }
            } else {

                while (head.GetLength() > 0) {
                    if (y >= rc.bottom) {
                        break;
                    }
                    i32 chW = MeasureText(CString(head.GetAt(0), 1)).width;
                    if (chW + x > rc.right) {
                        if (hcenter) {
                            i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
                            DrawLine(line, surf, cx, y, z);
                        } else {
                            DrawLine(line, surf, rc.left, y, z);
                        }
                        y = y + lineAdvance;
                        x = rc.left;
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
                    i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
                    DrawLine(line, surf, cx, y, z);
                } else {
                    DrawLine(line, surf, rc.left, y, z);
                }
                y = y + lineAdvance;
                x = rc.left;
                line = "";
            }
        }
    }
    if (y + lineAdvance <= rc.bottom && line.GetLength() > 0) {
        if (hcenter) {
            i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
            DrawLine(line, surf, cx, y, z);
        } else {
            DrawLine(line, surf, rc.left, y, z);
        }
    }
}

// @early-stop
RVA(0x0017ac50, 0xbd)
TextExtent FontRenderer::MeasureText(CString text) {
    TextExtent ext;

    Glyph g;
    g.height = 0;
    i32 i = 0;
    i32 width = 0;
    if (m_font == 0) {

        ext.height = 0;
        ext.width = 0;
        return ext;
    }
    for (; i < text.GetLength(); i++) {
        u8 c = text[i];

        width += m_font->GetGlyph(g, c).width;
    }
    ext.width = width;
    ext.height = m_font->GetMaxHeight();
    return ext;
}

// @early-stop
RVA(0x0017ad10, 0x402)
TextExtent FontRenderer::MeasureWrapped(CString text, i32 x0, i32 top, i32 right, i32 bottom) {
    TextExtent ext;
    i32 maxWidth = 0;
    i32 y = top;
    i32 x = x0;

    CString line;
    while (y < bottom) {
        i32 len = text.GetLength();
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

        TextExtent e = MeasureText(text);
        if (e.width + x <= right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= bottom) {
                i32 w = MeasureText(line).width;
                if (maxWidth <= w) {
                    maxWidth = w;
                }
            }
        } else {
            i32 i = 0;
            i32 breakNL = 0;

            while (i < text.GetLength()) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < text.GetLength() && text[i] == '\n') {
                breakNL = 1;
            }
            CString head = text.Left(i + 1);

            TextExtent he = MeasureText(head);
            i32 headW = he.width;
            text = text.Right(text.GetLength() - i - 1);
            if (headW + x < right) {
                line += head;
                x = headW + x;
            } else if (headW < right - x0) {
                i32 w = MeasureText(line).width;
                if (maxWidth <= w) {
                    maxWidth = w;
                }
                y = y + m_font->GetMaxHeight();
                x = x0;
                line = "";
                if (m_font->GetMaxHeight() + y < bottom) {
                    line += head;
                    x = headW + x0;
                }
            } else {

                for (i32 j = 0; j < head.GetLength(); j++) {
                    if (y >= bottom) {
                        break;
                    }
                    i32 chW = MeasureText(CString(head.GetAt(j), 1)).width;
                    if (chW + x > right) {
                        y = y + m_font->GetMaxHeight();
                        x = x0;
                        i32 w = MeasureText(line).width;
                        if (maxWidth <= w) {
                            maxWidth = w;
                        }
                    }
                    if (m_font->GetMaxHeight() + y >= bottom) {
                        break;
                    }
                    line += head[j];
                    x += chW;
                }
            }
            if (breakNL) {
                y = y + m_font->GetMaxHeight();
                x = x0;
                line = "";
            }
        }
    }
    ext.width = maxWidth - x0 + 1;
    ext.height = m_font->GetMaxHeight() + (y - top) + 1;
    return ext;
}

// @early-stop
RVA(0x0017b120, 0x3c6)
TextExtent
FontRenderer::LayoutWrapped(CString text, i32 x0, i32 begin, i32 right, i32 bottom, i32* outLen) {
    TextExtent ext;
    i32 totalChars = 0;
    i32 y = begin;
    i32 x = x0;

    CString line;
    while (y < bottom) {
        i32 len = text.GetLength();
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

        if (MeasureText(text).width + x <= right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= bottom) {
                totalChars += line.GetLength();
            }
            line = "";
        } else {
            i32 i = 0;
            i32 breakNL = 0;

            while (i < text.GetLength()) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < text.GetLength() && text[i] == '\n') {
                breakNL = 1;
            }
            CString head = text.Left(i + 1);

            TextExtent he = MeasureText(head);
            i32 headW = he.width;
            text = text.Right(text.GetLength() - i - 1);
            if (headW + x < right) {
                line += head;
                x = headW + x;
            } else if (headW < right - x0) {
                totalChars += line.GetLength();
                y = y + m_font->GetMaxHeight();
                x = x0;
                line = "";
                if (m_font->GetMaxHeight() + y < bottom) {
                    line += head;
                    x = headW + x0;
                }
            } else {

                while (head.GetLength() > 0) {
                    if (y >= bottom) {
                        break;
                    }
                    i32 chW = MeasureText(CString(head.GetAt(0), 1)).width;
                    if (chW + x > right) {
                        y = y + m_font->GetMaxHeight();
                        x = x0;
                        totalChars += line.GetLength();
                        line = "";
                    }
                    if (m_font->GetMaxHeight() + y >= bottom) {
                        break;
                    }
                    line += head[0];
                    x += chW;
                }
            }
            if (breakNL) {
                totalChars += line.GetLength();
                y = y + m_font->GetMaxHeight();
                x = x0;
                line = "";
            }
        }
    }
    if (m_font->GetMaxHeight() + y <= bottom && line.GetLength() > 0) {
        totalChars += line.GetLength();
    }
    if (outLen) {
        *outLen = totalChars;
    }
    ext.width = x;
    ext.height = m_font->GetMaxHeight() + y + 1;
    return ext;
}

RVA_COMPGEN(0x0017b4f0, 0xc, ?GetAt@CString@@QBEDH@Z)

RVA(0x0017b500, 0x8)
i32 CRect::Width() {
    return right - left;
}
