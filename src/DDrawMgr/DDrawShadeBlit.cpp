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
    if (static_cast<u8>(mode) == 2) {
        if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
            m_blendVariant = 1;
        } else {
            m_blendVariant = 0;
        }
    }

    i32 drawType = m_drawType;
    if (drawType == 1) {
        if (sel) {
            BlitCopyMirrored(dst, src, clip, vflip);
        } else {
            BlitCopyForward(dst, src, clip, vflip);
        }
        return 1;
    }

    if (drawType == 7) {
        if (m_srcBpp != 1 || static_cast<u8>(mode) != 2) {
            return 0;
        }
    }
    if (drawType == 0xa || drawType == 0xb) {
        if (m_srcBpp != 1 || static_cast<u8>(mode) != 2) {
            return 0;
        }
    }
    if (drawType == 8 || drawType == 0xb) {
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
RVA(0x00149d00, 0x4f8)
void CDDrawShadeBlit::BlitCopyMirrored(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = static_cast<u8*>(surf->Lock(0));

    i32 row = 0, pos = 0, x = 0;
    if (clip->top != 0) {
        do {
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += static_cast<i32>(b) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
    }
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        pitch = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
    }

    x = m_width;
    if (clip->left == 0 && clip->right == m_width - 1) {

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
                u8* s = &m_rleData[pos + 1];
                if (m_srcBpp == 1) {
                    u8* d = base + x * m_dstBpp;
                    for (i32 k = cnt; k > 0; k--) {
                        *d-- = *s++;
                    }
                } else {
                    u16* d = Pix16(base + x * m_dstBpp);
                    u16* sw = Pix16(s);
                    for (i32 k = cnt; k > 0; k--) {
                        *d-- = *sw++;
                    }
                }
                x -= cnt;
                pos += cnt * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else if (clip->left != 0) {

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
                u8* sd = &m_rleData[pos + 1];
                i32 bytes;
                if (x - cnt > clip->left) {
                    bytes = cnt * m_srcBpp;
                } else {
                    i32 vis = (x - clip->left) * m_srcBpp;
                    bytes = vis < 0 ? 0 : vis;
                }
                u8* dbase = base + (x - clip->left) * m_dstBpp;
                if (m_srcBpp == 1) {
                    u8* d = dbase;
                    for (i32 k = bytes; k > 0; k--) {
                        *d-- = *sd++;
                    }
                } else {
                    u16* d = Pix16(dbase);
                    u16* sw = Pix16(sd);
                    for (i32 k = bytes / 2; k > 0; k--) {
                        *d-- = *sw++;
                    }
                }
                x -= cnt;
                pos += cnt * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else {

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
                    i32 bytes = (clip->right - x) * m_srcBpp;
                    u8* s = &m_rleData[pos] - bytes;
                    if (m_srcBpp == 1) {
                        u8* d = base + clip->right * m_dstBpp;
                        for (i32 k = bytes; k > 0; k--) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = Pix16(base + clip->right * m_dstBpp);
                        u16* sw = Pix16(s);
                        for (i32 k = bytes / 2; k > 0; k--) {
                            *d-- = *sw++;
                        }
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
                    i32 cnt = b;
                    u8* s = &m_rleData[pos + 1];
                    if (m_srcBpp == 1) {
                        u8* d = base + x * m_dstBpp;
                        for (i32 k = cnt; k > 0; k--) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = Pix16(base + x * m_dstBpp);
                        u16* sw = Pix16(s);
                        for (i32 k = cnt; k > 0; k--) {
                            *d-- = *sw++;
                        }
                    }
                    x -= cnt;
                    pos += cnt * m_srcBpp + 1;
                }
            }
        }
    }

    surf->m_ddSurface->Unlock(0);
}

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
        do {
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += static_cast<i32>(b) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
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
                            case 2: {
                                u8* pal = m_palDescr->m_data;
                                memcpy(g_scratch, d, count);
                                u8* sc = g_scratch;
                                for (i = count; i > 0; i--) {
                                    d[0] = pal[(*sc << 8) + *s];
                                    d[pitch] = pal[(*sc << 8) + *s];
                                    d++;
                                    sc++;
                                    s++;
                                }
                                break;
                            }
                            case 3: {
                                u8* pal = m_palDescr->m_data;
                                memcpy(g_scratch, d, count);
                                u8* sc = g_scratch;
                                for (i = count; i > 0; i--) {
                                    d[0] = pal[(*sc << 8) + m_light];
                                    d[pitch] = pal[(*sc << 8) + m_light];
                                    d++;
                                    sc++;
                                }
                                break;
                            }
                            case 7: {
                                u16* pal1 = m_palDescr->Lut16();
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d, count * 2);
                                u16* sc = Scratch16();
                                i32 rd = pitch & ~1;
                                for (i = count; i > 0; i--) {
                                    u32 idx = pal2[*sc++];
                                    idx += (*s++ >> 4) << 12;
                                    u16 v = pal1[idx];
                                    Store16(d, static_cast<u16>(v));
                                    Store16(d + rd, static_cast<u16>(v));
                                    d += 2;
                                }
                                break;
                            }
                            case 8: {
                                memcpy(g_scratch, d, count * 2);
                                u16* sc = Scratch16();
                                u16* ss2 = Pix16(s);
                                i32 rd = pitch & ~1;
                                if (m_blendVariant) {
                                    for (i = count; i > 0; i--) {
                                        u32 dv = *sc++;
                                        u32 a = *ss2++;
                                        u32 r = (m_lutBank0)[(a >> 0xa) + ((dv >> 5) & 0xffe0)];
                                        r |= (m_lutBank1)
                                            [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                        r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                        Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
                                        d += 2;
                                    }
                                } else {
                                    for (i = count; i > 0; i--) {
                                        u32 dv = *sc++;
                                        u32 a = *ss2++;
                                        u32 r = (m_lutBank0)[(a >> 0xb) + ((dv >> 6) & 0xffe0)];
                                        r |= (m_lutBank1)
                                            [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                        r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                        Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
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
                        case 2: {
                            u8* pal = m_palDescr->m_data;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            for (i = count; i > 0; i--) {
                                *d++ = pal[(*sc++ << 8) + *s++];
                            }
                            break;
                        }
                        case 7: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_blendDescr->Lut16();
                            u16* sc = Scratch16();
                            memcpy(g_scratch, d, count * 2);
                            for (i = count; i > 0; i--) {
                                u32 idx = pal2[*sc++];
                                idx += (*s++ >> 4) << 12;
                                Store16(d, static_cast<u16>(pal1[idx]));
                                d += 2;
                            }
                            break;
                        }
                        case 10: {
                            u16* pal = m_palDescr->Lut16();
                            for (i = count; i > 0; i--) {
                                Store16(d, static_cast<u16>(pal[*s++]));
                                d += 2;
                            }
                            break;
                        }
                        case 8: {
                            memcpy(g_scratch, d, count * 2);
                            if (m_blendVariant) {
                                u16* sd = Scratch16();
                                u16* ss2 = Pix16(s);
                                for (i = count; i > 0; i--) {
                                    u32 a = *ss2++;
                                    u32 bb = *sd++;
                                    u32 r = (m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |=
                                        (m_lutBank1)[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    r |= (m_lutBank0)[(a >> 0xa) + (bb & 0xffe0)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d += 2;
                                }
                            } else {
                                u16* sd = Scratch16();
                                u16* ss2 = Pix16(s);
                                for (i = count; i > 0; i--) {
                                    u32 a = *sd++;
                                    u32 bb = *ss2++;
                                    u32 r =
                                        (m_lutBank0)[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= (m_lutBank1)[((a >> 0xb)) + (bb & 0xffe0)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d += 2;
                                }
                            }
                            break;
                        }
                        case 11: {
                            u16* pal = m_palDescr->Lut16();
                            memcpy(g_scratch, d, count * 2);
                            if (m_blendVariant) {
                                u16* sd = Scratch16();
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 bb = *sd++;
                                    u32 r =
                                        (m_lutBank1)[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    r |= (m_lutBank0)[(a >> 0xa) + (bb & 0xffe0)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d += 2;
                                }
                            } else {
                                u16* sd = Scratch16();
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 bb = *sd++;
                                    u32 r =
                                        (m_lutBank0)[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= (m_lutBank1)[(a >> 0xb) + (bb & 0xffe0)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d += 2;
                                }
                            }
                            break;
                        }
                        case 3: {
                            u8* pbase = m_palDescr->m_data;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            for (i = count; i > 0; i--) {
                                *d++ = pbase[(*sc++ << 8) + m_light];
                            }
                            break;
                        }
                        case 4: {
                            u8* pbase = m_palDescr->m_data;
                            for (i = count; i > 0; i--) {
                                *d++ = pbase[(*s++ << 8) + m_light];
                            }
                            break;
                        }
                        case 5: {
                            for (i = count; i > 0; i--) {
                                *d++ = static_cast<u8>(m_light);
                            }
                            break;
                        }
                        case 6: {
                            u8* pal = m_palDescr->m_data;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            for (i = count; i > 0; i--) {
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

                            i32 i;
                            u8* d = dd;
                            u8* s = ss;
                            switch (m_drawType) {
                                case 2: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, vis);
                                    u8* sc = g_scratch;
                                    for (i = vis; i > 0; i--) {
                                        d[0] = pal[(*sc << 8) + *s];
                                        d[pitch] = pal[(*sc << 8) + *s];
                                        d++;
                                        sc++;
                                        s++;
                                    }
                                    break;
                                }
                                case 3: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, vis);
                                    u8* sc = g_scratch;
                                    for (i = vis; i > 0; i--) {
                                        d[0] = pal[(*sc << 8) + m_light];
                                        d[pitch] = pal[(*sc << 8) + m_light];
                                        d++;
                                        sc++;
                                    }
                                    break;
                                }
                                case 7: {
                                    u16* pal1 = m_palDescr->Lut16();
                                    u16* pal2 = g_blendDescr->Lut16();
                                    memcpy(g_scratch, d, vis * 2);
                                    u16* sc = Scratch16();
                                    i32 rd = pitch & ~1;
                                    for (i = vis; i > 0; i--) {
                                        u32 idx = pal2[*sc++];
                                        idx += (*s++ >> 4) << 12;
                                        u16 v = pal1[idx];
                                        Store16(d, static_cast<u16>(v));
                                        Store16(d + rd, static_cast<u16>(v));
                                        d += 2;
                                    }
                                    break;
                                }
                                case 8: {
                                    memcpy(g_scratch, d, vis * 2);
                                    u16* sc = Scratch16();
                                    u16* ss2 = Pix16(s);
                                    i32 rd = pitch & ~1;
                                    if (m_blendVariant) {
                                        for (i = vis; i > 0; i--) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            u32 r = (m_lutBank0)[(a >> 0xa) + ((dv >> 5) & 0xffe0)];
                                            r |= (m_lutBank1)
                                                [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                            r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                            Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
                                            d += 2;
                                        }
                                    } else {
                                        for (i = vis; i > 0; i--) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            u32 r = (m_lutBank0)[(a >> 0xb) + ((dv >> 6) & 0xffe0)];
                                            r |= (m_lutBank1)
                                                [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                            r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                            Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
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

                            i32 i;
                            u8* d = dd;
                            u8* s = ss;
                            switch (m_drawType) {
                                case 2: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, count);
                                    u8* sc = g_scratch;
                                    for (i = count; i > 0; i--) {
                                        d[0] = pal[(*sc << 8) + *s];
                                        d[pitch] = pal[(*sc << 8) + *s];
                                        d++;
                                        sc++;
                                        s++;
                                    }
                                    break;
                                }
                                case 3: {
                                    u8* pal = m_palDescr->m_data;
                                    memcpy(g_scratch, d, count);
                                    u8* sc = g_scratch;
                                    for (i = count; i > 0; i--) {
                                        d[0] = pal[(*sc << 8) + m_light];
                                        d[pitch] = pal[(*sc << 8) + m_light];
                                        d++;
                                        sc++;
                                    }
                                    break;
                                }
                                case 7: {
                                    u16* pal1 = m_palDescr->Lut16();
                                    u16* pal2 = g_blendDescr->Lut16();
                                    memcpy(g_scratch, d, count * 2);
                                    u16* sc = Scratch16();
                                    i32 rd = pitch & ~1;
                                    for (i = count; i > 0; i--) {
                                        u32 idx = pal2[*sc++];
                                        idx += (*s++ >> 4) << 12;
                                        u16 v = pal1[idx];
                                        Store16(d, static_cast<u16>(v));
                                        Store16(d + rd, static_cast<u16>(v));
                                        d += 2;
                                    }
                                    break;
                                }
                                case 8: {
                                    memcpy(g_scratch, d, count * 2);
                                    u16* sc = Scratch16();
                                    u16* ss2 = Pix16(s);
                                    i32 rd = pitch & ~1;
                                    if (m_blendVariant) {
                                        for (i = count; i > 0; i--) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            u32 r = (m_lutBank0)[(a >> 0xa) + ((dv >> 5) & 0xffe0)];
                                            r |= (m_lutBank1)
                                                [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                            r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                            Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
                                            d += 2;
                                        }
                                    } else {
                                        for (i = count; i > 0; i--) {
                                            u32 dv = *sc++;
                                            u32 a = *ss2++;
                                            u32 r = (m_lutBank0)[(a >> 0xb) + ((dv >> 6) & 0xffe0)];
                                            r |= (m_lutBank1)
                                                [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                            r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                            Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                            Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
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
        do {
            u8 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += static_cast<i32>(b) * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
    }

    i32 rowInc;
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        rowInc = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
        rowInc = pitch;
    }

    x = m_width;
    if (clip->left == 0 && clip->right == m_width - 1) {

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
                            case 2: {
                                u8* pbase = m_palDescr->m_data;
                                memcpy(g_scratch, d - count + 1, count);
                                u8* sc = &g_scratch[count - 1];
                                for (i = count; i > 0; i--) {
                                    d[0] = pbase[(*sc << 8) + *s];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                    s++;
                                }
                                break;
                            }
                            case 3: {
                                u8* pbase = m_palDescr->m_data;
                                memcpy(g_scratch, d - count + 1, count);
                                u8* sc = &g_scratch[count - 1];
                                for (i = count; i > 0; i--) {
                                    d[0] = pbase[(*sc << 8) + m_light];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                }
                                break;
                            }
                            case 7: {
                                u16* pal1 = m_palDescr->Lut16();
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u16* sc = (Scratch16() + count - 1);
                                i32 rd = pitch & ~1;
                                for (i = count; i > 0; i--) {
                                    u32 idx = pal2[*sc--];
                                    idx += (*s++ >> 4) << 12;
                                    u16 v = pal1[idx];
                                    Store16(d, static_cast<u16>(v));
                                    Store16(d + rd, static_cast<u16>(v));
                                    d -= 2;
                                }
                                break;
                            }
                            case 8: {
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u16* sc = (Scratch16() + count - 1);
                                u16* ss2 = Pix16(s);
                                i32 rd = pitch & ~1;
                                if (m_blendVariant) {
                                    for (i = count; i > 0; i--) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        u32 r =
                                            (m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                        r |= (m_lutBank1)
                                            [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                        r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                        Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
                                        d -= 2;
                                    }
                                } else {
                                    for (i = count; i > 0; i--) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        u32 r =
                                            (m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                        r |= (m_lutBank1)
                                            [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                        r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                        Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
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
                        case 2: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            for (i = count; i > 0; i--) {
                                *d-- = cbase[(*sc-- << 8) + *s++];
                            }
                            break;
                        }
                        case 7: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_blendDescr->Lut16();
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (Scratch16() + count - 1);
                            for (i = count; i > 0; i--) {
                                u32 idx = pal2[*sc--];
                                idx += (*s++ >> 4) << 12;
                                Store16(d, static_cast<u16>(pal1[idx]));
                                d -= 2;
                            }
                            break;
                        }
                        case 10: {
                            u16* pal = m_palDescr->Lut16();
                            for (i = count; i > 0; i--) {
                                Store16(d, static_cast<u16>(pal[*s++]));
                                d -= 2;
                            }
                            break;
                        }
                        case 8: {
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (Scratch16() + count - 1);
                            u16* ss2 = Pix16(s);
                            if (m_blendVariant) {
                                for (i = count; i > 0; i--) {
                                    u32 a = *ss2++;
                                    u32 dv = *sc--;
                                    u32 r =
                                        (m_lutBank1)[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= (m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d -= 2;
                                }
                            } else {
                                for (i = count; i > 0; i--) {
                                    u32 a = *ss2++;
                                    u32 dv = *sc--;
                                    u32 r = (m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |=
                                        (m_lutBank1)[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d -= 2;
                                }
                            }
                            break;
                        }
                        case 11: {
                            u16* pal = m_palDescr->Lut16();
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (Scratch16() + count - 1);
                            if (m_blendVariant) {
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 dv = *sc--;
                                    u32 r =
                                        (m_lutBank1)[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= (m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d -= 2;
                                }
                            } else {
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 dv = *sc--;
                                    u32 r =
                                        (m_lutBank1)[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= (m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                    Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                    d -= 2;
                                }
                            }
                            break;
                        }
                        case 3: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            for (i = count; i > 0; i--) {
                                *d-- = cbase[(*sc-- << 8) + m_light];
                            }
                            break;
                        }
                        case 4: {
                            for (i = count; i > 0; i--) {
                                *d-- = cbase[(*s++ << 8) + m_light];
                            }
                            break;
                        }
                        case 5: {
                            for (i = count; i > 0; i--) {
                                *d-- = static_cast<u8>(m_light);
                            }
                            break;
                        }
                        case 6: {
                            memcpy(g_scratch, d - count - 1, count);
                            u8* sc = &g_scratch[count + 1];
                            for (i = count; i > 0; i--) {
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
                base += rowInc;
                x = m_width;
            }
        }
    } else if (clip->left != 0) {

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
                i32 vis;
                if (x - cnt > clip->left) {
                    vis = cnt;
                } else {
                    i32 v = x - clip->left;
                    vis = v < 0 ? 0 : v;
                }
                u8* dd = base + (x - clip->left) * m_dstBpp;
                if (m_doubleScanlines) {
                    if ((dst->top + row) % 2) {

                        i32 i;
                        u8* d = dd;
                        u8* s = ss;
                        switch (m_drawType) {
                            case 2: {
                                u8* pbase = m_palDescr->m_data;
                                memcpy(g_scratch, d - vis + 1, vis);
                                u8* sc = &g_scratch[vis - 1];
                                for (i = vis; i > 0; i--) {
                                    d[0] = pbase[(*sc << 8) + *s];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                    s++;
                                }
                                break;
                            }
                            case 3: {
                                u8* pbase = m_palDescr->m_data;
                                memcpy(g_scratch, d - vis + 1, vis);
                                u8* sc = &g_scratch[vis - 1];
                                for (i = vis; i > 0; i--) {
                                    d[0] = pbase[(*sc << 8) + m_light];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                }
                                break;
                            }
                            case 7: {
                                u16* pal1 = m_palDescr->Lut16();
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d - vis * 2 - 2, vis * 2);
                                u16* sc = (Scratch16() + vis - 1);
                                i32 rd = pitch & ~1;
                                for (i = vis; i > 0; i--) {
                                    u32 idx = pal2[*sc--];
                                    idx += (*s++ >> 4) << 12;
                                    u16 v = pal1[idx];
                                    Store16(d, static_cast<u16>(v));
                                    Store16(d + rd, static_cast<u16>(v));
                                    d -= 2;
                                }
                                break;
                            }
                            case 8: {
                                memcpy(g_scratch, d - vis * 2 - 2, vis * 2);
                                u16* sc = (Scratch16() + vis - 1);
                                u16* ss2 = Pix16(s);
                                i32 rd = pitch & ~1;
                                if (m_blendVariant) {
                                    for (i = vis; i > 0; i--) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        u32 r =
                                            (m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                        r |= (m_lutBank1)
                                            [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                        r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                        Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
                                        d -= 2;
                                    }
                                } else {
                                    for (i = vis; i > 0; i--) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        u32 r =
                                            (m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                        r |= (m_lutBank1)
                                            [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                        r |= (m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(d, static_cast<u16>(static_cast<u16>(r)));
                                        Store16(d + rd, static_cast<u16>(static_cast<u16>(r)));
                                        d -= 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {
                    ConvertRowFlip(dd, ss, vis);
                }
                x -= cnt;
                pos += cnt * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += rowInc;
                x = m_width;
            }
        }
    } else {

        while (row <= clip->bottom) {
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
                base += rowInc;
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

RVA(0x0014c9f0, 0x5d0)
void CDDrawShadeBlit::ConvertRow(u8* dst, u8* src, i32 count) {
    i32 i;
    switch (m_drawType) {
        case 2: {
            u8* pal = m_palDescr->m_data;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                *dst++ = pal[(*sc++ << 8) + *src++];
            }
            break;
        }
        case 7: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_blendDescr->Lut16();
            u16* sc = Scratch16();
            memcpy(g_scratch, dst, count * 2);
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc++];
                idx += (*src++ >> 4) << 12;
                Store16(dst, static_cast<u16>(pal1[idx]));
                dst += 2;
            }
            break;
        }
        case 10: {
            u16* pal = m_palDescr->Lut16();
            for (i = count; i > 0; i--) {
                Store16(dst, static_cast<u16>(pal[*src++]));
                dst += 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u16* sd = Scratch16();
                u16* ss = Pix16(src);
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 b = *sd++;
                    u32 r = (m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= (m_lutBank1)[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    r |= (m_lutBank0)[(a >> 0xa) + (b & 0xffe0)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst += 2;
                }
            } else {
                u16* sd = Scratch16();
                u16* ss = Pix16(src);
                for (i = count; i > 0; i--) {
                    u32 a = *sd++;
                    u32 b = *ss++;
                    u32 r = (m_lutBank0)[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    r |= (m_lutBank1)[((a >> 0xb)) + (b & 0xffe0)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst += 2;
                }
            }
            break;
        }
        case 11: {
            u16* pal = m_palDescr->Lut16();
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u16* sd = Scratch16();
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u32 r = (m_lutBank1)[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    r |= (m_lutBank0)[(a >> 0xa) + (b & 0xffe0)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst += 2;
                }
            } else {
                u16* sd = Scratch16();
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u32 r = (m_lutBank0)[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    r |= (m_lutBank1)[(a >> 0xb) + (b & 0xffe0)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst += 2;
                }
            }
            break;
        }
        case 3: {
            u8* base = m_palDescr->m_data;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                *dst++ = base[(*sc++ << 8) + m_light];
            }
            break;
        }
        case 4: {
            u8* base = m_palDescr->m_data;
            for (i = count; i > 0; i--) {
                *dst++ = base[(*src++ << 8) + m_light];
            }
            break;
        }
        case 5: {
            for (i = count; i > 0; i--) {
                *dst++ = static_cast<u8>(m_light);
            }
            break;
        }
        case 6: {
            u8* pal = m_palDescr->m_data;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
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

RVA(0x0014cfc0, 0x620)
void CDDrawShadeBlit::ConvertRowFlip(u8* dst, u8* src, i32 count) {
    u8* base = m_palDescr ? m_palDescr->m_data : src;
    i32 i;
    switch (m_drawType) {
        case 2: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                *dst-- = base[(*sc-- << 8) + *src++];
            }
            break;
        }
        case 7: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc--];
                idx += (*src++ >> 4) << 12;
                Store16(dst, static_cast<u16>(pal1[idx]));
                dst -= 2;
            }
            break;
        }
        case 10: {
            u16* pal = m_palDescr->Lut16();
            for (i = count; i > 0; i--) {
                Store16(dst, static_cast<u16>(pal[*src++]));
                dst -= 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            u16* ss = Pix16(src);
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = (m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= (m_lutBank0)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = (m_lutBank0)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= (m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst -= 2;
                }
            }
            break;
        }
        case 11: {
            u16* pal = m_palDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u32 r = (m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= (m_lutBank0)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u32 r = (m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= (m_lutBank0)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    dst -= 2;
                }
            }
            break;
        }
        case 3: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                *dst-- = base[(*sc-- << 8) + m_light];
            }
            break;
        }
        case 4: {
            for (i = count; i > 0; i--) {
                *dst-- = base[(*src++ << 8) + m_light];
            }
            break;
        }
        case 5: {
            for (i = count; i > 0; i--) {
                *dst-- = static_cast<u8>(m_light);
            }
            break;
        }
        case 6: {
            memcpy(g_scratch, dst - count - 1, count);
            u8* sc = &g_scratch[count + 1];
            for (i = count; i > 0; i--) {
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

RVA(0x0014d5e0, 0x370)
void CDDrawShadeBlit::ConvertRowDoubleFwd(u8* dst, u8* src, i32 count, i32 rowDelta) {
    i32 i;
    switch (m_drawType) {
        case 2: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + *src];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst++;
                sc++;
                src++;
            }
            break;
        }
        case 3: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + m_light];
                dst[rowDelta] = base[(*sc << 8) + m_light];
                dst++;
                sc++;
            }
            break;
        }
        case 7: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst, count * 2);
            u16* sc = Scratch16();
            i32 rd = rowDelta & ~1;
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc++];
                idx += (*src++ >> 4) << 12;
                u16 v = pal1[idx];
                Store16(dst, static_cast<u16>(v));
                Store16(dst + rd, static_cast<u16>(v));
                dst += 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst, count * 2);
            u16* sc = Scratch16();
            u16* ss = Pix16(src);
            i32 rd = rowDelta & ~1;
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    u32 r = (m_lutBank0)[(a >> 0xa) + ((d >> 5) & 0xffe0)];
                    r |= (m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    Store16(dst + rd, static_cast<u16>(static_cast<u16>(r)));
                    dst += 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    u32 r = (m_lutBank0)[(a >> 0xb) + ((d >> 6) & 0xffe0)];
                    r |= (m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    Store16(dst + rd, static_cast<u16>(static_cast<u16>(r)));
                    dst += 2;
                }
            }
            break;
        }
    }
}

RVA(0x0014d950, 0x3a0)
void CDDrawShadeBlit::ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta) {
    i32 i;
    switch (m_drawType) {
        case 2: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + *src];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
                src++;
            }
            break;
        }
        case 3: {
            u8* base = m_palDescr->m_data;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + m_light];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
            }
            break;
        }
        case 7: {
            u16* pal1 = m_palDescr->Lut16();
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            i32 rd = rowDelta & ~1;
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc--];
                idx += (*src++ >> 4) << 12;
                u16 v = pal1[idx];
                Store16(dst, static_cast<u16>(v));
                Store16(dst + rd, static_cast<u16>(v));
                dst -= 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (Scratch16() + count - 1);
            u16* ss = Pix16(src);
            i32 rd = rowDelta & ~1;
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = (m_lutBank0)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    r |= (m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    Store16(dst + rd, static_cast<u16>(static_cast<u16>(r)));
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = (m_lutBank0)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    r |= (m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= (m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, static_cast<u16>(static_cast<u16>(r)));
                    Store16(dst + rd, static_cast<u16>(static_cast<u16>(r)));
                    dst -= 2;
                }
            }
            break;
        }
    }
}

RVA(0x0014dcf0, 0xa0)
void SetShadeDescr(CShadeTable* v, int mode) {
    switch (mode) {
        case 2:
            g_shadeDescr208 = v;
            break;
        case 3:
            g_shadeDescr20c = v;
            break;
        case 4:
            g_shadeDescr210 = v;
            break;
        case 6:
            g_shadeDescr214 = v;
            break;
        case 7:
            g_shadeDescr21c = v;
            break;
        case 10:
            g_shadeDescr220 = v;
            break;
        case 11:
            g_shadeDescr220 = v;
            break;
        case 9:
            g_blendDescr = v;
            break;
    }
}

RVA(0x0014dd90, 0xa0)
void CDDrawShadeBlit::Select(i32 mode, CShadeTable* descr) {
    m_drawType = mode;
    if (descr == 0) {
        switch (mode) {
            case 2:
                m_palDescr = g_shadeDescr208;
                break;
            case 3:
                m_palDescr = g_shadeDescr20c;
                break;
            case 4:
                m_palDescr = g_shadeDescr210;
                break;
            case 6:
                m_palDescr = g_shadeDescr214;
                break;
            case 7:
                m_palDescr = g_shadeDescr21c;
                break;
            case 10:
                m_palDescr = g_shadeDescr220;
                break;
            case 11:
                m_palDescr = g_shadeDescr220;
                break;
        }
    } else {
        m_palDescr = descr;
    }
}
