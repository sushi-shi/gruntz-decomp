#include <rva.h>

#include <Font/Font.h>

#include <Mfc.h>
#include <MfcNoInline.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>

#include <ddraw.h>
#include <limits.h>

#define SET_FONT_GLYPH(c, glyph) m_glyphs[c] = glyph

RVA(0x00179700, 0x10)
Font::Font() {
    m_surfaces = NULL;
    m_glyphs = NULL;
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

    m_surfaces = new u8*[m_count];
    m_glyphs = new Glyph[m_count];

    for (i32 i = 0; i < m_count; i++) {
        m_surfaces[i] = NULL;

        Glyph g;
        g.width = 0;
        g.height = 0;
        SET_FONT_GLYPH(i, g);
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
                delete[] m_surfaces[i];
                m_surfaces[i] = NULL;
            }
        }
        delete[] m_surfaces;
        m_surfaces = NULL;
        if (m_glyphs) {
            delete[] m_glyphs;
            m_glyphs = NULL;
        }
        m_count = 0;
        m_ready = 0;
    }
}

RVA(0x00179830, 0x1b1)
i32 Font::LoadFont(CString szFileName) {
    FreeMemory();

    CFile file;
    if (!file.Open(szFileName, 0, NULL)) {
        return 0;
    }

    CArchive ar(&file, 1, 0x1000, NULL);

    ar >> m_count;
    AllocateMemory(m_count);

    for (i32 i = 0; i < m_count; i++) {
        ar.Read(&m_glyphs[i], sizeof(Glyph));
        m_surfaces[i] = new u8[m_glyphs[i].width * m_glyphs[i].height];
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001799f0, 0x16d)
i32 Font::SaveFont(CString szFileName) {
    CFile file;
    if (!file.Open(szFileName, 0x1001, NULL)) {
        return 0;
    }

    CArchive ar(&file, 0, 0x1000, NULL);

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
u8** Font::GetSurface(u8 c) {
    return &m_surfaces[c];
}

RVA(0x00179b80, 0x22)
Glyph& Font::GetGlyph(Glyph& out, u8 c) {
    out = m_glyphs[c];
    return out;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179bb0, 0x1e)
void Font::SetGlyph(u8 c, Glyph glyph) {
    SET_FONT_GLYPH(c, glyph);
}

RVA(0x00179bd0, 0x4)
i32 Font::GetMaxHeight() {
    return m_maxHeight;
}

RVA(0x00179be0, 0x14)
FontRenderer::FontRenderer() {
    m_font = NULL;
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
    if (m_font == NULL) {
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

static inline LONG RunRightEdge(const CRect& rc, i32 x) {
    return x - rc.left + rc.right;
}

RVA(0x00179e70, 0x5ec)
void FontRenderer::DrawGlyphRun(CString text, CDDSurface* surf, CRect rc, i32 x, i32 y, i32 blend) {
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

    if (RunRightEdge(rc, x) > surf->m_width) {
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

    u16* bits = static_cast<u16*>(surf->Lock(NULL));
    i32 pitch = surf->m_pitch;
    if (bits == NULL) {
        return;
    }

    i32 destX = x;
    i32 red = static_cast<u8>(m_color);
    i32 green = (m_color >> 8) & 0xff;
    i32 blue = (m_color >> 16) & 0xff;
    i32 rightPartial = 0;
    i32 firstCol = 0;
    u16 packedColor =
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

    for (i32 ci = startChar; ci < endChar; ci++) {
        Glyph g;
        Glyph gm = m_font->GetGlyph(g, text[ci]);
        i32 clippedW;
        if (ci == endChar - 1) {
            clippedW = gm.width - rightPartial;
        } else {
            clippedW = g.width;
        }
        u8* glyphBuf = m_font->GetSurface(text[ci])[0];
        i32 startCol = firstCol;
        if (blend) {
            for (i32 row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + y) * pitch) / 2 + destX;
                for (i32 col = startCol; col < clippedW; col++) {
                    u8 cover = glyphBuf[row * g.width + col];

                    if (cover == 0) {
                    } else if (cover != UCHAR_MAX) {
                        i32 inv = 255 - cover;
                        u16 dp = *dst;
                        u8 dr = static_cast<u8>((static_cast<u8>((dp >> g_rUp)) << g_rDown));
                        u8 dg = static_cast<u8>((static_cast<u8>((dp >> g_gUp)) << g_gDown));
                        u8 db = static_cast<u8>((static_cast<u8>(dp) << g_bDown));
                        *dst = (static_cast<u8>((db * inv) / 256 + (blue * cover) / 256)
                                >> static_cast<u8>(g_bDown))
                               | (static_cast<u8>(
                                      (static_cast<u8>((dr * inv) / 256 + (red * cover) / 256))
                                      >> static_cast<u8>(g_rDown)
                                  )
                                  << g_rUp)
                               | (static_cast<u8>(
                                      (static_cast<u8>((dg * inv) / 256 + (green * cover) / 256))
                                      >> static_cast<u8>(g_gDown)
                                  )
                                  << g_gUp);
                    } else {
                        *dst = packedColor;
                    }
                    dst++;
                }
            }
        } else {
            for (i32 row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + y) * pitch) / 2 + destX;
                for (i32 col = startCol; col < clippedW; col++) {
                    if (glyphBuf[row * g.width + col] != 0) {
                        *dst = packedColor;
                    }
                    dst++;
                }
            }
        }
        destX += clippedW - startCol;
        firstCol = 0;
    }

    surf->m_ddSurface->Unlock(NULL);
}

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
        TextExtent m = MeasureWrapped(text, rc);
        rc.top = rc.top + (rc.bottom - rc.top) / 2 - m.height / 2;
    }

    i32 x = rc.left;

    CString line;
    while (rc.top < rc.bottom) {
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

        TextExtent e;
        e = MeasureText(text);
        if (e.width + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (rc.top + lineAdvance <= rc.bottom) {
                if (hcenter) {
                    TextExtent le = MeasureText(line);
                    i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
                    DrawLine(line, surf, cx, rc.top, z);
                    x = cx;
                } else {
                    DrawLine(line, surf, rc.left, rc.top, z);
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
            TextExtent he;
            he = MeasureText(head);
            i32 headW = he.width;
            text = text.Right(len - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.right - rc.left) {
                if (hcenter) {
                    TextExtent le = MeasureText(line);
                    i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
                    DrawLine(line, surf, cx, rc.top, z);
                } else {
                    DrawLine(line, surf, rc.left, rc.top, z);
                }
                rc.top = rc.top + lineAdvance;
                x = rc.left;
                line = "";
                if (lineAdvance + rc.top < rc.bottom) {
                    line += head;
                    x = headW + rc.left;
                }
            } else {

                while (head.GetLength() > 0) {
                    if (rc.top >= rc.bottom) {
                        break;
                    }
                    TextExtent ce;
                    ce = MeasureText(CString(head.GetAt(0), 1));
                    i32 chW = ce.width;
                    if (chW + x > rc.right) {
                        if (hcenter) {
                            TextExtent le = MeasureText(line);
                            i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
                            DrawLine(line, surf, cx, rc.top, z);
                        } else {
                            DrawLine(line, surf, rc.left, rc.top, z);
                        }
                        rc.top = rc.top + lineAdvance;
                        x = rc.left;
                        line = "";
                    }
                    if (lineAdvance + rc.top >= rc.bottom) {
                        break;
                    }
                    line += head[0];
                    x += chW;
                }
            }
            if (breakNL) {
                if (hcenter) {
                    TextExtent le = MeasureText(line);
                    i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
                    DrawLine(line, surf, cx, rc.top, z);
                } else {
                    DrawLine(line, surf, rc.left, rc.top, z);
                }
                rc.top = rc.top + lineAdvance;
                x = rc.left;
                line = "";
            }
        }
    }
    if (rc.top + lineAdvance <= rc.bottom && line.GetLength() > 0) {
        if (hcenter) {
            TextExtent le = MeasureText(line);
            i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
            DrawLine(line, surf, cx, rc.top, z);
        } else {
            DrawLine(line, surf, rc.left, rc.top, z);
        }
    }
}

RVA(0x0017ac50, 0xbd)
TextExtent FontRenderer::MeasureText(CString text) {
    TextExtent ext;

    Glyph g;
    g.height = 0;
    i32 i = 0;
    i32 width = 0;
    if (m_font == NULL) {
        return TextExtent(0, 0);
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
TextExtent FontRenderer::MeasureWrapped(CString text, CRect rc) {
    i32 y = rc.top;
    i32 maxWidth = 0;
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

        TextExtent e;
        e = MeasureText(text);
        if (e.width + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= rc.bottom) {
                TextExtent lw = MeasureText(line);
                i32 w = lw.width;
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

            TextExtent he;
            he = MeasureText(head);
            i32 headW = he.width;
            text = text.Right(text.GetLength() - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.right - rc.left) {
                TextExtent lw = MeasureText(line);
                i32 w = lw.width;
                if (maxWidth <= w) {
                    maxWidth = w;
                }
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
                if (m_font->GetMaxHeight() + y < rc.bottom) {
                    line += head;
                    x = headW + rc.left;
                }
            } else {

                for (i32 j = 0; j < head.GetLength(); j++) {
                    if (y >= rc.bottom) {
                        break;
                    }
                    TextExtent ce;
                    ce = MeasureText(CString(head.GetAt(j), 1));
                    i32 chW = ce.width;
                    if (chW + x > rc.right) {
                        y = y + m_font->GetMaxHeight();
                        x = rc.left;
                        TextExtent lw = MeasureText(line);
                        i32 w = lw.width;
                        if (maxWidth <= w) {
                            maxWidth = w;
                        }
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
    return TextExtent(maxWidth - rc.left + 1, m_font->GetMaxHeight() + (y - rc.top) + 1);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017b120, 0x3c6)
TextExtent FontRenderer::LayoutWrapped(CString text, CRect rc, i32* outLen) {
    i32 y = rc.top;
    i32 totalChars = 0;
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

        TextExtent e;
        e = MeasureText(text);
        if (e.width + x <= rc.right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= rc.bottom) {
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

            TextExtent he;
            he = MeasureText(head);
            i32 headW = he.width;
            text = text.Right(text.GetLength() - i - 1);
            if (headW + x < rc.right) {
                line += head;
                x = headW + x;
            } else if (headW < rc.right - rc.left) {
                totalChars += line.GetLength();
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
                if (m_font->GetMaxHeight() + y < rc.bottom) {
                    line += head;
                    x = headW + rc.left;
                }
            } else {

                while (head.GetLength() > 0) {
                    if (y >= rc.bottom) {
                        break;
                    }
                    TextExtent ce;
                    ce = MeasureText(CString(head.GetAt(0), 1));
                    i32 chW = ce.width;
                    if (chW + x > rc.right) {
                        y = y + m_font->GetMaxHeight();
                        x = rc.left;
                        totalChars += line.GetLength();
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
                totalChars += line.GetLength();
                y = y + m_font->GetMaxHeight();
                x = rc.left;
                line = "";
            }
        }
    }
    if (m_font->GetMaxHeight() + y <= rc.bottom && line.GetLength() > 0) {
        totalChars += line.GetLength();
    }
    if (outLen) {
        *outLen = totalChars;
    }
    return TextExtent(x, m_font->GetMaxHeight() + y + 1);
}

RVA_COMPGEN(0x0017b4f0, 0xc, ?GetAt@CString@@QBEDH@Z)
