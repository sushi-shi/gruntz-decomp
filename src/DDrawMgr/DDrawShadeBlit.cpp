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
    m_light = SHADE_LIGHT_MIDPOINT;
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

                        while (i < m_width && (i - runStart) < SHADE_RLE_MAX_RUN
                               && static_cast<i32>(src[i]) != keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.GetSize(), static_cast<u8>((i - runStart)));
                        for (i32 j = runStart; j < i; j++) {
                            ba.SetAtGrow(ba.GetSize(), src[j]);
                        }
                        runStart = i;
                    } else {

                        while (i < m_width && (i - runStart) < SHADE_RLE_MAX_RUN
                               && static_cast<i32>(src[i]) == keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(
                            ba.GetSize(),
                            static_cast<u8>(((i - runStart) | SHADE_RLE_TRANSPARENT_FLAG))
                        );
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
    u8* bits = static_cast<u8*>(surf->Lock(NULL));
    if (bits == NULL) {
        return 0;
    }
    i32 r = BuildRle(bits, surf->m_width, surf->m_height, surf->m_pitch, keyVal, palette);
    surf->m_ddSurface->Unlock(NULL);
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00148fc0, 0x104)
i32 CDDrawShadeBlit::LoadFromFile(CString name, ColorDepth fmt) {
    CFile file;
    if (!file.Open(name, CFile::modeRead | CFile::typeBinary, NULL)) {
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

            i32 destIndex = 0;
            i32 sourceIndex = 0;
            do {
                destIndex++;
                m_palette[destIndex - 1].peRed = src->pixels[m_rleLen + sourceIndex];
                sourceIndex += 3;
                m_palette[destIndex - 1].peGreen = src->pixels[m_rleLen + sourceIndex - 2];
                m_palette[destIndex - 1].peBlue = src->pixels[m_rleLen + sourceIndex - 1];
            } while (sourceIndex < PALETTE_RGB_BYTE_COUNT);
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
i32 CDDrawShadeBlit::WritePidFile(CString path, PidWriteHeader header) {
    if (m_srcBpp != PIXEL8_BYTES_PER_PIXEL) {
        return 0;
    }

    CFile file;
    if (file.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, NULL) == false) {
        return 0;
    }
    file.Write(&header, sizeof(header));
    file.Write(m_rleData, m_rleLen);
    if (HAS(static_cast<PidFlags>(header.flags), PID_EMBEDDED_PALETTE)) {
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

// Preserved compiler bug: direct member writes leave header.flags uninitialized.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001493b0, 0xfd)

i32 CDDrawShadeBlit::SavePid(CString path, i32 offsetX, i32 offsetY) {
    if (m_srcBpp != PIXEL8_BYTES_PER_PIXEL) {
        return 0;
    }
    PidWriteHeader header;
    header.formatTag = 0;
    header.flags = 0x3d;
    if (m_palette != NULL) {
        header.flags = 0xbd;
    }
    header.width = m_width;
    header.height = m_height;
    header.offsetX = offsetX;
    header.offsetY = offsetY;
    header.fill = 0;
    header.reserved1c = 0;
    if (m_colorKey != -1) {
        header.fill = static_cast<u8>(m_colorKey);
        header.flags |= IDX(PID_FILL_IS_WORD);
    }
    if (m_palette != NULL) {
        header.flags |= IDX(PID_EMBEDDED_PALETTE);
    }
    return WritePidFile(path, header);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001494b0, 0x11a)
i32 CDDrawShadeBlit::Decompress(u8* dest) {
    if (m_srcBpp != PIXEL8_BYTES_PER_PIXEL) {
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
        if (m_rleData[cursor] & SHADE_RLE_TRANSPARENT_FLAG) {
            memset(dest + y * m_width + x, fill, m_rleData[cursor] - SHADE_RLE_TRANSPARENT_FLAG);
            x += m_rleData[cursor] - SHADE_RLE_TRANSPARENT_FLAG;
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
RVA(0x001495d0, 0x1a6)
u8* CDDrawShadeBlit::EncodeRle16(const u8* src) {
    u16 table[256];
    {
        const PALETTEENTRY* pal = m_palette;
        u16* t = table;
        for (i32 i = PALETTE_ENTRY_COUNT; i != 0; i--) {
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
                if (src[idx] & SHADE_RLE_TRANSPARENT_FLAG) {
                    m_rleLen++;
                    idx++;
                    x += static_cast<i32>(m_rleData[idx - 1]) - SHADE_RLE_TRANSPARENT_FLAG;
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
                if (tk & SHADE_RLE_TRANSPARENT_FLAG) {
                    outidx++;
                    x2 += static_cast<i32>(m_rleData[srcidx]) - SHADE_RLE_TRANSPARENT_FLAG;
                    srcidx++;
                } else {
                    outidx++;
                    if (src[srcidx] > 0) {
                        const u8* run = src + srcidx + 1;
                        i32 k = 0;
                        do {
                            u16 px = table[run[k]];
                            out[outidx] = static_cast<u8>(px);
                            out[outidx + 1] = static_cast<u8>((px >> PIXEL_BITS_PER_BYTE));
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
        i32 bank = (m_light >> CLUT_ALPHA_LEVEL_SHIFT) * CLUT_ALPHA_BANK_ENTRY_COUNT * sizeof(u16);
        // Keep the link-time channel offset separate from the runtime bank index.
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

RVA(0x00149950, 0x3a1)
void CDDrawShadeBlit::BlitCopyForward(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(NULL));

    i32 pos = 0;
    i32 row = 0;
    i32 x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
                    if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                        x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
                if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                    x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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

    surf->m_ddSurface->Unlock(NULL);
}

RVA(0x00149d00, 0x4f8)
void CDDrawShadeBlit::BlitCopyMirrored(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(NULL));

    i32 pos = 0;
    i32 row = 0;
    i32 x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
                    if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                        x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
                if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                    x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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

    surf->m_ddSurface->Unlock(NULL);
}

RVA(0x0014a200, 0x1570)
void CDDrawShadeBlit::BlitShadedForward(
    ShadeRect* dst,
    CDDSurface* src,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = src->m_pitch;
    u8* base = static_cast<u8*>(src->Lock(NULL));

    u32 pos = 0;
    i32 row = 0, x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
                                    d[0] = pal[(*sc << PALETTE_INDEX_BITS) + *s];
                                    d[pitch] = pal[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                    d[0] = pal[(*sc << PALETTE_INDEX_BITS) + m_light];
                                    d[pitch] = pal[(*sc << PALETTE_INDEX_BITS) + m_light];
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
                                    hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                    idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                        i32 v =
                                            m_lutBank0
                                                [(a >> RGB555_RED_UP)
                                                 + ((dv >> PIXEL16_GREEN_UP)
                                                    & ~RGB555_CHANNEL_MASK)]
                                            | m_lutBank1
                                                [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                                 + (((dv >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                                    << RGB555_CHANNEL_BITS)]
                                            | m_lutBank2
                                                [(a & RGB555_CHANNEL_MASK)
                                                 + ((dv & RGB555_CHANNEL_MASK)
                                                    << RGB555_CHANNEL_BITS)];
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
                                        i32 v = m_lutBank0
                                                    [(a >> RGB565_RED_UP)
                                                     + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                        & ~RGB555_CHANNEL_MASK)]
                                                | m_lutBank1
                                                    [((a >> RGB565_GREEN_TO_5_SHIFT)
                                                      & RGB555_CHANNEL_MASK)
                                                     + (((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                         & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)]
                                                | m_lutBank2
                                                    [(a & RGB555_CHANNEL_MASK)
                                                     + ((dv & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)];
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
                                *d++ = pal[(*sc++ << PALETTE_INDEX_BITS) + *s++];
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
                                hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                    u16 r = m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((bb & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank0
                                        [(a >> RGB555_RED_UP)
                                         + ((bb >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                                    r |= m_lutBank1
                                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                         + (((bb >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
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
                                    u16 r = m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((bb & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank1
                                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                         + (((bb >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank0
                                        [(a >> RGB565_RED_UP)
                                         + ((bb >> RGB565_GREEN_TO_5_SHIFT)
                                            & ~RGB555_CHANNEL_MASK)];
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
                                    u16 r = m_lutBank1
                                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                         + (((bb >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank0
                                        [(a >> RGB555_RED_UP)
                                         + ((bb >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                                    r |= m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((bb & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                                    Store16(sd + db, r);
                                    sd += 2;
                                }
                            } else {
                                u8* sd = g_scratch;
                                while (count-- > 0) {
                                    i32 db = d - g_scratch;
                                    u32 a = pal[*s++];
                                    u32 bb = Load16(sd);
                                    u16 r = m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((bb & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank1
                                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                         + (((bb >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank0
                                        [(a >> RGB565_RED_UP)
                                         + ((bb >> RGB565_GREEN_TO_5_SHIFT)
                                            & ~RGB555_CHANNEL_MASK)];
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
                                *d++ = pbase[(*sc++ << PALETTE_INDEX_BITS) + m_light];
                            }
                            break;
                        }
                        case SHADE_SRC_BY_LEVEL: {
                            u8* pbase = m_palDescr->m_data;
                            while (count-- > 0) {
                                *d++ = pbase[(*s++ << PALETTE_INDEX_BITS) + m_light];
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
                                i32 sv = pal[*sc++ + PALETTE_ENTRY_COUNT];
                                i32 dv = pal[*s + PALETTE_ENTRY_COUNT];
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
                    if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                        x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
                                        d[0] = pal[(*sc << PALETTE_INDEX_BITS) + *s];
                                        d[pitch] = pal[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                        d[0] = pal[(*sc << PALETTE_INDEX_BITS) + m_light];
                                        d[pitch] = pal[(*sc << PALETTE_INDEX_BITS) + m_light];
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
                                        hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                        idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                                m_lutBank0
                                                    [(a >> RGB555_RED_UP)
                                                     + ((dv >> PIXEL16_GREEN_UP)
                                                        & ~RGB555_CHANNEL_MASK)]
                                                | m_lutBank1
                                                    [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                                     + (((dv >> PIXEL16_GREEN_UP)
                                                         & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)]
                                                | m_lutBank2
                                                    [(a & RGB555_CHANNEL_MASK)
                                                     + ((dv & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)];
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
                                            i32 v = m_lutBank0
                                                        [(a >> RGB565_RED_UP)
                                                         + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                            & ~RGB555_CHANNEL_MASK)]
                                                    | m_lutBank1
                                                        [((a >> RGB565_GREEN_TO_5_SHIFT)
                                                          & RGB555_CHANNEL_MASK)
                                                         + (((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                             & RGB555_CHANNEL_MASK)
                                                            << RGB555_CHANNEL_BITS)]
                                                    | m_lutBank2
                                                        [(a & RGB555_CHANNEL_MASK)
                                                         + ((dv & RGB555_CHANNEL_MASK)
                                                            << RGB555_CHANNEL_BITS)];
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
                if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                    x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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
                                        d[0] = pal[(*sc << PALETTE_INDEX_BITS) + *s];
                                        d[pitch] = pal[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                        d[0] = pal[(*sc << PALETTE_INDEX_BITS) + m_light];
                                        d[pitch] = pal[(*sc << PALETTE_INDEX_BITS) + m_light];
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
                                        hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                        idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                                m_lutBank0
                                                    [(a >> RGB555_RED_UP)
                                                     + ((dv >> PIXEL16_GREEN_UP)
                                                        & ~RGB555_CHANNEL_MASK)]
                                                | m_lutBank1
                                                    [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                                     + (((dv >> PIXEL16_GREEN_UP)
                                                         & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)]
                                                | m_lutBank2
                                                    [(a & RGB555_CHANNEL_MASK)
                                                     + ((dv & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)];
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
                                            i32 v = m_lutBank0
                                                        [(a >> RGB565_RED_UP)
                                                         + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                            & ~RGB555_CHANNEL_MASK)]
                                                    | m_lutBank1
                                                        [((a >> RGB565_GREEN_TO_5_SHIFT)
                                                          & RGB555_CHANNEL_MASK)
                                                         + (((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                             & RGB555_CHANNEL_MASK)
                                                            << RGB555_CHANNEL_BITS)]
                                                    | m_lutBank2
                                                        [(a & RGB555_CHANNEL_MASK)
                                                         + ((dv & RGB555_CHANNEL_MASK)
                                                            << RGB555_CHANNEL_BITS)];
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
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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

    src->m_ddSurface->Unlock(NULL);
}

RVA(0x0014b770, 0x1280)
void CDDrawShadeBlit::BlitShadedMirrored(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(NULL));

    i32 pos = 0, row = 0, x = 0;

    if (clip->top != 0) {
        while (row < clip->top) {
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += m_rleData[pos] - SHADE_RLE_TRANSPARENT_FLAG;
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

    if (clip->left == 0 && clip->right == m_width - 1) {
        x = m_width;

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
                                    d[0] = pbase[(*sc << PALETTE_INDEX_BITS) + *s];
                                    d[pitch] = pbase[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                    d[0] = pbase[(*sc << PALETTE_INDEX_BITS) + m_light];
                                    d[pitch] = pbase[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                    hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                    idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                        i32 v = m_lutBank0
                                            [(a >> RGB555_RED_UP)
                                             + ((dv >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                                        v |= m_lutBank1
                                            [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                             + (((dv >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                                << RGB555_CHANNEL_BITS)];
                                        v |= m_lutBank2
                                            [(a & RGB555_CHANNEL_MASK)
                                             + ((dv & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
                                        i32 v = m_lutBank0
                                            [(a >> RGB565_RED_UP)
                                             + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                & ~RGB555_CHANNEL_MASK)];
                                        v |= m_lutBank1
                                            [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                             + (((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                 & RGB555_CHANNEL_MASK)
                                                << RGB555_CHANNEL_BITS)];
                                        v |= m_lutBank2
                                            [(a & RGB555_CHANNEL_MASK)
                                             + ((dv & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
                                *d-- = cbase[*s++ + (*sc-- << PALETTE_INDEX_BITS)];
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
                                hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                    u16 r = m_lutBank0
                                        [(a >> RGB555_RED_UP)
                                         + ((dv >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                                    r |= m_lutBank1
                                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                         + (((dv >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((dv & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                    ss2 += 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = Load16(ss2);
                                    u32 dv = Load16(sc);
                                    u16 r = m_lutBank0
                                        [(a >> RGB565_RED_UP)
                                         + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                            & ~RGB555_CHANNEL_MASK)];
                                    r |= m_lutBank1
                                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                         + (((dv >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((dv & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
                                    u16 r = m_lutBank0
                                        [(a >> RGB555_RED_UP)
                                         + ((dv >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                                    r |= m_lutBank1
                                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                         + (((dv >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((dv & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = Load16(sc);
                                    u16 r = m_lutBank0
                                        [(a >> RGB565_RED_UP)
                                         + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                            & ~RGB555_CHANNEL_MASK)];
                                    r |= m_lutBank1
                                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                         + (((dv >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                            << RGB555_CHANNEL_BITS)];
                                    r |= m_lutBank2
                                        [(a & RGB555_CHANNEL_MASK)
                                         + ((dv & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
                                *d-- = cbase[(*sc-- << PALETTE_INDEX_BITS) + m_light];
                            }
                            break;
                        }
                        case SHADE_SRC_BY_LEVEL: {
                            while (count-- > 0) {
                                *d-- = cbase[(*s++ << PALETTE_INDEX_BITS) + m_light];
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
                                i32 sv = cbase[*sc-- + PALETTE_ENTRY_COUNT];
                                i32 dv = cbase[*s + PALETTE_ENTRY_COUNT];
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
                base += rowInc;
                x = m_width;
            }
        }
    } else if (clip->left != 0) {
        x = m_width;

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
                                        d[0] = pbase[(*sc << PALETTE_INDEX_BITS) + *s];
                                        d[pitch] = pbase[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                        d[0] = pbase[(*sc << PALETTE_INDEX_BITS) + m_light];
                                        d[pitch] = pbase[(*sc << PALETTE_INDEX_BITS) + *s];
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
                                        hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                                        idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                                                m_lutBank0
                                                    [(a >> RGB555_RED_UP)
                                                     + ((dv >> PIXEL16_GREEN_UP)
                                                        & ~RGB555_CHANNEL_MASK)]
                                                | m_lutBank1
                                                    [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                                     + (((dv >> PIXEL16_GREEN_UP)
                                                         & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)]
                                                | m_lutBank2
                                                    [(a & RGB555_CHANNEL_MASK)
                                                     + ((dv & RGB555_CHANNEL_MASK)
                                                        << RGB555_CHANNEL_BITS)];
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
                                            i32 v = m_lutBank0
                                                        [(a >> RGB565_RED_UP)
                                                         + ((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                            & ~RGB555_CHANNEL_MASK)]
                                                    | m_lutBank1
                                                        [((a >> RGB565_GREEN_TO_5_SHIFT)
                                                          & RGB555_CHANNEL_MASK)
                                                         + (((dv >> RGB565_GREEN_TO_5_SHIFT)
                                                             & RGB555_CHANNEL_MASK)
                                                            << RGB555_CHANNEL_BITS)]
                                                    | m_lutBank2
                                                        [(a & RGB555_CHANNEL_MASK)
                                                         + ((dv & RGB555_CHANNEL_MASK)
                                                            << RGB555_CHANNEL_BITS)];
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
                base += rowInc;
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
                    if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                        x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
                if (m_rleData[pos] & SHADE_RLE_TRANSPARENT_FLAG) {
                    x += SHADE_RLE_TRANSPARENT_FLAG - static_cast<i32>(m_rleData[pos]);
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
                base += rowInc;
                x = m_width;
            }
        }
    }

    surf->m_ddSurface->Unlock(NULL);
}

RVA(0x0014c9f0, 0x5d0)
void CDDrawShadeBlit::ConvertRow(u8* dst, u8* src, i32 count) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* pal = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                u8* row = pal + (*sc++ << PALETTE_INDEX_BITS);
                *dst++ = row[*src++];
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_greyShadeTable->Lut16();
            memcpy(g_scratch, dst, count * 2);
            u8* pd = dst;
            u8* sc = g_scratch;
            while (count-- > 0) {
                u32 idx = pal2[Load16(sc)];
                sc += 2;
                u32 hi = *src++;
                hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                idx += hi << CLUT_ALPHA_INDEX_SHIFT;
                Store16(pd, pal1[idx]);
                pd += 2;
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
                    u16 r = m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((b & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB555_RED_UP) + ((b >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank1
                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                         + (((b >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
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
                    u16 r = m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((b & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB565_RED_UP)
                         + ((b >> RGB565_GREEN_TO_5_SHIFT) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank1
                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                         + (((b >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
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
                    u16 r = m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((b & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB555_RED_UP) + ((b >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank1
                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                         + (((b >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
                    Store16(sc + db, r);
                    sc += 2;
                }
            } else {
                u8* sc = g_scratch;
                while (count-- > 0) {
                    i32 db = dst - g_scratch;
                    u32 a = pal[*src++];
                    u32 b = Load16(sc);
                    u16 r = m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((b & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB565_RED_UP)
                         + ((b >> RGB565_GREEN_TO_5_SHIFT) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank1
                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                         + (((b >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
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
                *dst++ = base[(*sc++ << PALETTE_INDEX_BITS) + m_light];
            }
            break;
        }
        case SHADE_SRC_BY_LEVEL: {
            u8* base = m_palDescr->m_data;
            while (count-- > 0) {
                *dst++ = base[(*src++ << PALETTE_INDEX_BITS) + m_light];
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
                i32 s = pal[*sc++ + PALETTE_ENTRY_COUNT];
                i32 d = pal[*src + PALETTE_ENTRY_COUNT];
                i32 t = (d - s) * m_light / 255 + s;
                *dst++ = pal[t];
                src++;
            }
            break;
        }
    }
}

RVA(0x0014cfc0, 0x620)
void CDDrawShadeBlit::ConvertRowFlip(u8* dst, u8* src, i32 count) {
    u8* base = m_palDescr ? m_palDescr->m_data : src;
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            u8* ss = src;
            while (count-- > 0) {
                *dst-- = base[(*sc-- << PALETTE_INDEX_BITS) + *ss++];
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
                hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
            u8* sw = dst;
            u8* ss = src;
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    u16 r = m_lutBank1
                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                         + (((d >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB555_RED_UP) + ((d >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    Store16(sw, r);
                    sc -= 2;
                    ss += 2;
                    sw -= 2;
                }
            } else {
                while (count-- > 0) {
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    u16 r = m_lutBank1
                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                         + (((d >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB565_RED_UP)
                         + ((d >> RGB565_GREEN_TO_5_SHIFT) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
            u8* sc = &g_scratch[count * 2 - 2];
            u8* sw = dst;
            u16* pal = m_palDescr->Lut16();
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 d = Load16(sc);
                    u16 r = m_lutBank1
                        [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                         + (((d >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB555_RED_UP) + ((d >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    Store16(sw, r);
                    sc -= 2;
                    sw -= 2;
                }
            } else {
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 d = Load16(sc);
                    u16 r = m_lutBank1
                        [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                         + (((d >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                            << RGB555_CHANNEL_BITS)];
                    r |= m_lutBank0
                        [(a >> RGB565_RED_UP)
                         + ((d >> RGB565_GREEN_TO_5_SHIFT) & ~RGB555_CHANNEL_MASK)];
                    r |= m_lutBank2
                        [(a & RGB555_CHANNEL_MASK)
                         + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
                *dst-- = base[(*sc-- << PALETTE_INDEX_BITS) + m_light];
            }
            break;
        }
        case SHADE_SRC_BY_LEVEL: {
            u8* ss = src;
            while (count-- > 0) {
                *dst-- = base[(*ss++ << PALETTE_INDEX_BITS) + m_light];
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
                i32 s = base[*sc-- + PALETTE_ENTRY_COUNT];
                i32 d = base[*ss + PALETTE_ENTRY_COUNT];
                i32 t = (d - s) * m_light / 255 + s;
                *dst-- = base[t];
                ss++;
            }
            break;
        }
    }
}

RVA(0x0014d5e0, 0x370)
void CDDrawShadeBlit::ConvertRowDoubleFwd(u8* dst, u8* src, i32 count, i32 rowDelta) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                dst[0] = base[(*sc << PALETTE_INDEX_BITS) + *src];
                dst[rowDelta] = base[(*sc << PALETTE_INDEX_BITS) + *src];
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
                dst[0] = base[(*sc << PALETTE_INDEX_BITS) + m_light];
                dst[rowDelta] = base[(*sc << PALETTE_INDEX_BITS) + m_light];
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
                hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                idx += hi << CLUT_ALPHA_INDEX_SHIFT;
                u16 v = pal1[idx];
                Store16(dst, v);
                Store16(dst + rd, v);
                dst += 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u8* sc = g_scratch;
                u8* dd = dst;
                u8* ss = src;
                while (count-- > 0) {
                    i32 rd = rowDelta / 2 * 2;
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
                    i32 v = m_lutBank0
                                [(a >> RGB555_RED_UP)
                                 + ((d >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)]
                            | m_lutBank1
                                [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                 + (((d >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                    << RGB555_CHANNEL_BITS)]
                            | m_lutBank2
                                [(a & RGB555_CHANNEL_MASK)
                                 + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    Store16(dd, v);
                    Store16(dd + rd, v);
                    sc += 2;
                    dd += 2;
                    ss += 2;
                }
            } else {
                u8* sc = g_scratch;
                u8* dd = dst;
                u8* ss = src;
                while (count-- > 0) {
                    i32 rd = rowDelta / 2 * 2;
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
                    i32 v = m_lutBank0
                                [(a >> RGB565_RED_UP)
                                 + ((d >> RGB565_GREEN_TO_5_SHIFT) & ~RGB555_CHANNEL_MASK)]
                            | m_lutBank1
                                [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                 + (((d >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                    << RGB555_CHANNEL_BITS)]
                            | m_lutBank2
                                [(a & RGB555_CHANNEL_MASK)
                                 + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
                    Store16(dd, v);
                    Store16(dd + rd, v);
                    sc += 2;
                    dd += 2;
                    ss += 2;
                }
            }
            break;
        }
    }
}

RVA(0x0014d950, 0x3a0)
void CDDrawShadeBlit::ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            while (count-- > 0) {
                dst[0] = base[(*sc << PALETTE_INDEX_BITS) + *src];
                dst[rowDelta] = base[(*sc << PALETTE_INDEX_BITS) + *src];
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
                dst[0] = base[(*sc << PALETTE_INDEX_BITS) + m_light];
                dst[rowDelta] = base[(*sc << PALETTE_INDEX_BITS) + *src];
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
                hi >>= CLUT_ALPHA_NIBBLE_SHIFT;
                idx += hi << CLUT_ALPHA_INDEX_SHIFT;
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
                    i32 v = m_lutBank0
                                [(a >> RGB555_RED_UP)
                                 + ((d >> PIXEL16_GREEN_UP) & ~RGB555_CHANNEL_MASK)]
                            | m_lutBank1
                                [((a >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                 + (((d >> PIXEL16_GREEN_UP) & RGB555_CHANNEL_MASK)
                                    << RGB555_CHANNEL_BITS)]
                            | m_lutBank2
                                [(a & RGB555_CHANNEL_MASK)
                                 + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
                    i32 v = m_lutBank0
                                [(a >> RGB565_RED_UP)
                                 + ((d >> RGB565_GREEN_TO_5_SHIFT) & ~RGB555_CHANNEL_MASK)]
                            | m_lutBank1
                                [((a >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                 + (((d >> RGB565_GREEN_TO_5_SHIFT) & RGB555_CHANNEL_MASK)
                                    << RGB555_CHANNEL_BITS)]
                            | m_lutBank2
                                [(a & RGB555_CHANNEL_MASK)
                                 + ((d & RGB555_CHANNEL_MASK) << RGB555_CHANNEL_BITS)];
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
