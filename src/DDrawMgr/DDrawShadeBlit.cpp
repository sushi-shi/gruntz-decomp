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
        m_lutBank0 = Pix16(g_clut + 0x20000 + bank);
        m_lutBank1 = Pix16(g_clut + bank);
        m_lutBank2 = Pix16(g_clut + 0x10000 + bank);
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
// 1-instruction reload plus the test where retail tests in one block. One branch is
// still unaccounted for (56 vs 55): retail threads the `x < 0` edge of the
// clip->right arm straight into the row-advance block.
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
// Block skeleton now matches retail (197 blocks both sides). Four structural facts
// were recovered from the target: arm 2's row loop is `row < clip->bottom` (retail
// `jge`, not `jg`); arm 3 tests `x + run >= clip->right` with the CLAMPED path as
// the if-body (retail `jl` to the full path), which is also what lets cl tail-merge
// the two ConvertRow call sites into one; the destination/source/count expressions
// are recomputed inside each doubleScanlines and each non-doubleScanlines arm rather
// than hoisted above the mode test; and the per-arm loop invariants (rd, the scratch
// and source biases) are declared INSIDE the loop so LICM lands them in the preheader
// where retail has them, not in the pre-guard block.
// REGISTER-HOMING residue: inside the 16bpp/ALPHA arms cl strength-reduces `sc + db`
// into a SECOND induction variable where retail keeps one cursor and rebuilds the
// address from a reloaded bias (`lea eax,[edx+edi]`), and arm 1 memory-homes x/pos
// where retail keeps them in esi/ebp.
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
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d, count * 2);
                                i32 sc = g_scratch - d;
                                while (count-- > 0) {
                                    i32 rd = pitch / 2 * 2;
                                    u32 idx = pal2[Load16(d + sc)];
                                    d += 2;
                                    u32 hi = *s++;
                                    hi >>= 4;
                                    idx += hi << 12;
                                    u16 v = pal1[idx];
                                    Store16(d - 2, v);
                                    Store16(d - 2 + rd, v);
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
                                        Store16(sc + db, v);
                                        Store16(sc + db + rd, v);
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
                                        Store16(sc + db, v);
                                        Store16(sc + db + rd, v);
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
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count * 2);
                            while (count-- > 0) {
                                u32 idx = pal2[Load16(sc)];
                                u32 hi = *s++;
                                hi >>= 4;
                                idx += hi << 12;
                                Store16(d, static_cast<u16>(pal1[idx]));
                                d += 2;
                                sc += 2;
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
                                u8* ss2 = s;
                                while (count-- > 0) {
                                    u32 a = Load16(ss2);
                                    u32 bb = Load16(sd);
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((bb >> 5) & ~0x1f)];
                                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    Store16(d, r);
                                    d += 2;
                                    sd += 2;
                                    ss2 += 2;
                                }
                            } else {
                                u8* sd = g_scratch;
                                u8* ss2 = s;
                                while (count-- > 0) {
                                    u32 a = Load16(ss2);
                                    u32 bb = Load16(sd);
                                    u16 r = m_lutBank2[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((bb >> 6) & ~0x1f)];
                                    Store16(d, r);
                                    d += 2;
                                    sd += 2;
                                    ss2 += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_PAL_ALPHA_16: {
                            u16* pal = m_palDescr->Lut16();
                            memcpy(g_scratch, d, count * 2);
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
                                    u16* pal2 = g_blendDescr->Lut16();
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
                                            d += 2;
                                            sc += 2;
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
                                            d += 2;
                                            sc += 2;
                                            ss2 += 2;
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
                                    u16* pal2 = g_blendDescr->Lut16();
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
                                    u8* ss2 = s;
                                    if (m_blendVariant) {
                                        while (count-- > 0) {
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
                                            d += 2;
                                            sc += 2;
                                            ss2 += 2;
                                        }
                                    } else {
                                        while (count-- > 0) {
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
                                            d += 2;
                                            sc += 2;
                                            ss2 += 2;
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
    } else {

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
                                u16* pal2 = g_blendDescr->Lut16();
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u8* sc = &g_scratch[count * 2 - 2];
                                while (count-- > 0) {
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
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u8* sc = &g_scratch[count * 2 - 2];
                                u8* ss2 = s;
                                i32 db = d - sc;
                                if (m_blendVariant) {
                                    while (count-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        u32 a = Load16(ss2);
                                        u32 dv = Load16(sc);
                                        i32 v = m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(sc + db, v);
                                        Store16(sc + db + rd, v);
                                        sc -= 2;
                                        ss2 += 2;
                                    }
                                } else {
                                    while (count-- > 0) {
                                        i32 rd = pitch / 2 * 2;
                                        u32 a = Load16(ss2);
                                        u32 dv = Load16(sc);
                                        i32 v = m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)]
                                                | m_lutBank1
                                                    [((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)]
                                                | m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        Store16(sc + db, v);
                                        Store16(sc + db + rd, v);
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
                                *d-- = cbase[(*sc-- << 8) + *s++];
                            }
                            break;
                        }
                        case SHADE_DST_BY_SRC_16: {
                            u16* pal1 = m_palDescr->Lut16();
                            u16* pal2 = g_blendDescr->Lut16();
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
                                    u16 r =
                                        m_lutBank1[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)];
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
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    Store16(d, r);
                                    d -= 2;
                                    sc -= 2;
                                    ss2 += 2;
                                }
                            }
                            break;
                        }
                        case SHADE_PAL_ALPHA_16: {
                            u16* pal = m_palDescr->Lut16();
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u8* sc = &g_scratch[count * 2 - 2];
                            i32 db = d - sc;
                            if (m_blendVariant) {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = Load16(sc);
                                    u16 r =
                                        m_lutBank1[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xa) + ((dv >> 5) & ~0x1f)];
                                    Store16(sc + db, r);
                                    sc -= 2;
                                }
                            } else {
                                while (count-- > 0) {
                                    u32 a = pal[*s++];
                                    u32 dv = Load16(sc);
                                    u16 r =
                                        m_lutBank1[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    r |= m_lutBank2[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= m_lutBank0[(a >> 0xb) + ((dv >> 6) & ~0x1f)];
                                    Store16(sc + db, r);
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
                                    u16* pal2 = g_blendDescr->Lut16();
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
                                            u32 a = Load16(ss2);
                                            u32 dv = Load16(sc);
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
                                            u32 a = Load16(ss2);
                                            u32 dv = Load16(sc);
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
// edi, cl does the opposite and pays a reload+dec+store per iteration; in the ALPHA
// and PAL_ALPHA arms cl strength-reduces the biased store address into its own
// induction variable so the surviving cursor is dst, where retail's surviving cursor
// is g_scratch and the dst/src biases live in the recycled parameter slots. The
// explicit one-cursor-plus-bias transcription is byte-neutral here (measured), so the
// natural multi-cursor spelling is kept.
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
            u16* pal2 = g_blendDescr->Lut16();
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
                u8* sd = g_scratch;
                u8* ss = src;
                u8* sw = dst;
                while (count-- > 0) {
                    u32 a = Load16(ss);
                    u32 b = Load16(sd);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((b >> 5) & ~0x1f)];
                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    Store16(sw, r);
                    sd += 2;
                    ss += 2;
                    sw += 2;
                }
            } else {
                u8* sd = g_scratch;
                u8* ss = src;
                u8* sw = dst;
                while (count-- > 0) {
                    u32 a = Load16(ss);
                    u32 b = Load16(sd);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((b >> 6) & ~0x1f)];
                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    Store16(sw, r);
                    sd += 2;
                    ss += 2;
                    sw += 2;
                }
            }
            break;
        }
        case SHADE_PAL_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            u16* pal = m_palDescr->Lut16();
            if (m_blendVariant) {
                u8* sd = g_scratch;
                u8* sw = dst;
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 b = Load16(sd);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xa) + ((b >> 5) & ~0x1f)];
                    r |= m_lutBank1[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    Store16(sw, r);
                    sd += 2;
                    sw += 2;
                }
            } else {
                u8* sd = g_scratch;
                u8* sw = dst;
                while (count-- > 0) {
                    u32 a = pal[*src++];
                    u32 b = Load16(sd);
                    u16 r = m_lutBank2[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= m_lutBank0[(a >> 0xb) + ((b >> 6) & ~0x1f)];
                    r |= m_lutBank1[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    Store16(sw, r);
                    sd += 2;
                    sw += 2;
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
// As ConvertRow: retail runs the 16bpp arms off a single biased cursor. The one
// structural row left is the LERP arm, where base needs an entry `jmp` and a
// 1-instruction loop header to reload the palette base that retail keeps pinned in
// ebp (retail memory-homes src and dst instead) - a spill-choice wall, not a shape.
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
            u8* sc = &g_scratch[count * 2 - 2];
            while (count-- > 0) {
                u32 idx = pal2[Load16(sc)];
                dst -= 2;
                sc -= 2;
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                Store16(dst + 2, pal1[idx]);
            }
            break;
        }
        case SHADE_PAL_16: {
            u16* pal = m_palDescr->Lut16();
            u8* sw = dst;
            while (count-- > 0) {
                Store16(sw, pal[*src++]);
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
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
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
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
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
            u16* pal = m_palDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u8* sc = &g_scratch[count * 2 - 2];
            u8* sw = dst;
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 a = pal[*src++];
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
                    u32 a = pal[*src++];
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
// Block skeleton matches retail (26 blocks both sides). REGISTER-HOMING residue:
// per-arm cursor-anchor and widen-idiom coins inside the double-scanline arms.
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
            i32 sc = g_scratch - dst;
            while (count-- > 0) {
                u32 idx = pal2[Load16(dst + sc)];
                dst += 2;
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                u16 v = pal1[idx];
                Store16(dst - 2, v);
                Store16(dst - 2 + rowDelta / 2 * 2, v);
            }
            break;
        }
        case SHADE_ALPHA_16: {
            memcpy(g_scratch, dst, count * 2);
            u8* sc = g_scratch;
            u8* ss = src;
            if (m_blendVariant) {
                while (count-- > 0) {
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    i32 v = m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)]
                            | m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
                    dst += 2;
                    sc += 2;
                    ss += 2;
                }
            } else {
                while (count-- > 0) {
                    u32 d = Load16(sc);
                    u32 a = Load16(ss);
                    i32 v = m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)]
                            | m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
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
            u16* pal2 = g_blendDescr->Lut16();
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u8* sc = &g_scratch[count * 2 - 2];
            while (count-- > 0) {
                u32 idx = pal2[Load16(sc)];
                u32 hi = *src++;
                hi >>= 4;
                idx += hi << 12;
                u16 v = pal1[idx];
                Store16(dst, v);
                Store16(dst + rowDelta / 2 * 2, v);
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
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
                    i32 v = m_lutBank0[(a >> 0xa) + ((d >> 5) & ~0x1f)]
                            | m_lutBank1[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
                    dst -= 2;
                    sc -= 2;
                    ss += 2;
                }
            } else {
                while (count-- > 0) {
                    u32 a = Load16(ss);
                    u32 d = Load16(sc);
                    i32 v = m_lutBank0[(a >> 0xb) + ((d >> 6) & ~0x1f)]
                            | m_lutBank1[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)]
                            | m_lutBank2[(a & 0x1f) + ((d & 0x1f) << 5)];
                    Store16(dst, v);
                    Store16(dst + rowDelta / 2 * 2, v);
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
