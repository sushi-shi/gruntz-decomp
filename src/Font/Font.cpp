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
#include <DDrawMgr/DDSurface.h> // CDDSurface - the draw family's surface arg (m_height in DrawLine)
#include <Font/Font.h>
#include <ddraw.h> // IDirectDrawSurface::Unlock (surf->m_8 COM dispatch in DrawGlyphRun)
#include <rva.h>

// The live screen RGB-format shift table at 0x683ea0..0x683eb4 (file RVA 0x283ea0..) -
// already named by SpriteRef.cpp / FileImage.cpp / LightFxRender.cpp. DrawGlyphRun packs
// m_color into the screen 16bpp format and (in the blend path) unpacks the dest pixel
// through these. Reloc-masked DIR32 data refs.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0  red   up-shift   (channel position in the 16bpp word)
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4  green up-shift
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac  red   down-shift (scale 8-bit -> 5/6-bit)
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0  green down-shift
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4  blue  down-shift

// The FontInterfaceObject::IsInterface1-5 GUID predicates (0x1794b0-0x179570), their
// g_guid1-5 tables, and CWapNodeB::FreeStrings (0x179680) are re-homed to the
// NetMgr.cpp tail (src/Net/NetMgr.cpp): they precede ??0Font (0x179700) in retail,
// i.e. they are NetMgr.cpp-obj code, not Font.cpp's (docs/exe-map/
// interval-dossiers.md calibration case; the netmgr+font seam).

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
    if (!file.Open(szFileName, 0, 0)) {
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

// ---------------------------------------------------------------------------
// Font::SaveFont
// The write mirror of LoadFont: open the named file for create (flags 0x1001),
// layer a store-mode CArchive over it, emit the glyph count, then for each glyph
// write its 8-byte metric record + its width*height pixel surface. Same /GX
// CFile/CArchive/CString stack frame as LoadFont. Returns 0 on open failure, 1
// once the font is fully written.
RVA(0x001799f0, 0x16d)
i32 Font::SaveFont(CString szFileName) {
    CFile file;
    if (!file.Open(szFileName, 0x1001, 0)) {
        return 0;
    }

    CArchive ar(&file, 0 /*CArchive::store*/, 0x1000, 0);

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

// Font::GetSurface (0x179b60) - &m_surfaces[c].
RVA(0x00179b60, 0x12)
void** Font::GetSurface(u8 c) {
    return &m_surfaces[c];
}

// Font::GetGlyph (0x179b80) - copy glyph metric c into out.
RVA(0x00179b80, 0x22)
Glyph& Font::GetGlyph(Glyph& out, u8 c) {
    out = m_glyphs[c];
    return out;
}

// Font::GetMaxHeight (0x179bd0) - the font line-height.
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

// FontRenderer::SetColor (0x179c20) - set the packed colour.
RVA(0x00179c20, 0xa)
void FontRenderer::SetColor(i32 color) {
    m_color = color;
}

// CharCursor::GetChar (0x0017b4f0) is now an inline member in the header.

// =========================================================================
// FontRenderer::DrawLine
// Public single-line entry: measure the run, reject it if it would overflow
// the destination surface's height, otherwise hand off to DrawLineClipped with
// the full-extent rect {0,0,ext.width,ext.height}. No-op when no font is
// loaded. Real arg flow (0x179c30 disasm): arg2 is the CDDSurface* itself
// (its +0x18 dwHeight is the vertical limit), forwarded as DrawLineClipped's
// surface; z is forwarded unchanged (the blend flag).
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

// =========================================================================
// FontRenderer::DrawLineClipped
// Draw one text run with up to two shadow passes: a white pass offset by
// (+1,+1) when m_clip is set, a black pass offset by (.,+2) when m_surface is
// set, then the main pass in m_color. Each pass forwards the same rect+text to
// the inner glyph-blit (DrawGlyphRun, external). The CString is taken by value
// (destroyed by the EH frame).
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

// FontRenderer::DrawGlyphRun (0x179e70, 0x5ec = 1516 B) - the inner glyph-run blit.
// Clip `rc` against the surface (m_width/m_height) and the measured text extent
// {0,0,mw,mh} (Win32 IntersectRect), Lock the surface, pack m_color into the screen
// 16bpp format, walk the run left-to-right skipping glyphs left of rc.left and stopping
// past rc.right, and for each visible glyph blit its 8bpp coverage buffer as 16bpp
// pixels: opaque (any non-zero coverage writes m_color) when `blend`==0, else per-pixel
// alpha-blended by the coverage byte (0=skip, 0xff=opaque, else src*cov+dst*(255-cov)).
// The by-value CString `text` forces the /GX EH frame; the MeasureText call copy-
// constructs a CString temp (destroyed by MeasureText); rc/x/y are text-space clip
// coords, (x,y) positions the run on the surface.
// @early-stop
// ~62% (from 0%): full body + control flow + the whole call/data set byte-match
// (MeasureText, IntersectRect, Lock, GetGlyph x3, GetSurface, m_8->Unlock, the 5
// g_r/g/b Up/Down pixel-format globals, the 16bpp pack + per-pixel alpha blend). The
// three DrawLineClipped CALLs (0x179d10 +0x84/+0xe3/+0x132) now BIND. Residual is a
// callee-saved-register-assignment wall: retail pins this=edi / surf=ebp / x=ebx, cl
// pins this=ebp / surf=esi, so every this/surf/x access re-encodes (different modrm)
// and cl additionally materializes m_color into a GP reg + spill to extract its bytes
// where retail reads this->m_color fresh (+~10 insns, +3 stack dwords -> frame 0x88 vs
// retail 0x7c). A global allocation decision, not steerable by operand order. Verified
// base-vs-target with the objdiff mnemonic stream (442 vs 432 insns).
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

    // Clip the run's extent to the surface (in-place widen/narrow of rc).
    if (x - rc.left + rc.right > surf->m_width) {
        rc.right = rc.right + rc.right - rc.left + x - surf->m_width;
    }
    if (y - rc.top + rc.bottom > surf->m_height) {
        rc.bottom = rc.bottom + rc.bottom - rc.top + y - surf->m_height;
    }

    // Intersect the clip rect with the measured text extent {0,0,mw,mh}.
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

    u16* bits = (u16*)surf->Lock(0);
    if (bits == 0) {
        return;
    }
    i32 pitch = surf->m_pitch;

    // Pack m_color into the screen 16bpp format.
    i32 destX = x;
    i32 red = m_color & 0xff;
    i32 green = (m_color >> 8) & 0xff;
    i32 blue = (m_color >> 16) & 0xff;
    i32 rightPartial = 0;
    i32 firstCol = 0;
    i32 packedColor = ((u8)((u8)red >> (u8)g_rDown) << g_rUp)
                      | ((u8)((u8)green >> (u8)g_gDown) << g_gUp) | ((u8)blue >> (u8)g_bDown);

    // Left clip: skip glyphs entirely left of rc.left; firstCol is the sub-glyph
    // column offset into the first partly-visible glyph.
    i32 startChar = 0;
    i32 acc = 0;
    if (rc.left != 0) {
        i32 prev = 0;
        while (acc < rc.left) {
            Glyph g;
            prev = acc;
            m_font->GetGlyph(g, text[startChar]);
            acc += g.width;
            startChar++;
        }
        startChar--;
        firstCol = rc.left - prev;
    }

    // Right clip: find the end char index + the right overshoot of the last glyph.
    i32 endChar;
    i32 w = 0;
    if (rc.right == m.width) {
        endChar = text.GetLength();
    } else {
        i32 j = 0;
        endChar = 0;
        if (rc.right >= 0) {
            do {
                Glyph g;
                m_font->GetGlyph(g, text[j]);
                w += g.width;
                j++;
            } while (w <= rc.right);
            endChar = j;
        }
        rightPartial = w - rc.right;
    }

    // Blit each visible glyph's coverage buffer as 16bpp pixels.
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
        u8* glyphBuf = (u8*)m_font->GetSurface(text[ci])[0];
        i32 startCol = firstCol;
        if (blend) {
            for (i32 row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + y) * pitch) / 2 + destX;
                for (i32 col = startCol; col < clippedW; col++) {
                    u8 cover = glyphBuf[row * gw + col];
                    if (cover == 0) {
                    } else if (cover == 0xff) {
                        *dst = (u16)packedColor;
                    } else {
                        i32 inv = 255 - cover;
                        u16 dp = *dst;
                        i32 dr = (u8)((u8)(dp >> g_rUp) << g_rDown);
                        i32 dg = (u8)((u8)(dp >> g_gUp) << g_gDown);
                        i32 db = (u8)((u8)dp << g_bDown);
                        i32 bB = (db * inv) / 256 + (blue * cover) / 256;
                        i32 rB = (dr * inv) / 256 + (red * cover) / 256;
                        i32 gB = (dg * inv) / 256 + (green * cover) / 256;
                        *dst = (u16)((u8)((u8)bB >> (u8)g_bDown)
                                     | ((u8)((u8)rB >> (u8)g_rDown) << g_rUp)
                                     | ((u8)((u8)gB >> (u8)g_gDown) << g_gUp));
                    }
                    dst++;
                }
            }
        } else {
            for (i32 row = rc.top; row < rc.bottom; row++) {
                u16* dst = bits + ((row - rc.top + y) * pitch) / 2 + destX;
                for (i32 col = startCol; col < clippedW; col++) {
                    if (glyphBuf[row * gw + col] != 0) {
                        *dst = (u16)packedColor;
                    }
                    dst++;
                }
            }
        }
        destX += clippedW - startCol;
        firstCol = 0;
    }

    surf->m_8->Unlock(0);
}

// =========================================================================
// FontRenderer::DrawWrapped  (0x17a460, 0x7ec = 2028 B), the cluster's largest.
// Word-wrap layout + draw: measures the block (MeasureWrapped) for vertical
// centering when `hcenter` is set, then greedily breaks the run into lines (the
// same skeleton as MeasureWrapped) and draws each line via DrawLine - centered
// horizontally within [x0, right] using TextRange::Span (the {x0, top, right}
// arg triple reinterpreted as a TextRange) when `hcenter` is set.
// @early-stop
// ~73% (from 0.5%): logic + control flow + the full call set (6 DrawLine, 8
// MeasureText, 5 Span, 2 Left, 17 CString ctors, 8 operator=, ...) byte-match.
// Residual is the temp-layout/regalloc/EH-state wall shared with its siblings,
// plus a codegen-form split on the MeasureWrapped-arg block (retail materializes
// the 4-int {x0,top,right,bottom} arg tuple via `sub esp,0x10`+stores; cl emits
// pushes). Verified base-vs-target with llvm-objdump -dr.
RVA(0x0017a460, 0x7ec)
void FontRenderer::DrawWrapped(
    CString text,
    CDDSurface* surf,
    i32 x0,
    i32 top,
    i32 right,
    i32 bottom,
    i32 z,
    i32 hcenter,
    i32 spacing
) {
    i32 lineAdvance = m_font->GetMaxHeight() + spacing;
    if (hcenter) {
        TextExtent m = MeasureWrapped(text, x0, top, right, bottom);
        top = top + (bottom - top) / 2 - m.height / 2;
    }

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
            if (y + lineAdvance <= bottom) {
                if (hcenter) {
                    i32 cx = x0 + ((TextRange*)&x0)->Span() / 2 - MeasureText(line).width / 2;
                    DrawLine(line, surf, cx, y, z);
                } else {
                    DrawLine(line, surf, x0, y, z);
                }
            }
            line = "";
        } else {
            i32 i = 0;
            i32 breakNL = 0;
            while (i < len) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < len && text[i] == '\n') {
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
            if (headW + x < right) {
                line += head;
                x = headW + x;
            } else if (headW < right - x0) {
                if (hcenter) {
                    i32 cx = x0 + ((TextRange*)&x0)->Span() / 2 - MeasureText(line).width / 2;
                    DrawLine(line, surf, cx, y, z);
                } else {
                    DrawLine(line, surf, x0, y, z);
                }
                y = y + lineAdvance;
                x = x0;
                line = "";
                if (lineAdvance + y < bottom) {
                    line += head;
                    x = headW + x0;
                }
            } else {
                if (head.GetLength() > 0) {
                    while (y < bottom) {
                        i32 chW =
                            MeasureText(CString((char)((CharCursor*)&head)->GetChar(0), 1)).width;
                        if (chW + x > right) {
                            if (hcenter) {
                                i32 cx = x0 + ((TextRange*)&x0)->Span() / 2
                                         - MeasureText(line).width / 2;
                                DrawLine(line, surf, cx, y, z);
                            } else {
                                DrawLine(line, surf, x0, y, z);
                            }
                            y = y + lineAdvance;
                            x = x0;
                            line = "";
                        }
                        if (lineAdvance + y >= bottom) {
                            break;
                        }
                        line += head[0];
                        x += chW;
                        if (head.GetLength() <= 0) {
                            break;
                        }
                    }
                }
            }
            if (breakNL) {
                if (hcenter) {
                    i32 cx = x0 + ((TextRange*)&x0)->Span() / 2 - MeasureText(line).width / 2;
                    DrawLine(line, surf, cx, y, z);
                } else {
                    DrawLine(line, surf, x0, y, z);
                }
                y = y + lineAdvance;
                x = x0;
                line = "";
            }
        }
    }
    if (y + lineAdvance <= bottom && line.GetLength() > 0) {
        if (hcenter) {
            i32 cx = x0 + ((TextRange*)&x0)->Span() / 2 - MeasureText(line).width / 2;
            DrawLine(line, surf, cx, y, z);
        } else {
            DrawLine(line, surf, x0, y, z);
        }
    }
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
        u8 c = text[i];
        m_font->GetGlyph(g, c);
        width += g.width;
    }
    ext.width = width;
    ext.height = m_font->GetMaxHeight();
    return ext;
}

// =========================================================================
// FontRenderer::MeasureWrapped  (0x17ad10, 0x402 = 1026 B)
// Greedy word-wrap bounding-box measurer. Walks `text` line by line from y=top
// down to y<bottom, greedily breaking on spaces/newlines, and returns the box
// {maxLineWidth - x0 + 1, lineHeight + (y - top) + 1}. A destructible CString
// `line` accumulator plus per-break Left/Right temps under the /GX EH frame.
// GetChar is dispatched on the `head` temp (the retail call takes a CString as
// `this` - modeled by reinterpreting &head as the FontRenderer accessor, which
// reads the same +0 char*; reloc-masked).
// @early-stop
// ~75.5% (from 0.13%): logic + control flow + CString-op sequence byte-exact.
// Residual is the temp-layout/regalloc wall - cl pins `this` in edi (retail esi)
// and DSE-eliminates the dead `e.height` store the retail keeps, so the /GX frame
// is 0x50 vs retail's 0x54; that 4-byte shift cascades every [esp+N] offset.
// Verified base-vs-target with llvm-objdump -dr.
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
            while (i < len) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < len && text[i] == '\n') {
                breakNL = 1;
            }
            CString head = text.Left(i + 1);
            i32 headW = MeasureText(head).width;
            text = text.Right(len - i - 1);
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
                i32 headLen = head.GetLength();
                if (headLen > 0) {
                    i32 j = 0;
                    while (y < bottom) {
                        i32 chW =
                            MeasureText(CString((char)((CharCursor*)&head)->GetChar(j), 1)).width;
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
                        j++;
                        if (j >= head.GetLength()) {
                            break;
                        }
                    }
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

// =========================================================================
// FontRenderer::LayoutWrapped  (0x17b120, 0x3c6 = 966 B) - the third wrap entry.
// Greedily lays out `text` from y=`begin` down to y<`bottom`, line by line, the
// same greedy-break skeleton as MeasureWrapped but accumulating a per-line char
// count (totalChars) instead of a max width. Returns the final cursor
// {x, lineHeight + y + 1} and writes totalChars to *outLen. The char-split path
// consumes head[0] (index 0, unlike MeasureWrapped's per-char index).
// @early-stop
// ~79.2% (from 4.2%): logic + control flow + CString-op sequence byte-exact.
// Same temp-layout/regalloc wall as MeasureWrapped (dead measure-height store DSE
// + a callee-saved-register pin shift); verified base-vs-target with llvm-objdump.
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

        TextExtent e = MeasureText(text);
        if (e.width + x <= right && !nl) {
            line += text;
            text = "";
            if (m_font->GetMaxHeight() + y <= bottom) {
                totalChars += line.GetLength();
            }
            line = "";
        } else {
            i32 i = 0;
            i32 breakNL = 0;
            while (i < len) {
                u8 ch = text[i];
                if (ch == ' ' || ch == '\n') {
                    break;
                }
                i++;
            }
            if (i < len && text[i] == '\n') {
                breakNL = 1;
            }
            CString head = text.Left(i + 1);
            i32 headW = MeasureText(head).width;
            text = text.Right(len - i - 1);
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
                if (head.GetLength() > 0) {
                    while (y < bottom) {
                        i32 chW =
                            MeasureText(CString((char)((CharCursor*)&head)->GetChar(0), 1)).width;
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
                        if (head.GetLength() <= 0) {
                            break;
                        }
                    }
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

// TextRange::Span (0x17b500) - byte distance end - begin.
RVA(0x0017b500, 0x8)
i32 TextRange::Span() {
    return m_end - m_begin;
}
