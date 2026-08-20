#include <rva.h>

#include <DDrawMgr/DDrawShadeBlit.h>

#include <Mfc.h>

#include <DDrawMgr/ClutTable.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Ints.h>
#include <Io/FileStream.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>

#include <ddraw.h>
#include <string.h>

// The whole CDDrawShadeBlit compiland: 0x148ce0..0x14de04, one gapless .text run
// of 21 CDDrawShadeBlit methods with no foreign body inside it and no method of
// the class outside it. It used to be split three ways - src/Image/ImageOwned.cpp
// (0x148ce0..0x1495ca) and src/Image/ImageRle16Encode.cpp (0x1495d0..0x149776)
// were holding TUs invented by an earlier wave, not a retail partition; their
// "Image" library assignment in config/retail/link-order.tsv was derived from the
// src/ directory they sat in, so it was circular. Merging them back restores the
// preceding compiler state the blit half is compiled under.
//
// TU-STATE FINGERPRINT (diagnostic; probes are never shipped). A throwaway
// declaration above the first definition moves BlitShaded{Forward,Mirrored},
// ConvertRow{,Flip,Double,DoubleFwd} by several points and leaves BlitAt,
// Blit, BlitCopy{Forward,Mirrored} and the whole ImageOwned half untouched; the
// window is aperiodic over N = 1..16 and never reaches 100, and the kind of the
// probe (fwd-decl / typedef / empty class / class with members / static function
// with a body / file-scope float / string literal / class with inline members)
// only selects which subset moves. So the blit half's residue is genuinely
// TU-composition-sensitive, but it is not one missing declaration.
DATA(0x002bed08)
u8 g_scratch[1280];

DATA(0x002bf208)
CShadeTable* g_dstBySrcShadeTable = NULL;
DATA(0x002bf20c)
CShadeTable* g_dstByLevelShadeTable = NULL;
DATA(0x002bf210)
CShadeTable* g_srcByLevelShadeTable = NULL;
DATA(0x002bf214)
CShadeTable* g_lerpLevelShadeTable = NULL;
DATA(0x002bf218)
CShadeTable* g_greyShadeTable = NULL;
DATA(0x002bf21c)
CShadeTable* g_dstBySrc16ShadeTable = NULL;
DATA(0x002bf220)
CShadeTable* g_palette16ShadeTable = NULL;

static inline void Store16(u8* p, u16 v) {
    Pix16Ptr c;
    c.m_bytes = p;
    *c.m_words = v;
}
static inline u16 Load16(const u8* p) {
    Pix16CPtr c;
    c.m_bytes = p;
    return *c.m_words;
}

RVA(0x00148ce0, 0x2f)
CDDrawShadeBlit::CDDrawShadeBlit() {
    m_rleData = NULL;
    m_rleLen = 0;
    m_palDescr = NULL;
    m_drawType = SHADE_COPY;
    m_light = 0x80;
    m_doubleScanlines = 0;
    m_palette = NULL;
    m_srcBpp = PIXEL8_BYTES_PER_PIXEL;
    m_dstBpp = PIXEL8_BYTES_PER_PIXEL;
    m_colorKey = -1;
}

RVA(0x00148d10, 0x25)
void CDDrawShadeBlit::Teardown() {
    if (m_rleData) {
        delete[] m_rleData;
    }
    if (m_palette) {
        delete[] m_palette;
    }
}

// @early-stop
RVA(0x00148d40, 0x202)
i32 CDDrawShadeBlit::BuildRle(
    u8* pixels,
    i32 width,
    i32 height,
    i32 stride,
    i32 keyVal,
    PALETTEENTRY* palette
) {
    u8* src = pixels;
    if (src == NULL) {
        return 0;
    }
    m_colorKey = keyVal;
    if (stride == -1) {
        stride = width;
    }
    m_width = width;
    m_height = height;

    CByteArray ba;
    ba.SetSize(0, 0x3e8);

    i32 row = 0;
    if (m_height > 0) {
        do {
            i32 i = 0;
            i32 runStart = 0;
            if (m_width > 0) {
                do {
                    if (static_cast<i32>(src[i]) != keyVal) {

                        while (i < m_width && (i - runStart) < 0x7e
                               && static_cast<i32>(src[i]) != keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.GetSize(), static_cast<u8>((i - runStart)));
                        for (i32 j = runStart; j < i; j++) {
                            ba.SetAtGrow(ba.GetSize(), src[j]);
                        }
                        runStart = i;
                    } else {

                        while (i < m_width && (i - runStart) < 0x7e
                               && static_cast<i32>(src[i]) == keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.GetSize(), static_cast<u8>(((i - runStart) | 0x80)));
                        runStart = i;
                    }
                } while (i < m_width);
            }
            row++;
            src += stride;
        } while (row < m_height);
    }

    if (m_rleData != NULL) {
        delete[] m_rleData;
    }
    m_rleLen = ba.GetSize();
    m_rleData = new u8[ba.GetSize()];
    for (i32 k = 0; k < static_cast<i32>(m_rleLen); k++) {
        m_rleData[k] = ba.GetData()[k];
    }

    if (palette != NULL) {
        if (m_palette != NULL) {
            delete[] m_palette;
        }
        m_palette = new PALETTEENTRY[PALETTE_ENTRY_COUNT];
        memcpy(m_palette, palette, PALETTE_ENTRY_COUNT * sizeof(PALETTEENTRY));
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00148f50, 0x61)
i32 CDDrawShadeBlit::BuildFromSurface(CDDSurface* surf, i32 keyVal, PALETTEENTRY* palette) {
    if (surf == NULL) {
        return 0;
    }
    m_colorKey = keyVal;
    u8* bits = static_cast<u8*>(surf->Lock(0));
    if (bits == NULL) {
        return 0;
    }
    i32 r = BuildRle(bits, surf->m_width, surf->m_height, surf->m_pitch, keyVal, palette);
    surf->m_ddSurface->Unlock(0);
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00148fc0, 0x104)
i32 CDDrawShadeBlit::LoadFromFile(CString name, ColorDepth fmt) {
    CFile file;
    if (!file.Open(name, 0x8000, 0)) {
        return 0;
    }
    RecordBytes<PidHeader> fileData;
    fileData.m_bytes = new u8[file.GetLength()];
    file.Read(fileData.m_bytes, file.GetLength());
    i32 r = Build(fileData.m_rec, file.GetLength(), fmt);
    file.Close();
    delete[] fileData.m_bytes;
    return r;
}

// @early-stop
// The depth parameter is BYTE-wide (docs/patterns/
// byte-wide-compare-of-a-parameter-means-a-narrow-parameter.md): retail reads its
// home with `mov cl,BYTE PTR [esp+0x1c]` and compares `cmp cl,0x10` / `cmp cl,0x8`,
// which cl 5.0 never derives from an enum's value range. Residue: the embedded-
// palette copy loop, where retail re-loads m_rleLen and m_palette in front of EACH
// of the three channel stores and cl reloads them once per iteration. Retail also
// groups the address as `(pid + m_rleLen) + i` (`mov edx,esi / add edx,edi` then
// `[edx+eax+0x20]`) where cl groups `(pid + i) + m_rleLen` and needs a second
// register for m_palette, so it must rematerialise the constant 2 after the loop.
// Spelling the grouping explicitly - `(src->pixels + m_rleLen)[i]` - is
// byte-identical, so the association is cl's, not the source's.
RVA(0x001490d0, 0x173)
i32 CDDrawShadeBlit::Build(PidHeader* src, i32 size, GZ_ENUM_PARAM(ColorDepth, u8) fmt) {
    PidFlags flags = src->flags;

    if ((HAS(flags, PID_SRC_8BPP_SHADE)) || (HAS(flags, PID_SRC_8BPP))) {
        if (fmt == BPP_RGB_16) {
            m_srcBpp = PIXEL8_BYTES_PER_PIXEL;
            m_dstBpp = PIXEL16_BYTES_PER_PIXEL;
        } else {
            m_srcBpp = PIXEL8_BYTES_PER_PIXEL;
            m_dstBpp = PIXEL8_BYTES_PER_PIXEL;
        }
    } else if (fmt == BPP_RGB_16) {
        m_srcBpp = PIXEL16_BYTES_PER_PIXEL;
        m_dstBpp = PIXEL16_BYTES_PER_PIXEL;
    } else {
        m_srcBpp = PIXEL8_BYTES_PER_PIXEL;
        m_dstBpp = PIXEL8_BYTES_PER_PIXEL;
    }

    if (HAS(src->flags, PID_FILL_IS_WORD)) {
        m_colorKey = static_cast<u8>(src->fill);
    } else {
        m_colorKey = -1;
    }

    i32 stride = size - 0x20;
    m_rleLen = stride;
    if (fmt != BPP_PALETTED_8 && fmt != BPP_RGB_16) {
        return 0;
    }

    if (HAS(src->flags, PID_EMBEDDED_PALETTE)) {
        stride -= PALETTE_RGB_BYTE_COUNT;
        m_rleLen = stride;
        if (fmt == BPP_RGB_16) {
            if (m_palette != NULL) {
                delete[] m_palette;
            }
            m_palette = new PALETTEENTRY[PALETTE_ENTRY_COUNT];

            // The embedded palette sits directly after the RLE payload, so it
            // starts at pixels[m_rleLen] - retail forms `pid + m_rleLen` once
            // and indexes +0x20/+0x1e/+0x1f off it.
            i32 i = 0;
            i32 d = 0;
            do {
                d++;
                m_palette[d - 1].peRed = src->pixels[m_rleLen + i];
                i += 3;
                m_palette[d - 1].peGreen = src->pixels[m_rleLen + i - 2];
                m_palette[d - 1].peBlue = src->pixels[m_rleLen + i - 1];
            } while (i < 0x300);
        }
    }

    m_width = src->width;
    m_height = src->height;
    if (m_rleData != NULL) {
        delete[] m_rleData;
    }
    m_rleData = new u8[m_rleLen];

    memcpy(m_rleData, src->pixels, m_rleLen);

    if (m_srcBpp == PIXEL16_BYTES_PER_PIXEL) {
        u8* remapped = EncodeRle16(m_rleData);
        delete[] m_rleData;
        m_rleData = remapped;
        delete[] m_palette;
        m_palette = NULL;
    }
    return 1;
}

RVA(0x00149250, 0x158)
i32 CDDrawShadeBlit::DecodeFrame(CString name, CImageFrameRebuildDesc desc) {
    if (m_srcBpp != 1) {
        return 0;
    }

    CFile file;
    if (file.Open(name, 0x9001, 0) == 0) {
        return 0;
    }
    file.Write(&desc, sizeof(desc));
    file.Write(m_rleData, m_rleLen);
    if (desc.f1 & 0x80) {
        if (m_palette == NULL) {
            return 0;
        }
        for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
            file.Write(&m_palette[i].peRed, sizeof(m_palette[i].peRed));
            file.Write(&m_palette[i].peGreen, sizeof(m_palette[i].peGreen));
            file.Write(&m_palette[i].peBlue, sizeof(m_palette[i].peBlue));
        }
    }
    file.Close();
    return 1;
}

// desc.f1 is written directly as the MEMBER, never through a `flags` local: cl 5.0
// registerises the field in esi, keeps the 0x3d/0xbd/0x100/0x80 chain live, and
// never spills it back before the by-value argument copy (`lea esi,[esp+0x2c]`
// clobbers it) - so retail ships an UNINITIALISED f1 and the arithmetic survives
// as its ghost. A `flags` local cannot reproduce that: stored, it emits a real f1
// write; deleted, the whole chain folds away. See docs/patterns/
// registerized-member-miscompile-ships-uninitialized-field.md.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001493b0, 0xfd)

i32 CDDrawShadeBlit::Rebuild(CString name, i32 offsetX, i32 offsetY) {
    if (m_srcBpp != 1) {
        return 0;
    }
    CImageFrameRebuildDesc desc;
    desc.f0 = 0;
    desc.f1 = 0x3d;
    if (m_palette != NULL) {
        desc.f1 = 0xbd;
    }
    desc.f2 = m_width;
    desc.f3 = m_height;
    desc.f4 = offsetX;
    desc.f5 = offsetY;
    desc.f6 = 0;
    desc.f7 = 0;
    if (m_colorKey != -1) {
        desc.f6 = static_cast<u8>(m_colorKey);
        desc.f1 |= 0x100;
    }
    if (m_palette != NULL) {
        desc.f1 |= 0x80;
    }
    return DecodeFrame(name, desc);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001494b0, 0x11a)
i32 CDDrawShadeBlit::Decompress(u8* dest) {
    if (m_srcBpp != 1) {
        return 0;
    }
    if (dest == NULL) {
        return 0;
    }
    i32 fill = m_colorKey;
    if (fill == -1) {
        fill = 0;
    }
    i32 x = 0;
    i32 cursor = 0;
    for (i32 y = 0; y < m_height;) {
        if (m_rleData[cursor] & 0x80) {
            memset(dest + y * m_width + x, fill, m_rleData[cursor] - 0x80);
            x += m_rleData[cursor] - 0x80;
            cursor += 1;
        } else {
            memcpy(dest + y * m_width + x, m_rleData + cursor + 1, m_rleData[cursor]);
            x += m_rleData[cursor];
            cursor += m_rleData[cursor] + 1;
        }
        if (x >= m_width) {
            y++;
            x = 0;
        }
    }
    return 1;
}

// @early-stop
// 147/147 instructions on retail's frame (0x20c); the residue is one register
// coloring flip - cl gives srcidx edi and k esi where retail has them the other
// way round - plus `add ecx,2` for the two `outidx++` retail splits into two `inc`s
// (spelling them apart costs more than it saves).
RVA(0x001495d0, 0x1a6)
u8* CDDrawShadeBlit::EncodeRle16(const u8* src) {
    u16 table[256];
    {
        const PALETTEENTRY* pal = m_palette;
        u16* t = table;
        for (i32 i = 0x100; i != 0; i--) {
            *t++ = static_cast<u16>(
                ((static_cast<u16>(static_cast<u8>(pal->peGreen) >> g_gDown) << g_gUp)
                 | (static_cast<u16>(static_cast<u8>(pal->peRed) >> g_rDown) << g_rUp)
                 | static_cast<u16>(static_cast<u8>(pal->peBlue) >> g_bDown))
            );
            pal++;
        }
    }

    m_rleLen = 0;
    {
        i32 x = 0, row = 0, idx = 0;
        if (m_height > 0) {
            i32 w1 = m_width - 1;
            do {
                if (src[idx] & 0x80) {
                    m_rleLen++;
                    idx++;
                    x += static_cast<i32>(m_rleData[idx - 1]) - 0x80;
                } else {
                    m_rleLen++;
                    m_rleLen += static_cast<i32>(src[idx]) * 2;
                    x += static_cast<i32>(m_rleData[idx]);
                    idx += static_cast<i32>(m_rleData[idx]) + 1;
                }
                if (x >= w1) {
                    row++;
                    x = 0;
                }
            } while (row < m_height);
        }
    }

    u8* out = new u8[m_rleLen];
    {
        i32 outidx = 0, srcidx = 0;
        i32 x2 = 0, row2 = 0;
        if (m_height > 0) {
            do {
                u8 tk = src[srcidx];
                out[outidx] = tk;
                if (tk & 0x80) {
                    outidx++;
                    x2 += static_cast<i32>(m_rleData[srcidx]) - 0x80;
                    srcidx++;
                } else {
                    // Re-read the run length rather than naming it: a local is live
                    // across the loop and costs a spill slot retail does not have.
                    outidx++;
                    if (src[srcidx] > 0) {
                        const u8* run = src + srcidx + 1;
                        i32 k = 0;
                        do {
                            u16 px = table[run[k]];
                            out[outidx] = static_cast<u8>(px);
                            out[outidx + 1] = static_cast<u8>((px >> 8));
                            outidx += 2;
                            k++;
                        } while (k < src[srcidx]);
                    }
                    x2 += static_cast<i32>(m_rleData[srcidx]);
                    srcidx += static_cast<i32>(m_rleData[srcidx]) + 1;
                }
                if (x2 >= m_width - 1) {
                    row2++;
                    x2 = 0;
                }
            } while (row2 < m_height);
        }
    }
    return out;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00149780, 0x69)
i32 CDDrawShadeBlit::BlitAt(CDDSurface* dstSurf, i32 x, i32 y, i32 sel, i32 vflip) {
    ShadeRect clip;
    ShadeRect dst;
    clip.left = 0;
    clip.top = 0;
    clip.right = m_width - 1;
    clip.bottom = m_height - 1;
    dst.left = x;
    dst.top = y;
    dst.right = x + m_width - 1;
    dst.bottom = y + m_height - 1;
    return Blit(&dst, dstSurf, &clip, sel, vflip);
}

RVA(0x001497f0, 0x154)
i32 CDDrawShadeBlit::Blit(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 sel, i32 vflip) {
    if (clip->left < 0 || clip->right > m_width - 1 || clip->top < 0
        || clip->bottom > m_height - 1) {
        return 0;
    }

    i32 mode = src->m_bytesPerPixel;
    m_dstBpp = static_cast<u8>(mode);
    if (static_cast<u8>(mode) == PIXEL16_BYTES_PER_PIXEL) {
        if (g_rDown == PIXEL16_RED_DOWN && g_gDown == RGB555_GREEN_DOWN
            && g_bDown == PIXEL16_BLUE_DOWN && g_rUp == RGB555_RED_UP
            && g_gUp == PIXEL16_GREEN_UP) {
            m_blendVariant = 1;
        } else {
            m_blendVariant = 0;
        }
    }

    ShadeMode drawType = m_drawType;
    if (drawType == SHADE_COPY) {
        if (sel) {
            BlitCopyMirrored(dst, src, clip, vflip);
        } else {
            BlitCopyForward(dst, src, clip, vflip);
        }
        return 1;
    }

    if (drawType == SHADE_DST_BY_SRC_16) {
        if (m_srcBpp != PIXEL8_BYTES_PER_PIXEL
            || static_cast<u8>(mode) != PIXEL16_BYTES_PER_PIXEL) {
            return 0;
        }
    }
    if (drawType == SHADE_PAL_16 || drawType == SHADE_PAL_ALPHA_16) {
        if (m_srcBpp != PIXEL8_BYTES_PER_PIXEL
            || static_cast<u8>(mode) != PIXEL16_BYTES_PER_PIXEL) {
            return 0;
        }
    }
    if (drawType == SHADE_ALPHA_16 || drawType == SHADE_PAL_ALPHA_16) {
        i32 bank = (m_light >> 3) * CLUT_ALPHA_BANK_ENTRY_COUNT * sizeof(u16);
        // The channel offset is a link-time DIR32 addend on g_clut and the bank is
        // the run-time index, so the two must be added in that order: folding the
        // bank in first makes cl derive the other two channels from it and loses
        // one of the three relocations.
        ClutByteCursor red;
        red.m_words = g_clut;
        red.m_bytes += CLUT_RED_OFFSET * sizeof(u16);
        red.m_bytes += bank;
        m_lutBank0 = red.m_words;
        ClutByteCursor green;
        green.m_words = g_clut;
        green.m_bytes += CLUT_GREEN_OFFSET * sizeof(u16);
        green.m_bytes += bank;
        m_lutBank1 = green.m_words;
        ClutByteCursor blue;
        blue.m_words = g_clut;
        blue.m_bytes += CLUT_BLUE_OFFSET * sizeof(u16);
        blue.m_bytes += bank;
        m_lutBank2 = blue.m_words;
    }

    if (sel) {
        BlitShadedMirrored(dst, src, clip, vflip);
    } else {
        BlitShadedForward(dst, src, clip, vflip);
    }
    return 1;
}

// @early-stop
// REGISTER-HOMING wall. Arm structure and branch sequence agree with retail; the
// residue is which values win registers. Base carries two blocks retail does not
// (a lone `jmp` preheader plus a 1-instruction loop header) because cl spills the
// `clip` pointer and reloads it at the top of every arm-2 iteration where retail
// keeps it pinned in edi; the frame is 0xc against retail's 0x8 and every [esp+N]
// shifts with it. Not reachable from the loop form (while(1)+break is worse).
RVA(0x00149950, 0x3a1)
void CDDrawShadeBlit::BlitCopyForward(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(0));

    i32 pos = 0;
    i32 row = 0;
    i32 x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        }
    }

    i32 rowInc;
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        rowInc = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
        rowInc = pitch;
    }

    x = 0;
    if (clip->left == 0 && clip->right == m_width - 1) {

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                memcpy(
                    base + x * m_dstBpp,
                    &m_rleData[pos + 1],
                    static_cast<i32>(m_rleData[pos]) * m_srcBpp
                );
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    } else if (clip->left != 0) {

        while (row < clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x < clip->left) {
                i32 trans;
                do {
                    if (m_rleData[pos] & 0x80) {
                        x += m_rleData[pos] - 0x80;
                        pos++;
                        trans = 1;
                    } else {
                        x += m_rleData[pos];
                        pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x < clip->left);
                if (x > clip->left && trans == 0) {
                    i32 bytes = (x - clip->left) * m_srcBpp;
                    memcpy(base, &m_rleData[pos] - bytes, bytes);
                }
            }
            if (x < m_width) {
                if (m_rleData[pos] & 0x80) {
                    x += m_rleData[pos] - 0x80;
                    pos++;
                } else {
                    memcpy(
                        base + (x - clip->left) * m_dstBpp,
                        &m_rleData[pos + 1],
                        static_cast<i32>(m_rleData[pos]) * m_srcBpp
                    );
                    x += m_rleData[pos];
                    pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                }
            } else {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    } else if (clip->right != m_width - 1) {

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                i32 bytes;
                if (x + static_cast<i32>(m_rleData[pos]) >= clip->right) {
                    i32 vis = (clip->right - x) * m_srcBpp;
                    bytes = vis < 0 ? 0 : vis;
                } else {
                    bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;
                }
                memcpy(base + x * m_dstBpp, &m_rleData[pos + 1], bytes);
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    }

    surf->m_ddSurface->Unlock(0);
}

// @early-stop
// REGISTER-HOMING wall, same family as BlitCopyForward. Retail keeps the pitch in
// ebp across the Lock call where cl spills it (frame 0xc against retail's 0x8), and
// the top-skip loop diverges from its first block: base splits the loop head into a
// 1-instruction reload plus the test where retail tests in one block.
// The branch counts now agree (55/55): the third arm has NO `pos >= m_rleLen`
// break - retail's arm-3 loop head is only the `clip->right == m_width - 1` exit
// and the `row < clip->bottom` test, and it relies on the `x >= m_width` row
// advance to terminate. The extra break was ours.
RVA(0x00149d00, 0x4f8)
void CDDrawShadeBlit::BlitCopyMirrored(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(0));

    i32 pos = 0;
    i32 row = 0;
    i32 x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        }
    }
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        pitch = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
    }

    if (clip->left == 0 && clip->right == m_width - 1) {
        x = m_width;

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += 0x80 - static_cast<i32>(m_rleData[pos]);
                pos++;
            } else {
                i32 bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;
                u8* s = &m_rleData[pos + 1];
                u8* dst0 = base + x * m_dstBpp;
                if (m_srcBpp == 1) {
                    u8* d = dst0;
                    while (bytes-- > 0) {
                        *d-- = *s++;
                    }
                } else {
                    u16* d = Pix16(dst0);
                    u16* sw = Pix16(s);
                    while (bytes-- > 0) {
                        *d-- = *sw++;
                        bytes--;
                    }
                }
                x -= m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else if (clip->left != 0) {
        x = m_width;

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += 0x80 - static_cast<i32>(m_rleData[pos]);
                pos++;
            } else {
                u8* sd = &m_rleData[pos + 1];
                if (x - m_rleData[pos] <= clip->left) {
                    i32 vis = (x - clip->left) * m_srcBpp;
                    i32 bytes = vis < 0 ? 0 : vis;
                    u8* dbase = base + (x - clip->left) * m_dstBpp;
                    if (m_srcBpp == 1) {
                        u8* d = dbase;
                        while (bytes-- > 0) {
                            *d-- = *sd++;
                        }
                    } else {
                        u16* d = Pix16(dbase);
                        u16* sw = Pix16(sd);
                        while (bytes-- > 0) {
                            *d-- = *sw++;
                            bytes--;
                        }
                    }
                } else {
                    i32 bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;
                    u8* dbase = base + (x - clip->left) * m_dstBpp;
                    if (m_srcBpp == 1) {
                        u8* d = dbase;
                        while (bytes-- > 0) {
                            *d-- = *sd++;
                        }
                    } else {
                        u16* d = Pix16(dbase);
                        u16* sw = Pix16(sd);
                        while (bytes-- > 0) {
                            *d-- = *sw++;
                            bytes--;
                        }
                    }
                }
                x -= m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else if (m_width - 1 != clip->right) {
        x = m_width;

        while (row < clip->bottom) {
            if (x > clip->right) {
                i32 trans = 0;
                do {
                    if (m_rleData[pos] & 0x80) {
                        x += 0x80 - static_cast<i32>(m_rleData[pos]);
                        pos++;
                        trans = 1;
                    } else {
                        x -= m_rleData[pos];
                        pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x > clip->right);
                if (x >= 0 && trans == 0) {
                    i32 bytes = (clip->right - x) * m_srcBpp;
                    u8* s = &m_rleData[pos] - bytes;
                    if (m_srcBpp == 1) {
                        u8* d = base + clip->right * m_dstBpp;
                        while (bytes-- > 0) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = Pix16(base + clip->right * m_dstBpp);
                        u16* sw = Pix16(s);
                        while (bytes-- > 0) {
                            *d-- = *sw++;
                            bytes--;
                        }
                    }
                }
            }
            if (x > 0) {
                if (m_rleData[pos] & 0x80) {
                    x += 0x80 - static_cast<i32>(m_rleData[pos]);
                    pos++;
                } else {
                    i32 bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;
                    u8* s = &m_rleData[pos + 1];
                    u8* dst0 = base + x * m_dstBpp;
                    if (m_srcBpp == 1) {
                        u8* d = dst0;
                        while (bytes-- > 0) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = Pix16(dst0);
                        u16* sw = Pix16(s);
                        while (bytes-- > 0) {
                            *d-- = *sw++;
                            bytes--;
                        }
                    }
                    x -= m_rleData[pos];
                    pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                }
            } else {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    }

    surf->m_ddSurface->Unlock(0);
}

// @early-stop
// The conditional-branch census is 102/102 with one remaining target mismatch;
// the block skeleton is 196/197. Four structural facts were recovered from the
// target: arm 2's row loop is `row < clip->bottom` (retail
// `jge`, not `jg`); arm 3 tests `x + run >= clip->right` with the CLAMPED path as
// the if-body (retail `jl` to the full path), which is also what lets cl tail-merge
// the two ConvertRow call sites into one; the destination/source/count expressions
// are recomputed inside each doubleScanlines and each non-doubleScanlines arm rather
// than hoisted above the mode test; and the per-arm loop invariants (rd, the scratch
// and source biases) are declared INSIDE the loop so LICM lands them in the preheader
// where retail has them, not in the pre-guard block.
// Arm 1's non-doubleScanlines 16bpp arms now carry retail's one-cursor-plus-bias
// shape and its palette/scratch statement order (docs/patterns/
// scratch-loop-is-one-cursor-plus-biases.md).
// REGISTER-HOMING residue: the three lut loads inside every ALPHA/PAL_ALPHA arm.
// cl zero-extends each `mov reg16,[bank+idx*2]` with an `xor reg,reg` that retail
// omits (retail's accumulator's high bits are dead and it proves it), and it picks
// its own order for the `|` chain regardless of how the source spells it. Retyping
// the accumulator `u16` makes cl mask instead of zero, which is worse; reordering
// the terms moves nothing. Arm 1 also memory-homes x/pos where retail keeps them
// in esi/ebp.
// The arm chain is three EXPLICIT conditions, not an if/else-if/else: retail
// re-tests `clip->right != m_width - 1` at the head of arm 3 (`cmp eax,ecx; je
// <exit>` before the row and rleLen guards) because it cannot prove the memory
// did not change across arm 1. Spelling arm 3 as a bare `else` loses that branch.
RVA(0x0014a200, 0x1570)
void CDDrawShadeBlit::BlitShadedForward(
    ShadeRect* dst,
    CDDSurface* src,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = src->m_pitch;
    u8* base = static_cast<u8*>(src->Lock(0));

    u32 pos = 0;
    i32 row = 0, x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        }
    }

    i32 rowInc;
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        rowInc = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
        rowInc = pitch;
    }

    x = 0;
    if (clip->left == 0 && clip->right == m_width - 1) {

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                if (m_doubleScanlines) {
                    if ((dst->top + row) % 2) {

                        u8* d = base + x * m_dstBpp;
                        u8* s = &m_rleData[pos + 1];
                        i32 count = m_rleData[pos];
                        switch (m_drawType) {
                            case SHADE_DST_BY_SRC: {
                                u8* pal = m_palDescr->m_data;
                                memcpy(g_scratch, d, count);
                                u8* sc = g_scratch;
                                while (count-- > 0) {
                                    d[0] = pal[(*sc << 8) + *s];
                                    d[pitch] = pal[(*sc << 8) + *s];
                                    d++;
                                    sc++;
                                    s++;
                                }
                                break;
                            }
                            case SHADE_DST_BY_LEVEL: {
                                u8* pal = m_palDescr->m_data;
                                memcpy(g_scratch, d, count);
                                u8* sc = g_scratch;
                                while (count-- > 0) {
                                    d[0] = pal[(*sc << 8) + m_light];
                                    d[pitch] = pal[(*sc << 8) + m_light];
                                    d++;
                                    sc++;
                                }
                                break;
                            }
                            case SHADE_DST_BY_SRC_16: {
                                u16* pal1 = m_palDescr->Lut16();
                                u16* pal2 = g_greyShadeTable->Lut16();
                                memcpy(g_scratch, d, count * 2);
                                i32 sc = g_scratch - d;
                                while (count-- > 0) {
                                    i32 rd = pitch / 2 * 2;
                                    u32 idx = pal2[Load16(d + sc)];
                                    u32 hi = *s++;
                                    hi >>= 4;
                                    idx += hi << 12;
                                    u16 v = pal1[idx];
                                    Store16(d, v);
                                    Store16(d + rd, v);
                                    d += 2;
                                }
                                break;
                            }
                            case SHADE_ALPHA_16: {
                                memcpy(g_scratch, d, count * 2);
                                if (m_blendVariant) {
                                    u8* sc = g_scratch;
                                    while (count-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        i32 db = d - g_scratch;
                                        i32 sb = s - g_scratch;
                                        u32 dv = Load16(sc);
                                        u32 a = Load16(sc + sb);
                                        i32 v = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        u8* p = sc + db;
                                        Store16(p, v);
                                        Store16(p + rd, v);
                                        sc += 2;
                                    }
                                } else {
                                    u8* sc = g_scratch;
                                    while (count-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        i32 db = d - g_scratch;
                                        i32 sb = s - g_scratch;
                                        u32 dv = Load16(sc);
                                        u32 a = Load16(sc + sb);
                                        i32 v = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        u8* p = sc + db;
                                        Store16(p, v);
                                        Store16(p + rd, v);
                                        sc += 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {

                    u8* d = base + x * m_dstBpp;
                    u8* s = &m_rleData[pos + 1];
                    i32 count = m_rleData[pos];
                    switch (m_drawType) {
                        case SHADE_DST_BY_SRC: {
                            u8* pal = m_palDescr->m_data;
                            memcpy(g_scratch, d, count);
                            u8* sc = g_scratch;
                            while (count-- > 0) {
                                *d++ = pal[(*sc++ << 8) + *s++];
                            }
                            break;
                        }
                        case SHADE_DST_BY_SRC_16: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_greyShadeTable->Lut16();
                            memcpy(g_scratch, d, count * 2);
                            i32 sc = g_scratch - d;
                            while (count-- > 0) {
                                u32 idx = pal2[Load16(d + sc)];
                                d += 2;
                                u32 hi = *s++;
                                hi >>= 4;
                                idx += hi << 12;
                                Store16(d - 2, pal1[idx]);
                            }
                            break;
                        }
                        case SHADE_PAL_16: {
                            u16* pal = m_palDescr->Lut16();
                            while (count-- > 0) {
                                Store16(d, static_cast<u16>(pal[*s++]));
                                d += 2;
                            }
                            break;
                        }
                        case SHADE_ALPHA_16: {
                            memcpy(g_scratch, d, count * 2);
                            if (m_blendVariant) {
                                u8* sd = g_scratch;
                                while (count-- > 0) {
                                    i32 db = d - g_scratch;
                                    i32 sb = s - g_scratch;
                                    u32 bb = Load16(sd);
                                    u32 a = Load16(sd + sb);
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((bb >> 5) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    Store16(sd + db, r);
                                    sd += 2;
                                }
                            } else {
                                u8* sd = g_scratch;
                                while (count-- > 0) {
                                    i32 db = d - g_scratch;
                                    i32 sb = s - g_scratch;
                                    u32 bb = Load16(sd);
                                    u32 a = Load16(sd + sb);
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((bb >> 6) & ~0x1f)];
                                    Store16(sd + db, r);
                                    sd += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_PAL_ALPHA_16: {
                            memcpy(g_scratch, d, count * 2);
                            u16* pal = m_palDescr->Lut16();
                            if (m_blendVariant) {
                                u8* sd = g_scratch;
                                while (count-- > 0) {
                                    i32 db = d - g_scratch;
                                    u32 a = pal[*s++];
                                    u32 bb = Load16(sd);
                                    u16 r =
                                        m_lutBank1[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((bb >> 5) & ~0x1f)];
                                    r |= m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    Store16(sd + db, r);
                                    sd += 2;
                                }
                            } else {
                                u8* sd = g_scratch;
                                while (count-- > 0) {
                                    i32 db = d - g_scratch;
                                    u32 a = pal[*s++];
                                    u32 bb = Load16(sd);
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((bb >> 6) & ~0x1f)];
                                    Store16(sd + db, r);
                                    sd += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_DST_BY_LEVEL: {
                            u8* pbase = m_palDescr->m_data;
                            memcpy(g_scratch, d, count);
                            u8* sc = g_scratch;
                            while (count-- > 0) {
                                *d++ = pbase[(*sc++ << 8) + m_light];
                            }
                            break;
                        }
                        case SHADE_SRC_BY_LEVEL: {
                            u8* pbase = m_palDescr->m_data;
                            while (count-- > 0) {
                                *d++ = pbase[(*s++ << 8) + m_light];
                            }
                            break;
                        }
                        case SHADE_FILL_LEVEL: {
                            while (count-- > 0) {
                                *d++ = static_cast<u8>(m_light);
                            }
                            break;
                        }
                        case SHADE_LERP_LEVEL: {
                            u8* pal = m_palDescr->m_data;
                            memcpy(g_scratch, d, count);
                            u8* sc = g_scratch;
                            while (count-- > 0) {
                                i32 sv = pal[*sc++ + 0x100];
                                i32 dv = pal[*s + 0x100];
                                i32 t = (dv - sv) * m_light / 255 + sv;
                                *d++ = pal[t];
                                s++;
                            }
                            break;
                        }
                    }
                }
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    } else if (clip->left != 0) {

        while (row < clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x < clip->left) {
                i32 trans;
                do {
                    if (m_rleData[pos] & 0x80) {
                        x += m_rleData[pos] - 0x80;
                        pos++;
                        trans = 1;
                    } else {
                        x += m_rleData[pos];
                        pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x < clip->left);
                if (x > clip->left && trans == 0) {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            i32 vis = x - clip->left;
                            u8* d = base;
                            u8* s = &m_rleData[pos] - vis * m_srcBpp;
                            switch (m_drawType) {
                                case SHADE_DST_BY_SRC: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, vis);
                                    u8* sc = g_scratch;
                                    while (vis-- > 0) {
                                        d[0] = pal[(*sc << 8) + *s];
                                        d[pitch] = pal[(*sc << 8) + *s];
                                        d++;
                                        sc++;
                                        s++;
                                    }
                                    break;
                                }
                                case SHADE_DST_BY_LEVEL: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, vis);
                                    u8* sc = g_scratch;
                                    while (vis-- > 0) {
                                        d[0] = pal[(*sc << 8) + m_light];
                                        d[pitch] = pal[(*sc << 8) + m_light];
                                        d++;
                                        sc++;
                                    }
                                    break;
                                }
                                case SHADE_DST_BY_SRC_16: {
                                    u16* pal1 = m_palDescr->Lut16();
                                    u16* pal2 = g_greyShadeTable->Lut16();
                                    memcpy(g_scratch, d, vis * 2);
                                    u8* sc = g_scratch;
                                    while (vis-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        u32 idx = pal2[Load16(sc)];
                                        u32 hi = *s++;
                                        hi >>= 4;
                                        idx += hi << 12;
                                        u16 v = pal1[idx];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d += 2;
                                        sc += 2;
                                    }
                                    break;
                                }
                                case SHADE_ALPHA_16: {
                                    memcpy(g_scratch, d, vis * 2);
                                    u8* sc = g_scratch;
                                    if (m_blendVariant) {
                                        while (vis-- > 0) {
                                            i32 rd = pitch / 2 * 2;
                                            i32 db = d - g_scratch;
                                            i32 sb = s - g_scratch;
                                            u32 dv = Load16(sc);
                                            u32 a = Load16(sc + sb);
                                            i32 v =
                                                m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            u8* p = sc + db;
                                            Store16(p, v);
                                            Store16(p + rd, v);
                                            sc += 2;
                                        }
                                    } else {
                                        while (vis-- > 0) {
                                            i32 rd = pitch / 2 * 2;
                                            i32 db = d - g_scratch;
                                            i32 sb = s - g_scratch;
                                            u32 dv = Load16(sc);
                                            u32 a = Load16(sc + sb);
                                            i32 v =
                                                m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            u8* p = sc + db;
                                            Store16(p, v);
                                            Store16(p + rd, v);
                                            sc += 2;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        i32 vis = x - clip->left;
                        ConvertRow(base, &m_rleData[pos] - vis * m_srcBpp, vis);
                    }
                }
            }
            if (x < m_width) {
                if (m_rleData[pos] & 0x80) {
                    x += m_rleData[pos] - 0x80;
                    pos++;
                } else {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            i32 count = m_rleData[pos];
                            u8* s = &m_rleData[pos + 1];
                            u8* d = base + (x - clip->left) * m_dstBpp;
                            switch (m_drawType) {
                                case SHADE_DST_BY_SRC: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, count);
                                    u8* sc = g_scratch;
                                    while (count-- > 0) {
                                        d[0] = pal[(*sc << 8) + *s];
                                        d[pitch] = pal[(*sc << 8) + *s];
                                        d++;
                                        sc++;
                                        s++;
                                    }
                                    break;
                                }
                                case SHADE_DST_BY_LEVEL: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, count);
                                    u8* sc = g_scratch;
                                    while (count-- > 0) {
                                        d[0] = pal[(*sc << 8) + m_light];
                                        d[pitch] = pal[(*sc << 8) + m_light];
                                        d++;
                                        sc++;
                                    }
                                    break;
                                }
                                case SHADE_DST_BY_SRC_16: {
                                    u16* pal1 = m_palDescr->Lut16();
                                    u16* pal2 = g_greyShadeTable->Lut16();
                                    memcpy(g_scratch, d, count * 2);
                                    u8* sc = g_scratch;
                                    while (count-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        u32 idx = pal2[Load16(sc)];
                                        u32 hi = *s++;
                                        hi >>= 4;
                                        idx += hi << 12;
                                        u16 v = pal1[idx];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d += 2;
                                        sc += 2;
                                    }
                                    break;
                                }
                                case SHADE_ALPHA_16: {
                                    memcpy(g_scratch, d, count * 2);
                                    u8* sc = g_scratch;
                                    if (m_blendVariant) {
                                        while (count-- > 0) {
                                            i32 rd = pitch / 2 * 2;
                                            i32 db = d - g_scratch;
                                            i32 sb = s - g_scratch;
                                            u32 dv = Load16(sc);
                                            u32 a = Load16(sc + sb);
                                            i32 v =
                                                m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            u8* p = sc + db;
                                            Store16(p, v);
                                            Store16(p + rd, v);
                                            sc += 2;
                                        }
                                    } else {
                                        while (count-- > 0) {
                                            i32 rd = pitch / 2 * 2;
                                            i32 db = d - g_scratch;
                                            i32 sb = s - g_scratch;
                                            u32 dv = Load16(sc);
                                            u32 a = Load16(sc + sb);
                                            i32 v =
                                                m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            u8* p = sc + db;
                                            Store16(p, v);
                                            Store16(p + rd, v);
                                            sc += 2;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        ConvertRow(
                            base + (x - clip->left) * m_dstBpp,
                            &m_rleData[pos + 1],
                            m_rleData[pos]
                        );
                    }
                    x += m_rleData[pos];
                    pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                }
            } else {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    } else if (clip->right != m_width - 1) {

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                if (x + static_cast<i32>(m_rleData[pos]) >= clip->right) {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            i32 v = clip->right - x;
                            ConvertRowDoubleFwd(
                                base + x * m_dstBpp,
                                &m_rleData[pos + 1],
                                v < 0 ? 0 : v,
                                pitch
                            );
                        }
                    } else {
                        i32 v = clip->right - x;
                        ConvertRow(base + x * m_dstBpp, &m_rleData[pos + 1], v < 0 ? 0 : v);
                    }
                } else {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDoubleFwd(
                                base + x * m_dstBpp,
                                &m_rleData[pos + 1],
                                m_rleData[pos],
                                pitch
                            );
                        }
                    } else {
                        ConvertRow(base + x * m_dstBpp, &m_rleData[pos + 1], m_rleData[pos]);
                    }
                }
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    }

    src->m_ddSurface->Unlock(0);
}

// @early-stop
// Block skeleton matches retail exactly (166 blocks both sides, every branch target
// agreeing). Recovered from the target: arm 2 tests `x - run <= clip->left` with the
// INLINED switch as the if-body and the ConvertRowDouble/Flip calls as the else
// (retail `jg` to the calls), and arm 3's row advance is the ELSE of `x > 0`, not the
// if-body; both arms' address/count expressions are computed inside the branch that
// uses them, and the loop invariants sit inside the loop so LICM places them in the
// preheader.
// REGISTER-HOMING residue only: per-arm register coloring and the same
// second-induction-variable split described on BlitShadedForward.
RVA(0x0014b770, 0x1280)
void CDDrawShadeBlit::BlitShadedMirrored(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(0));

    i32 pos = 0, row = 0, x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & 0x80) {
                x += m_rleData[pos] - 0x80;
                pos++;
            } else {
                x += m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        }
    }

    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        pitch = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
    }

    if (clip->left == 0 && clip->right == m_width - 1) {
        x = m_width;

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += 0x80 - static_cast<i32>(m_rleData[pos]);
                pos++;
            } else {
                if (m_doubleScanlines) {
                    if ((dst->top + row) % 2) {

                        u8* d = base + x * m_dstBpp;
                        u8* s = &m_rleData[pos + 1];
                        i32 count = m_rleData[pos];
                        switch (m_drawType) {
                            case SHADE_DST_BY_SRC: {
                                u8* pbase = m_palDescr->m_data;
                                memcpy(g_scratch, d - count + 1, count);
                                u8* sc = &g_scratch[count - 1];
                                while (count-- > 0) {
                                    d[0] = pbase[(*sc << 8) + *s];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                    s++;
                                }
                                break;
                            }
                            case SHADE_DST_BY_LEVEL: {
                                u8* pbase = m_palDescr->m_data;
                                memcpy(g_scratch, d - count + 1, count);
                                u8* sc = &g_scratch[count - 1];
                                while (count-- > 0) {
                                    d[0] = pbase[(*sc << 8) + m_light];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                }
                                break;
                            }
                            case SHADE_DST_BY_SRC_16: {
                                u16* pal1 = m_palDescr->Lut16();
                                u16* pal2 = g_greyShadeTable->Lut16();
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u8* sc = &g_scratch[count * 2 - 2];
                                while (count-- > 0) {
                                    i32 rd = 2 * pitch / 2;
                                    u32 idx = pal2[Load16(sc)];
                                    u32 hi = *s++;
                                    hi >>= 4;
                                    idx += hi << 12;
                                    u16 v = pal1[idx];
                                    Store16(d, v);
                                    Store16(d + rd, v);
                                    d -= 2;
                                    sc -= 2;
                                }
                                break;
                            }
                            case SHADE_ALPHA_16: {
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u8* sc = &g_scratch[count * 2 - 2];
                                u8* ss2 = s;
                                if (m_blendVariant) {
                                    while (count-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        u32 a = Load16(ss2);
                                        u32 dv = Load16(sc);
                                        i32 v = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d -= 2;
                                        sc -= 2;
                                        ss2 += 2;
                                    }
                                } else {
                                    while (count-- > 0) {
                                        i32 rd = 2 * pitch / 2;
                                        u32 a = Load16(ss2);
                                        u32 dv = Load16(sc);
                                        i32 v = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d -= 2;
                                        sc -= 2;
                                        ss2 += 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {

                    u8* d = base + x * m_dstBpp;
                    u8* s = &m_rleData[pos + 1];
                    i32 count = m_rleData[pos];
                    u8* cbase = m_palDescr ? m_palDescr->m_data : s;
                    switch (m_drawType) {
                        case SHADE_DST_BY_SRC: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            while (count-- > 0) {
                                *d-- = cbase[*s++ + (*sc-- << 8)];
                            }
                            break;
                        }
                        case SHADE_DST_BY_SRC_16: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_greyShadeTable->Lut16();
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u8* sc = &g_scratch[count * 2 - 2];
                            while (count-- > 0) {
                                u32 idx = pal2[Load16(sc)];
                                u32 hi = *s++;
                                hi >>= 4;
                                idx += hi << 12;
                                Store16(d, static_cast<u16>(pal1[idx]));
                                d -= 2;
                                sc -= 2;
                            }
                            break;
                        }
                        case SHADE_PAL_16: {
                            u16* pal = m_palDescr->Lut16();
                            while (count-- > 0) {
                                Store16(d, static_cast<u16>(pal[*s++]));
                                d -= 2;
                            }
                            break;
                        }
                        case SHADE_ALPHA_16: {
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u8* sc = &g_scratch[count * 2 - 2];
                            u8* ss2 = s;
                            if (m_blendVariant) {
                                while (count-- > 0) {
                                    u32 a = Load16(ss2);
                                    u32 dv = Load16(sc);
                                    u16 r = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                    ss2 += 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = Load16(ss2);
                                    u32 dv = Load16(sc);
                                    u16 r = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                    ss2 += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_PAL_ALPHA_16: {
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* pal = m_palDescr->Lut16();
                            u8* sc = &g_scratch[count * 2 - 2];
                            if (m_blendVariant) {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = Load16(sc);
                                    u16 r = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = Load16(sc);
                                    u16 r = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                }
                            }
                            break;
                        }
                        case SHADE_DST_BY_LEVEL: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            while (count-- > 0) {
                                *d-- = cbase[(*sc-- << 8) + m_light];
                            }
                            break;
                        }
                        case SHADE_SRC_BY_LEVEL: {
                            while (count-- > 0) {
                                *d-- = cbase[(*s++ << 8) + m_light];
                            }
                            break;
                        }
                        case SHADE_FILL_LEVEL: {
                            while (count-- > 0) {
                                *d-- = static_cast<u8>(m_light);
                            }
                            break;
                        }
                        case SHADE_LERP_LEVEL: {
                            memcpy(g_scratch, d - count - 1, count);
                            u8* sc = &g_scratch[count + 1];
                            while (count-- > 0) {
                                i32 sv = cbase[*sc-- + 0x100];
                                i32 dv = cbase[*s + 0x100];
                                i32 t = (dv - sv) * m_light / 255 + sv;
                                *d-- = cbase[t];
                                s++;
                            }
                            break;
                        }
                    }
                }
                x -= m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else if (clip->left != 0) {
        x = m_width;

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & 0x80) {
                x += 0x80 - static_cast<i32>(m_rleData[pos]);
                pos++;
            } else {
                if (x - static_cast<i32>(m_rleData[pos]) <= clip->left) {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            i32 v = x - clip->left;
                            i32 vis = v < 0 ? 0 : v;
                            u8* s = &m_rleData[pos + 1];
                            u8* d = base + v * m_dstBpp;
                            switch (m_drawType) {
                                case SHADE_DST_BY_SRC: {
                                    u8* pbase = m_palDescr->m_data;
                                    memcpy(g_scratch, d - vis + 1, vis);
                                    u8* sc = &g_scratch[vis - 1];
                                    while (vis-- > 0) {
                                        d[0] = pbase[(*sc << 8) + *s];
                                        d[pitch] = pbase[(*sc << 8) + *s];
                                        d--;
                                        sc--;
                                        s++;
                                    }
                                    break;
                                }
                                case SHADE_DST_BY_LEVEL: {
                                    u8* pbase = m_palDescr->m_data;
                                    memcpy(g_scratch, d - vis + 1, vis);
                                    u8* sc = &g_scratch[vis - 1];
                                    while (vis-- > 0) {
                                        d[0] = pbase[(*sc << 8) + m_light];
                                        d[pitch] = pbase[(*sc << 8) + *s];
                                        d--;
                                        sc--;
                                    }
                                    break;
                                }
                                case SHADE_DST_BY_SRC_16: {
                                    u16* pal1 = m_palDescr->Lut16();
                                    u16* pal2 = g_greyShadeTable->Lut16();
                                    memcpy(g_scratch, d - vis * 2 - 2, vis * 2);
                                    u8* sc = &g_scratch[vis * 2 - 2];
                                    while (vis-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        u32 idx = pal2[Load16(sc)];
                                        u32 hi = *s++;
                                        hi >>= 4;
                                        idx += hi << 12;
                                        u16 v = pal1[idx];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d -= 2;
                                        sc -= 2;
                                    }
                                    break;
                                }
                                case SHADE_ALPHA_16: {
                                    memcpy(g_scratch, d - vis * 2 - 2, vis * 2);
                                    u8* sc = &g_scratch[vis * 2 - 2];
                                    u8* ss2 = s;
                                    if (m_blendVariant) {
                                        while (vis-- > 0) {
                                            i32 rd = pitch / 2 * 2;
                                            u32 dv = Load16(sc);
                                            u32 a = Load16(ss2);
                                            i32 v =
                                                m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d -= 2;
                                            sc -= 2;
                                            ss2 += 2;
                                        }
                                    } else {
                                        while (vis-- > 0) {
                                            i32 rd = pitch / 2 * 2;
                                            u32 dv = Load16(sc);
                                            u32 a = Load16(ss2);
                                            i32 v =
                                                m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d -= 2;
                                            sc -= 2;
                                            ss2 += 2;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        i32 v = x - clip->left;
                        i32 vis = v < 0 ? 0 : v;
                        ConvertRowFlip(base + v * m_dstBpp, &m_rleData[pos + 1], vis);
                    }
                    x -= m_rleData[pos];
                    pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                } else {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(
                                base + (x - clip->left) * m_dstBpp,
                                &m_rleData[pos + 1],
                                m_rleData[pos],
                                pitch
                            );
                        }
                    } else {
                        ConvertRowFlip(
                            base + (x - clip->left) * m_dstBpp,
                            &m_rleData[pos + 1],
                            m_rleData[pos]
                        );
                    }
                    x -= m_rleData[pos];
                    pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                }
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else if (clip->right != m_width - 1) {
        x = m_width;

        while (row < clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x > clip->right) {
                i32 trans = 0;
                do {
                    if (m_rleData[pos] & 0x80) {
                        x += 0x80 - static_cast<i32>(m_rleData[pos]);
                        pos++;
                        trans = 1;
                    } else {
                        x -= m_rleData[pos];
                        pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x > clip->right);
                if (x >= 0 && trans == 0) {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            i32 vis = clip->right - x;
                            ConvertRowDouble(
                                base + clip->right * m_dstBpp,
                                &m_rleData[pos] - vis * m_srcBpp,
                                vis,
                                pitch
                            );
                        }
                    } else {
                        i32 vis = clip->right - x;
                        ConvertRowFlip(
                            base + clip->right * m_dstBpp,
                            &m_rleData[pos] - vis * m_srcBpp,
                            vis
                        );
                    }
                }
            }
            if (x > 0) {
                if (m_rleData[pos] & 0x80) {
                    x += 0x80 - static_cast<i32>(m_rleData[pos]);
                    pos++;
                } else {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(
                                base + x * m_dstBpp,
                                &m_rleData[pos + 1],
                                m_rleData[pos],
                                pitch
                            );
                        }
                    } else {
                        ConvertRowFlip(base + x * m_dstBpp, &m_rleData[pos + 1], m_rleData[pos]);
                    }
                    x -= m_rleData[pos];
                    pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
                }
            } else {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    }

    surf->m_ddSurface->Unlock(0);
}

// @early-stop
// REGISTER-HOMING wall. Every arm's block skeleton matches; the residue is which
// value is spilled. In DST_BY_SRC_16 retail spills pal1 and keeps the trip count in
// edi, cl does the opposite and pays a reload+dec+store per iteration.
// The ALPHA and PAL_ALPHA arms now carry retail's ONE-cursor-plus-bias shape
// (retail 0x14cb70 `mov edx,0x6bed08 / sub eax,edx / sub ecx,edx` in the preheader,
// then `[edx]`, `[ecx+edx]`, store `[eax+edx]`) - see
// docs/patterns/scratch-loop-is-one-cursor-plus-biases.md.
// NOT a lever: reordering the three `|=` lut terms to retail's emission order. cl
// schedules that chain independently of source order in every arm of this TU.
RVA(0x0014c9f0, 0x5d0)
void CDDrawShadeBlit::ConvertRow(u8* dst, u8* src, i32 count) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* pal = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                u8* row = pal + (*sc++ << 8);
                *dst++ = row[*src++];
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_greyShadeTable->Lut16();
            memcpy(g_scratch, dst, count * 2);
            i32 sc = g_scratch - dst;
            while (count-- > 0) {
                u32 idx = pal2[Load16(dst + sc)];
                dst += 2;
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                Store16(dst - 2, pal1[idx]);
            }
            break;
        }
        case SHADE_PAL_16: {
            u16* pal = m_palDescr->Lut16();
            u8* sw = dst;
            while (count-- > 0) {
                Store16(sw, pal[*src++]);
                sw += 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u8* sc = g_scratch;
                while (count-- > 0) {
                    i32 db = dst - g_scratch;
                    i32 sb = src - g_scratch;
                    u32 a = Load16(sc + sb);
                    u32 b = Load16(sc);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((b >> 5) & ~0x1f)];
                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    Store16(sc + db, r);
                    sc += 2;
                }
            } else {
                u8* sc = g_scratch;
                while (count-- > 0) {
                    i32 db = dst - g_scratch;
                    i32 sb = src - g_scratch;
                    u32 a = Load16(sc + sb);
                    u32 b = Load16(sc);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((b >> 6) & ~0x1f)];
                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    Store16(sc + db, r);
                    sc += 2;
                }
            }
            break;
        }
        case SHADE_PAL_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            u16* pal = m_palDescr->Lut16();
            if (m_blendVariant) {
                u8* sc = g_scratch;
                while (count-- > 0) {
                    i32 db = dst - g_scratch;
                    u32 a = pal[*src++];
                    u32 b = Load16(sc);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((b >> 5) & ~0x1f)];
                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    Store16(sc + db, r);
                    sc += 2;
                }
            } else {
                u8* sc = g_scratch;
                while (count-- > 0) {
                    i32 db = dst - g_scratch;
                    u32 a = pal[*src++];
                    u32 b = Load16(sc);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((b >> 6) & ~0x1f)];
                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    Store16(sc + db, r);
                    sc += 2;
                }
            }
            break;
        }
        case SHADE_DST_BY_LEVEL: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                *dst++ = base[(*sc++ << 8) + m_light];
            }
            break;
        }
        case SHADE_SRC_BY_LEVEL: {
            u8* base = m_palDescr->m_data;
            while (count-- > 0) {
                *dst++ = base[(*src++ << 8) + m_light];
            }
            break;
        }
        case SHADE_FILL_LEVEL: {
            while (count-- > 0) {
                *dst++ = static_cast<u8>(m_light);
            }
            break;
        }
        case SHADE_LERP_LEVEL: {
            u8* pal = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                i32 s = pal[*sc++ + 0x100];
                i32 d = pal[*src + 0x100];
                i32 t = (d - s) * m_light / 255 + s;
                *dst++ = pal[t];
                src++;
            }
            break;
        }
    }
}

// @early-stop
// Each arm now takes a LOCAL copy of `src` (retail reloads the parameter slot in
// every arm preheader and never writes it back), which is what frees ebp for the
// m_palDescr ternary the way retail colours it.
// Residue: cl still preloads `src` into a register in the prologue for the ternary's
// else-arm where retail reads the slot in that arm only, and the LERP arm needs an
// entry `jmp` plus a 1-instruction loop header to reload the palette base.
// SCORING ARTIFACT, not a defect: retail's `lea reg,[idx*2+0x6bed06]` /
// `[idx+0x6bed07]` resolve to g_scratch MINUS 2 and MINUS 1, which land in the
// 4-byte alignment gap after g_DirectDrawMgr, so the delinker names them
// DAT_006bed06/07 while our reloc names g_scratch with a negative addend. Same
// bytes, different symbol - `reloc_multiset` reports it and it cannot be closed
// from source.
RVA(0x0014cfc0, 0x620)
void CDDrawShadeBlit::ConvertRowFlip(u8* dst, u8* src, i32 count) {
    u8* base = m_palDescr ? m_palDescr->m_data : src;
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            u8* ss = src;
            while (count-- > 0) {
                *dst-- = base[(*sc-- << 8) + *ss++];
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_greyShadeTable->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u8* sc = &g_scratch[count * 2 - 2];
            u8* ss = src;
            while (count-- > 0) {
                u32 idx = pal2[Load16(sc)];
                dst -= 2;
                sc -= 2;
                u32 hi = *ss++;
                hi >>= 4;
                idx += hi << 12;
                Store16(dst + 2, pal1[idx]);
            }
            break;
        }
        case SHADE_PAL_16: {
            u16* pal = m_palDescr->Lut16();
            u8* sw = dst;
            u8* ss = src;
            while (count-- > 0) {
                Store16(sw, pal[*ss++]);
                sw -= 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u8* sc = &g_scratch[count * 2 - 2];
            u8* ss = src;
            u8* sw = dst;
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    u16 r = m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(sw, r);
                    sc -= 2;
                    ss += 2;
                    sw -= 2;
                }
            } else {
                while (count-- > 0) {
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    u16 r = m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(sw, r);
                    sc -= 2;
                    ss += 2;
                    sw -= 2;
                }
            }
            break;
        }
        case SHADE_PAL_ALPHA_16: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* pal = m_palDescr->Lut16();
            u8* sc = &g_scratch[count * 2 - 2];
            u8* sw = dst;
            u8* ss = src;
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 a = pal[*ss++];
                    u32 d = Load16(sc);
                    u16 r = m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(sw, r);
                    sc -= 2;
                    sw -= 2;
                }
            } else {
                while (count-- > 0) {
                    u32 a = pal[*ss++];
                    u32 d = Load16(sc);
                    u16 r = m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(sw, r);
                    sc -= 2;
                    sw -= 2;
                }
            }
            break;
        }
        case SHADE_DST_BY_LEVEL: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            while (count-- > 0) {
                *dst-- = base[(*sc-- << 8) + m_light];
            }
            break;
        }
        case SHADE_SRC_BY_LEVEL: {
            u8* ss = src;
            while (count-- > 0) {
                *dst-- = base[(*ss++ << 8) + m_light];
            }
            break;
        }
        case SHADE_FILL_LEVEL: {
            while (count-- > 0) {
                *dst-- = static_cast<u8>(m_light);
            }
            break;
        }
        case SHADE_LERP_LEVEL: {
            memcpy(g_scratch, dst - count - 1, count);
            u8* sc = &g_scratch[count + 1];
            u8* ss = src;
            while (count-- > 0) {
                i32 s = base[*sc-- + 0x100];
                i32 d = base[*ss + 0x100];
                i32 t = (d - s) * m_light / 255 + s;
                *dst-- = base[t];
                ss++;
            }
            break;
        }
    }
}

// @early-stop
// Block skeleton matches retail (26 blocks both sides). REGISTER-HOMING residue:
// per-arm cursor-anchor and widen-idiom coins inside the double-scanline arms.
// Retail's SHADE_ALPHA_16 arm really is ONE g_scratch cursor plus db/sb biases
// (0x14d7f3 `sub eax,edi` twice in the preheader, then `lea eax,[edx+edi]` and
// `[eax]`/`[eax+edx]`), but spelling it that way makes cl anchor the induction
// variable on the STORE address instead and re-derive both biases from it - about
// 4.5 points worse, with or without an explicit `u8* p = sc + db;`. The
// three-cursor spelling is kept.
RVA(0x0014d5e0, 0x370)
void CDDrawShadeBlit::ConvertRowDoubleFwd(u8* dst, u8* src, i32 count, i32 rowDelta) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                dst[0] = base[(*sc << 8) + *src];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst++;
                sc++;
                src++;
            }
            break;
        }
        case SHADE_DST_BY_LEVEL: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                dst[0] = base[(*sc << 8) + m_light];
                dst[rowDelta] = base[(*sc << 8) + m_light];
                dst++;
                sc++;
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_greyShadeTable->Lut16();
            memcpy(g_scratch, dst, count * 2);
            i32 sc = g_scratch - dst;
            while (count-- > 0) {
                i32 rd = rowDelta / 2 * 2;
                u32 idx = pal2[Load16(dst + sc)];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                u16 v = pal1[idx];
                Store16(dst, v);
                Store16(dst + rd, v);
                dst += 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            u8* sc = g_scratch;
            u8* ss = src;
            if (m_blendVariant) {
                while (count-- > 0) {
                    i32 rd = rowDelta / 2 * 2;
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    i32 v = m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)]
                            | m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rd, v);
                    dst += 2;
                    sc += 2;
                    ss += 2;
                }
            } else {
                while (count-- > 0) {
                    i32 rd = rowDelta / 2 * 2;
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    i32 v = m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)]
                            | m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rd, v);
                    dst += 2;
                    sc += 2;
                    ss += 2;
                }
            }
            break;
        }
    }
}

// @early-stop
// The SHADE_DST_BY_LEVEL asymmetry (m_light for dst[0], *src for dst[rowDelta], src
// never advanced) is retail's, confirmed at 0x14d9f5 - do not "fix" it. Block
// skeleton matches retail (24 blocks both sides); REGISTER-HOMING residue only.
RVA(0x0014d950, 0x3a0)
void CDDrawShadeBlit::ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            while (count-- > 0) {
                dst[0] = base[(*sc << 8) + *src];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
                src++;
            }
            break;
        }
        case SHADE_DST_BY_LEVEL: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            while (count-- > 0) {
                dst[0] = base[(*sc << 8) + m_light];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_greyShadeTable->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u8* sc = &g_scratch[count * 2 - 2];
            while (count-- > 0) {
                i32 rd = rowDelta / 2 * 2;
                u32 idx = pal2[Load16(sc)];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                u16 v = pal1[idx];
                Store16(dst, v);
                Store16(dst + rd, v);
                dst -= 2;
                sc -= 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u8* sc = &g_scratch[count * 2 - 2];
            u8* ss = src;
            if (m_blendVariant) {
                while (count-- > 0) {
                    i32 rd = rowDelta / 2 * 2;
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
                    i32 v = m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)]
                            | m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rd, v);
                    dst -= 2;
                    sc -= 2;
                    ss += 2;
                }
            } else {
                while (count-- > 0) {
                    i32 rd = rowDelta / 2 * 2;
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
                    i32 v = m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)]
                            | m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rd, v);
                    dst -= 2;
                    sc -= 2;
                    ss += 2;
                }
            }
            break;
        }
    }
}

RVA(0x0014dcf0, 0xa0)
void SetShadeDescr(CShadeTable* v, ShadeMode mode) {
    switch (mode) {
        case SHADE_DST_BY_SRC:
            g_dstBySrcShadeTable = v;
            break;
        case SHADE_DST_BY_LEVEL:
            g_dstByLevelShadeTable = v;
            break;
        case SHADE_SRC_BY_LEVEL:
            g_srcByLevelShadeTable = v;
            break;
        case SHADE_LERP_LEVEL:
            g_lerpLevelShadeTable = v;
            break;
        case SHADE_DST_BY_SRC_16:
            g_dstBySrc16ShadeTable = v;
            break;
        case SHADE_PAL_16:
            g_palette16ShadeTable = v;
            break;
        case SHADE_PAL_ALPHA_16:
            g_palette16ShadeTable = v;
            break;
        case SHADE_GREY_TABLE:
            g_greyShadeTable = v;
            break;
    }
}

RVA(0x0014dd90, 0xa0)
void CDDrawShadeBlit::Select(ShadeMode mode, CShadeTable* descr) {
    m_drawType = mode;
    if (descr == NULL) {
        switch (mode) {
            case SHADE_DST_BY_SRC:
                m_palDescr = g_dstBySrcShadeTable;
                break;
            case SHADE_DST_BY_LEVEL:
                m_palDescr = g_dstByLevelShadeTable;
                break;
            case SHADE_SRC_BY_LEVEL:
                m_palDescr = g_srcByLevelShadeTable;
                break;
            case SHADE_LERP_LEVEL:
                m_palDescr = g_lerpLevelShadeTable;
                break;
            case SHADE_DST_BY_SRC_16:
                m_palDescr = g_dstBySrc16ShadeTable;
                break;
            case SHADE_PAL_16:
                m_palDescr = g_palette16ShadeTable;
                break;
            case SHADE_PAL_ALPHA_16:
                m_palDescr = g_palette16ShadeTable;
                break;
        }
    } else {
        m_palDescr = descr;
    }
}
