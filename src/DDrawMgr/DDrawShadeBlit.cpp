#include <rva.h>

#include <DDrawMgr/DDrawShadeBlit.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Pix16.h>

#include <ddraw.h>
#include <string.h>

DATA(0x002bed08)
u8 g_scratch[1280];

DATA(0x002bf208)
CShadeTable* g_shadeDescr208 = 0;
DATA(0x002bf20c)
CShadeTable* g_shadeDescr20c = 0;
DATA(0x002bf210)
CShadeTable* g_shadeDescr210 = 0;
DATA(0x002bf214)
CShadeTable* g_shadeDescr214 = 0;
DATA(0x002bf218)
CShadeTable* g_blendDescr;
DATA(0x002bf21c)
CShadeTable* g_shadeDescr21c = 0;
DATA(0x002bf220)
CShadeTable* g_shadeDescr220 = 0;

static inline u16* Pix16(void* p) {
    return static_cast<u16*>(p);
}
static inline void Store16(u8* p, i32 v) {
    Pix16Ptr c;
    c.m_bytes = p;
    *c.m_words = static_cast<u16>(v);
}
static inline u16 Load16(const u8* p) {
    Pix16CPtr c;
    c.m_bytes = p;
    return *c.m_words;
}

static inline u16* Scratch16() {
    return Pix16(g_scratch);
}

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
        i32 bank = (m_light >> 3) * 0x800;
        m_lutBank0 = Pix16(g_clut + 0x20002 + bank);
        m_lutBank1 = Pix16(g_clut + 0x2 + bank);
        m_lutBank2 = Pix16(g_clut + 0x10002 + bank);
    }

    if (sel) {
        BlitShadedMirrored(dst, src, clip, vflip);
    } else {
        BlitShadedForward(dst, src, clip, vflip);
    }
    return 1;
}

// @early-stop
// retail re-reads clip->top at the loop bottom; cl hoists it and pays a spill
// slot, so the frame is 0xc against retail's 0x8 and every offset shifts.
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
                i32 trans = 0;
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
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            } else {
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
                if (x + static_cast<i32>(m_rleData[pos]) < clip->right) {
                    bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;
                } else {
                    i32 vis = (clip->right - x) * m_srcBpp;
                    bytes = vis < 0 ? 0 : vis;
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
// same hoisted clip->top as BlitCopyForward, plus retail keeps the pitch in ebp
// across the Lock call where cl spills it: frame 0xc against retail's 0x8.
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
                i32 bytes;
                if (x - m_rleData[pos] > clip->left) {
                    bytes = static_cast<i32>(m_rleData[pos]) * m_srcBpp;
                } else {
                    i32 vis = (x - clip->left) * m_srcBpp;
                    bytes = vis < 0 ? 0 : vis;
                }
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
                x -= m_rleData[pos];
                pos += static_cast<i32>(m_rleData[pos]) * m_srcBpp + 1;
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
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            } else {
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
            }
        }
    }

    surf->m_ddSurface->Unlock(0);
}

// @early-stop
// the shade/copy row loops are transcribed; the residue is /O2 register
// allocation and induction-variable choice inside the inlined row bodies.
// blend-term set now matches retail (ten m_lutBank0 references each side).
RVA(0x0014a200, 0x1570)
void CDDrawShadeBlit::BlitShadedForward(
    ShadeRect* dst,
    CDDSurface* src,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = src->m_pitch;
    u8* base = static_cast<u8*>(src->Lock(0));

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
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                u8* dst0 = base + x * m_dstBpp;
                u8* src0 = &m_rleData[pos + 1];
                i32 count = b;
                i32 i;
                if (m_doubleScanlines) {
                    if ((dst->top + row) % 2) {

                        u8* d = dst0;
                        u8* s = src0;
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
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d, count * 2);
                                u16* sc = Scratch16();
                                i32 rd = pitch / 2 * 2;
                                while (count-- > 0) {
                                    u32 idx = pal2[*sc++];
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
                                u16* sc = Scratch16();
                                u16* ss2 = Pix16(s);
                                i32 rd = pitch / 2 * 2;
                                if (m_blendVariant) {
                                    while (count-- > 0) {
                                        u32 dv = *sc++;
                                        u32 a = *ss2++;
                                        i32 v = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d += 2;
                                    }
                                } else {
                                    while (count-- > 0) {
                                        u32 dv = *sc++;
                                        u32 a = *ss2++;
                                        i32 v = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d += 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {

                    u8* d = dst0;
                    u8* s = src0;
                    switch (m_drawType) {
                        case SHADE_DST_BY_SRC: {
                            u8* pal = m_palDescr->m_data;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            while (count-- > 0) {
                                *d++ = pal[(*sc++ << 8) + *s++];
                            }
                            break;
                        }
                        case SHADE_DST_BY_SRC_16: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_blendDescr->Lut16();
                            u16* sc = Scratch16();
                            memcpy(g_scratch, d, count * 2);
                            while (count-- > 0) {
                                u32 idx = pal2[*sc++];
                                u32 hi = *s++;
                                hi >>= 4;
                                idx += hi << 12;
                                Store16(d, static_cast<u16>(pal1[idx]));
                                d += 2;
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
                                u16* sd = Scratch16();
                                u16* ss2 = Pix16(s);
                                while (count-- > 0) {
                                    u32 a = *ss2++;
                                    u32 bb = *sd++;
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((bb >> 5) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    Store16(d, r);
                                    d += 2;
                                }
                            } else {
                                u16* sd = Scratch16();
                                u16* ss2 = Pix16(s);
                                while (count-- > 0) {
                                    u32 a = *ss2++;
                                    u32 bb = *sd++;
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((bb >> 6) & ~0x1f)];
                                    Store16(d, r);
                                    d += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_PAL_ALPHA_16: {
                            u16* pal = m_palDescr->Lut16();
                            memcpy(g_scratch, d, count * 2);
                            if (m_blendVariant) {
                                u16* sd = Scratch16();
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 bb = *sd++;
                                    u16 r =
                                        m_lutBank1[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((bb >> 5) & ~0x1f)];
                                    r |= m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    Store16(d, r);
                                    d += 2;
                                }
                            } else {
                                u16* sd = Scratch16();
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 bb = *sd++;
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((bb >> 6) & ~0x1f)];
                                    Store16(d, r);
                                    d += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_DST_BY_LEVEL: {
                            u8* pbase = m_palDescr->m_data;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
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
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
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
                x += b;
                pos += static_cast<i32>(b) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    } else if (clip->left != 0) {

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x < clip->left) {
                i32 trans = 0;
                do {
                    u8 b = m_rleData[pos];
                    if (b & 0x80) {
                        x += b - 0x80;
                        pos++;
                        trans = 1;
                    } else {
                        x += b;
                        pos += static_cast<i32>(b) * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x < clip->left);
                if (x > clip->left && trans == 0) {
                    i32 vis = x - clip->left;
                    u8* dd = base;
                    u8* ss = &m_rleData[pos] - vis * m_srcBpp;
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            u8* d = dd;
                            u8* s = ss;
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
                                    u16* pal2 = g_blendDescr->Lut16();
                                    memcpy(g_scratch, d, vis * 2);
                                    u16* sc = Scratch16();
                                    i32 rd = pitch / 2 * 2;
                                    while (vis-- > 0) {
                                        u32 idx = pal2[*sc++];
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
                                    memcpy(g_scratch, d, vis * 2);
                                    u16* sc = Scratch16();
                                    u16* ss2 = Pix16(s);
                                    i32 rd = pitch / 2 * 2;
                                    if (m_blendVariant) {
                                        while (vis-- > 0) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            i32 v =
                                                m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d += 2;
                                        }
                                    } else {
                                        while (vis-- > 0) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            i32 v =
                                                m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d += 2;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        ConvertRow(dd, ss, vis);
                    }
                }
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            } else {
                u8 b = m_rleData[pos];
                if (b & 0x80) {
                    x += b - 0x80;
                    pos++;
                } else {
                    u8* dd = base + (x - clip->left) * m_dstBpp;
                    u8* ss = &m_rleData[pos + 1];
                    i32 count = b;
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            u8* d = dd;
                            u8* s = ss;
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
                                    u16* pal2 = g_blendDescr->Lut16();
                                    memcpy(g_scratch, d, count * 2);
                                    u16* sc = Scratch16();
                                    i32 rd = pitch / 2 * 2;
                                    while (count-- > 0) {
                                        u32 idx = pal2[*sc++];
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
                                    u16* sc = Scratch16();
                                    u16* ss2 = Pix16(s);
                                    i32 rd = pitch / 2 * 2;
                                    if (m_blendVariant) {
                                        while (count-- > 0) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            i32 v =
                                                m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d += 2;
                                        }
                                    } else {
                                        while (count-- > 0) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            i32 v =
                                                m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d += 2;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        ConvertRow(dd, ss, count);
                    }
                    x += b;
                    pos += static_cast<i32>(b) * m_srcBpp + 1;
                }
            }
        }
    } else {

        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                i32 vis;
                if (x + static_cast<i32>(b) < clip->right) {
                    vis = b;
                } else {
                    i32 v = clip->right - x;
                    vis = v < 0 ? 0 : v;
                }
                u8* dd = base + x * m_dstBpp;
                u8* ss = &m_rleData[pos + 1];
                if (m_doubleScanlines) {
                    if ((dst->top + row) % 2) {
                        ConvertRowDoubleFwd(dd, ss, vis, pitch);
                    }
                } else {
                    ConvertRow(dd, ss, vis);
                }
                x += b;
                pos += static_cast<i32>(b) * m_srcBpp + 1;
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
// Call counts and branch counts now agree with retail (3 ConvertRowDouble,
// 4 ConvertRowFlip, 91 branches); the residue is ~37 instructions of address
// arithmetic retail recomputes per site and we hoist.
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
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += 0x80 - static_cast<i32>(b);
                pos++;
            } else {
                u8* dst0 = base + x * m_dstBpp;
                u8* src0 = &m_rleData[pos + 1];
                i32 count = b;
                i32 i;
                if (m_doubleScanlines) {
                    if ((dst->top + row) % 2) {

                        u8* d = dst0;
                        u8* s = src0;
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
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u16* sc = (Scratch16() + count - 1);
                                i32 rd = pitch / 2 * 2;
                                while (count-- > 0) {
                                    u32 idx = pal2[*sc--];
                                    u32 hi = *s++;
                                    hi >>= 4;
                                    idx += hi << 12;
                                    u16 v = pal1[idx];
                                    Store16(d, v);
                                    Store16(d + rd, v);
                                    d -= 2;
                                }
                                break;
                            }
                            case SHADE_ALPHA_16: {
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u16* sc = (Scratch16() + count - 1);
                                u16* ss2 = Pix16(s);
                                if (m_blendVariant) {
                                    i32 rd = pitch / 2 * 2;
                                    while (count-- > 0) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        i32 v = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d -= 2;
                                    }
                                } else {
                                    i32 rd = pitch / 2 * 2;
                                    while (count-- > 0) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        i32 v = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d -= 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {

                    u8* d = dst0;
                    u8* s = src0;
                    u8* cbase = m_palDescr ? m_palDescr->m_data : s;
                    switch (m_drawType) {
                        case SHADE_DST_BY_SRC: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            while (count-- > 0) {
                                *d-- = cbase[(*sc-- << 8) + *s++];
                            }
                            break;
                        }
                        case SHADE_DST_BY_SRC_16: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_blendDescr->Lut16();
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (Scratch16() + count - 1);
                            while (count-- > 0) {
                                u32 idx = pal2[*sc--];
                                u32 hi = *s++;
                                hi >>= 4;
                                idx += hi << 12;
                                Store16(d, static_cast<u16>(pal1[idx]));
                                d -= 2;
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
                            u16* sc = (Scratch16() + count - 1);
                            u16* ss2 = Pix16(s);
                            if (m_blendVariant) {
                                while (count-- > 0) {
                                    u32 a = *ss2++;
                                    u32 dv = *sc--;
                                    u16 r =
                                        m_lutBank1[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)];
                                    Store16(d, r);
                                    d -= 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = *ss2++;
                                    u32 dv = *sc--;
                                    u16 r = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    Store16(d, r);
                                    d -= 2;
                                }
                            }
                            break;
                        }
                        case SHADE_PAL_ALPHA_16: {
                            u16* pal = m_palDescr->Lut16();
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (Scratch16() + count - 1);
                            if (m_blendVariant) {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = *sc--;
                                    u16 r =
                                        m_lutBank1[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)];
                                    Store16(d, r);
                                    d -= 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = *sc--;
                                    u16 r =
                                        m_lutBank1[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)];
                                    Store16(d, r);
                                    d -= 2;
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
                x -= b;
                pos += static_cast<i32>(b) * m_srcBpp + 1;
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
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += 0x80 - static_cast<i32>(b);
                pos++;
            } else {
                i32 cnt = b;
                u8* ss = &m_rleData[pos + 1];
                if (x - cnt > clip->left) {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(base + (x - clip->left) * m_dstBpp, ss, cnt, pitch);
                        }
                    } else {
                        ConvertRowFlip(base + (x - clip->left) * m_dstBpp, ss, cnt);
                    }
                    x -= cnt;
                    pos += cnt * m_srcBpp + 1;
                } else {
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            i32 v = x - clip->left;
                            i32 vis = v < 0 ? 0 : v;
                            u8* d = base + v * m_dstBpp;
                            u8* s = ss;
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
                                    u16* pal2 = g_blendDescr->Lut16();
                                    memcpy(g_scratch, d - vis * 2 - 2, vis * 2);
                                    u16* sc = (Scratch16() + vis - 1);
                                    i32 rd = pitch / 2 * 2;
                                    while (vis-- > 0) {
                                        u32 idx = pal2[*sc--];
                                        u32 hi = *s++;
                                        hi >>= 4;
                                        idx += hi << 12;
                                        u16 v = pal1[idx];
                                        Store16(d, v);
                                        Store16(d + rd, v);
                                        d -= 2;
                                    }
                                    break;
                                }
                                case SHADE_ALPHA_16: {
                                    memcpy(g_scratch, d - vis * 2 - 2, vis * 2);
                                    u16* sc = (Scratch16() + vis - 1);
                                    u16* ss2 = Pix16(s);
                                    if (m_blendVariant) {
                                        i32 rd = pitch / 2 * 2;
                                        while (vis-- > 0) {
                                            u32 a = *ss2++;
                                            u32 dv = *sc--;
                                            i32 v =
                                                m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d -= 2;
                                        }
                                    } else {
                                        i32 rd = pitch / 2 * 2;
                                        while (vis-- > 0) {
                                            u32 a = *ss2++;
                                            u32 dv = *sc--;
                                            i32 v =
                                                m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, v);
                                            Store16(d + rd, v);
                                            d -= 2;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        i32 v = x - clip->left;
                        i32 vis = v < 0 ? 0 : v;
                        ConvertRowFlip(base + v * m_dstBpp, ss, vis);
                    }
                    x -= cnt;
                    pos += cnt * m_srcBpp + 1;
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
                    u8 b = m_rleData[pos];
                    if (b & 0x80) {
                        x += 0x80 - static_cast<i32>(b);
                        pos++;
                        trans = 1;
                    } else {
                        x -= b;
                        pos += static_cast<i32>(b) * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x > clip->right);
                if (x >= 0 && trans == 0) {
                    i32 vis = clip->right - x;
                    u8* ss = &m_rleData[pos] - vis * m_srcBpp;
                    u8* dd = base + clip->right * m_dstBpp;
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(dd, ss, vis, pitch);
                        }
                    } else {
                        ConvertRowFlip(dd, ss, vis);
                    }
                }
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            } else {
                u8 b = m_rleData[pos];
                if (b & 0x80) {
                    x += 0x80 - static_cast<i32>(b);
                    pos++;
                } else {
                    u8* dd = base + x * m_dstBpp;
                    u8* ss = &m_rleData[pos + 1];
                    i32 cnt = b;
                    if (m_doubleScanlines) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(dd, ss, cnt, pitch);
                        }
                    } else {
                        ConvertRowFlip(dd, ss, cnt);
                    }
                    x -= cnt;
                    pos += cnt * m_srcBpp + 1;
                }
            }
        }
    }

    surf->m_ddSurface->Unlock(0);
}

// @early-stop
// retail strength-reduces the DST_BY_SRC_16 arm to ONE cursor with the scratch
// pointer biased off it; cl keeps two and spills the trip count instead.
// 128 AST variants (commutative/hoist/merge/inline) moved none of it.
RVA(0x0014c9f0, 0x5d0)
void CDDrawShadeBlit::ConvertRow(u8* dst, u8* src, i32 count) {
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            u8* pal = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            while (count-- > 0) {
                *dst++ = pal[(*sc++ << 8) + *src++];
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst, count * 2);
            u16* sc = Scratch16();
            u16* sw = Pix16(dst);
            while (count-- > 0) {
                u32 idx = pal2[*sc++];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                *sw++ = pal1[idx];
            }
            break;
        }
        case SHADE_PAL_16: {
            u16* pal = m_palDescr->Lut16();
            u16* sw = Pix16(dst);
            while (count-- > 0) {
                *sw++ = pal[*src++];
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u16* sd = Scratch16();
                u16* ss = Pix16(src);
                u16* sw = Pix16(dst);
                while (count-- > 0) {
                    u32 a = *ss++;
                    u32 b = *sd++;
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((b >> 5) & ~0x1f)];
                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    *sw++ = r;
                }
            } else {
                u16* sd = Scratch16();
                u16* ss = Pix16(src);
                u16* sw = Pix16(dst);
                while (count-- > 0) {
                    u32 a = *ss++;
                    u32 b = *sd++;
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((b >> 6) & ~0x1f)];
                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    *sw++ = r;
                }
            }
            break;
        }
        case SHADE_PAL_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            u16* pal = m_palDescr->Lut16();
            if (m_blendVariant) {
                u16* sd = Scratch16();
                u16* sw = Pix16(dst);
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((b >> 5) & ~0x1f)];
                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    *sw++ = r;
                }
            } else {
                u16* sd = Scratch16();
                u16* sw = Pix16(dst);
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((b >> 6) & ~0x1f)];
                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    *sw++ = r;
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
// as ConvertRow: retail runs the 16bpp arms off a single biased cursor.
RVA(0x0014cfc0, 0x620)
void CDDrawShadeBlit::ConvertRowFlip(u8* dst, u8* src, i32 count) {
    u8* base = m_palDescr ? m_palDescr->m_data : src;
    switch (m_drawType) {
        case SHADE_DST_BY_SRC: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            while (count-- > 0) {
                *dst-- = base[(*sc-- << 8) + *src++];
            }
            break;
        }
        case SHADE_DST_BY_SRC_16: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            u16* sw = Pix16(dst);
            while (count-- > 0) {
                u32 idx = pal2[*sc--];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                *sw-- = pal1[idx];
            }
            break;
        }
        case SHADE_PAL_16: {
            u16* pal = m_palDescr->Lut16();
            u16* sw = Pix16(dst);
            while (count-- > 0) {
                *sw-- = pal[*src++];
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            u16* ss = Pix16(src);
            u16* sw = Pix16(dst);
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u16 r = m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *sw-- = r;
                }
            } else {
                while (count-- > 0) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u16 r = m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *sw-- = r;
                }
            }
            break;
        }
        case SHADE_PAL_ALPHA_16: {
            u16* pal = m_palDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            u16* sw = Pix16(dst);
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u16 r = m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *sw-- = r;
                }
            } else {
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u16 r = m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)];
                    r |= m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *sw-- = r;
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
            while (count-- > 0) {
                *dst-- = base[(*src++ << 8) + m_light];
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
            while (count-- > 0) {
                i32 s = base[*sc-- + 0x100];
                i32 d = base[*src + 0x100];
                i32 t = (d - s) * m_light / 255 + s;
                *dst-- = base[t];
                src++;
            }
            break;
        }
    }
}

// @early-stop
// retail has NO frame at all here - one register induction variable plus three
// biases parked in the dead parameter home slots. cl keeps three cursors live
// and needs two spill slots on top, so every stack offset shifts.
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
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst, count * 2);
            u16* sc = Scratch16();
            while (count-- > 0) {
                u32 idx = pal2[*sc++];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                u16 v = pal1[idx];
                Store16(dst, v);
                Store16(dst + rowDelta / 2 * 2, v);
                dst += 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            u16* sc = Scratch16();
            u16* ss = Pix16(src);
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    i32 v = m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)]
                            | m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
                    dst += 2;
                }
            } else {
                while (count-- > 0) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    i32 v = m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)]
                            | m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
                    dst += 2;
                }
            }
            break;
        }
    }
}

// @early-stop
// mirror of ConvertRowDoubleFwd: retail's frame is 0, ours is 0x8.
// the SHADE_DST_BY_LEVEL asymmetry (m_light for dst[0], *src for dst[rowDelta],
// src never advanced) is retail's, confirmed at 0x14d9f5 - do not "fix" it.
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
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            while (count-- > 0) {
                u32 idx = pal2[*sc--];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                u16 v = pal1[idx];
                Store16(dst, v);
                Store16(dst + rowDelta / 2 * 2, v);
                dst -= 2;
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            u16* ss = Pix16(src);
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    i32 v = m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)]
                            | m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
                    dst -= 2;
                }
            } else {
                while (count-- > 0) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    i32 v = m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)]
                            | m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
                    dst -= 2;
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
            g_shadeDescr208 = v;
            break;
        case SHADE_DST_BY_LEVEL:
            g_shadeDescr20c = v;
            break;
        case SHADE_SRC_BY_LEVEL:
            g_shadeDescr210 = v;
            break;
        case SHADE_LERP_LEVEL:
            g_shadeDescr214 = v;
            break;
        case SHADE_DST_BY_SRC_16:
            g_shadeDescr21c = v;
            break;
        case SHADE_PAL_16:
            g_shadeDescr220 = v;
            break;
        case SHADE_PAL_ALPHA_16:
            g_shadeDescr220 = v;
            break;
        case SHADE_GREY_TABLE:
            g_blendDescr = v;
            break;
    }
}

RVA(0x0014dd90, 0xa0)
void CDDrawShadeBlit::Select(ShadeMode mode, CShadeTable* descr) {
    m_drawType = mode;
    if (descr == NULL) {
        switch (mode) {
            case SHADE_DST_BY_SRC:
                m_palDescr = g_shadeDescr208;
                break;
            case SHADE_DST_BY_LEVEL:
                m_palDescr = g_shadeDescr20c;
                break;
            case SHADE_SRC_BY_LEVEL:
                m_palDescr = g_shadeDescr210;
                break;
            case SHADE_LERP_LEVEL:
                m_palDescr = g_shadeDescr214;
                break;
            case SHADE_DST_BY_SRC_16:
                m_palDescr = g_shadeDescr21c;
                break;
            case SHADE_PAL_16:
                m_palDescr = g_shadeDescr220;
                break;
            case SHADE_PAL_ALPHA_16:
                m_palDescr = g_shadeDescr220;
                break;
        }
    } else {
        m_palDescr = descr;
    }
}
