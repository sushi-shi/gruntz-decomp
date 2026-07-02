// CFaderTileRender.cpp - the per-column tile gather/remap scanline compositor of
// the big CFader subtype (the 0x7d5c-byte fader allocated by CFaderMgr::Add case
// 2; its Apply path 0x1817e0 calls this). __thiscall, two args (arg0 = base
// pixel row, arg1 = a leading X offset). For each of m_64 columns it gathers a
// (2*m_58)-pixel source line into the scratch line m_488 - either straight
// (per-bpp 1/2/3) or through a 64-wide 2D LUT (m_1c->m_08, keyed by the gathered
// byte and the per-pixel selector m_48c) when m_5c is set - then optionally
// copies/zeros a destination strip (gated on m_54) and finally writes the scratch
// line back into the working buffer. The two src bases (m_47c straight / m_484
// LUT) and dest base (m_480) are addressed in pixel*bpp units; the per-column
// row offsets come from m_44 / m_48 / m_4c, and the per-pixel source taps are
// pixel indices from the m_478 table. Field names are placeholders; offsets +
// code bytes are load-bearing.
#include <Ints.h>

#include <rva.h>
#include <string.h> // rep-stos memset of the dest strip

// m_38 target: a surface descriptor whose +0xb0 is the bytes-per-pixel (1/2/3).
struct TileSurf {
    char pad[0xb0];
    i32 m_b0; // bytes per pixel
};
SIZE_UNKNOWN(TileSurf);

// m_1c target: a palette/LUT descriptor whose +0x8 is the 2D remap table base.
struct TileLut {
    char pad[0x8];
    u8* m_08;
};
SIZE_UNKNOWN(TileLut);

class CFaderTileRender {
public:
    void RenderTile(i32 arg0, i32 arg1);     // 0x182610
    void RenderWarpTile(i32 arg0, i32 arg1); // 0x181e50

    char m_00[0x1c];
    TileLut* m_1c; // +0x1c remap-table descriptor
    char m_20[0x38 - 0x20];
    TileSurf* m_38; // +0x38 surface (->m_b0 = bytes/pixel)
    char m_3c[0x44 - 0x3c];
    i32* m_44; // +0x44 per-column straight-row byte offset
    i32* m_48; // +0x48 per-column dest-strip byte offset
    i32* m_4c; // +0x4c per-column gather-row byte offset
    i32 m_50;  // +0x50 placement mode (1 / 2)
    i32 m_54;  // +0x54 dest-strip mode (copy vs zero)
    i32 m_58;  // +0x58 half-width (line is 2*m_58 pixels)
    i32 m_5c;  // +0x5c LUT-gather flag
    i32 m_60;  // +0x60 wrap span (used by the PI-scaled warp)
    i32 m_64;  // +0x64 column count
    char m_68[0x478 - 0x68];
    i32* m_478; // +0x478 per-pixel source-tap table (pixel indices)
    u8* m_47c;  // +0x47c straight src base
    u8* m_480;  // +0x480 dest base
    u8* m_484;  // +0x484 gather src base
    u8* m_488;  // +0x488 scratch line
    u8* m_48c;  // +0x48c per-pixel LUT selector
};
SIZE(CFaderTileRender, 0x7d5c);

// ===========================================================================
// 0x182610 - RenderTile: assemble + write back one (2*m_58)-wide line per column.
// ===========================================================================
// @early-stop
// Regalloc / loop-scheduling wall: this is a deep nested gather with ~16 live
// member bases, four per-bpp inner variants and two byte-copy tails. The logic
// (column loop, the m_5c LUT gather vs the 1/2/3-byte straight copies, the m_54
// copy/zero strip, the scratch write-back) is reconstructed faithfully, but MSVC5
// reloads each member base per use and threads the dual src/dest pointers through
// a spill schedule that this C spelling does not reproduce instruction-for-
// instruction. Inner addressing, the *bpp scaling and the LUT keying are correct;
// the spill/reload schedule parks it. Final-sweep candidate.
RVA(0x00182610, 0x2eb)
void CFaderTileRender::RenderTile(i32 arg0, i32 arg1) {
    if (arg1 <= 0) {
        return;
    }
    i32 stride = m_58 * 2; // inner pixel count
    i32 rowBytes = stride + arg1;
    i32 bpp = m_38->m_b0;

    i32 x0;
    u8* src2base;
    u8* destBase;
    if (m_50 == 1) {
        src2base = m_488;
        x0 = arg1;
        destBase = m_480 + (arg0 - arg1) * bpp;
    } else if (m_50 == 2) {
        src2base = m_488 + bpp * stride;
        x0 = 0;
        destBase = m_480 + (arg0 + stride) * bpp;
    } else {
        return;
    }

    u8* srcA = m_47c + (arg0 - x0) * bpp;
    u8* srcB = m_484 + (arg0 - x0) * bpp;
    if (m_64 <= 0) {
        return;
    }

    for (i32 j = 0; j < m_64; j++) {
        u8* rowSrcA = srcA + m_44[j];
        u8* rowSrcB = srcB + m_4c[j];

        if (m_5c) {
            u8* lut = m_1c->m_08;
            for (i32 k = 0; k < stride; k++) {
                u8 b = rowSrcB[m_478[k]];
                m_488[x0 + k] = lut[(b << 6) + m_48c[k]];
            }
        } else if (bpp == 1) {
            for (i32 k = 0; k < stride; k++) {
                m_488[x0 + k] = rowSrcB[m_478[k]];
            }
        } else if (bpp == 2) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_478[k] * 2;
                u8* d = m_488 + (x0 + k) * 2;
                d[0] = s[0];
                d[1] = s[1];
            }
        } else if (bpp == 3) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_478[k] * 3;
                u8* d = m_488 + (x0 + k) * 3;
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
        }

        if (m_54) {
            u8* s = destBase + m_48[j];
            u8* d = src2base;
            for (i32 n = bpp * arg1; n > 0; n--) {
                *d++ = *s++;
            }
        } else {
            memset(src2base, 0, bpp * arg1);
        }

        u8* s = m_488;
        u8* d = rowSrcA;
        for (i32 n = bpp * rowBytes; n > 0; n--) {
            *d++ = *s++;
        }
    }
}

// ===========================================================================
// 0x181e50 - RenderWarpTile: the PI-scaled counterpart of RenderTile. Computes a
// per-tile column split point from a circular (m_58 * PI) arc scaling, then for
// each of m_64 columns gathers a (2*m_58) line (straight bytes + the m_478-tapped
// remainder, or the m_5c LUT path) and writes it back, with the m_54 copy/zero
// dest strip. param_2 = base pixel row, param_3 = leading width.
// ===========================================================================
// @early-stop
// Dual wall: (1) the same deep-loop / many-live-base regalloc schedule that parks
// the sibling RenderTile, and (2) the x87 arc/scale block (fild/fmul PI/fidiv/fimul
// /__ftol) whose fp-stack scheduling cl reorders. Logic (the two condition-gated
// scroll halves, the per-bpp 1/2/3 gather, the LUT remap, the copy/zero strip and
// the scratch write-back) is reconstructed faithfully; FP scheduling + spill order
// park it. Final-sweep candidate.
RVA(0x00181e50, 0x7b9)
void CFaderTileRender::RenderWarpTile(i32 arg0, i32 arg1) {
    i32 stride = m_58 * 2;
    if (arg1 <= 0) {
        return;
    }
    i32 arc = (i32)((double)m_58 * 3.14159);
    i32 bpp = m_38->m_b0;

    i32 colBase;
    if ((m_50 == 1 && m_54 != 0) || (m_50 == 2 && m_54 == 0)) {
        colBase = stride - (i32)((double)stride / (arc - m_58) * (m_60 - arg0 - stride));
    } else {
        colBase = arg0;
    }
    if ((m_50 == 1 && m_54 == 0) || (m_50 == 2 && m_54 != 0)) {
        colBase = (i32)((double)stride / (arc - m_58) * arg0);
    }

    if ((m_50 == 1 && m_54 != 0) || (m_50 == 2 && m_54 == 0)) {
        i32 col = 0;
        if (m_64 > 0) {
            i32 base = bpp * arg0;
            do {
                u8* dstLine = m_44[col] + base + m_47c;
                u8* gsrc = m_4c[col] + base + m_484;
                u8* ssrc = m_48[col] + base + m_480;
                if (m_5c == 0) {
                    if (bpp == 1) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                m_488[i] = ssrc[i];
                                i++;
                            } while (i < colBase);
                        }
                        for (; t < stride; t++) {
                            m_488[t] = gsrc[m_478[t]];
                        }
                    } else if (bpp == 2) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                i32 o = i * 2;
                                m_488[o] = ssrc[o];
                                m_488[o + 1] = ssrc[o + 1];
                                i++;
                            } while (i < colBase);
                        }
                        while (t < stride) {
                            i32 e = t + 1;
                            m_488[e * 2 - 2] = gsrc[m_478[t] * 2];
                            m_488[e * 2 - 1] = gsrc[m_478[t] * 2 + 1];
                            t = e;
                        }
                    } else if (bpp == 3) {
                        if (colBase > 0) {
                            i32 d = 0;
                            u8* sp = ssrc + 2;
                            i32 c = colBase;
                            do {
                                m_488[d] = sp[-2];
                                m_488[d + 1] = sp[-1];
                                m_488[d + 2] = *sp;
                                d += 3;
                                c--;
                                sp += 3;
                            } while (c != 0);
                        }
                        if (colBase < stride) {
                            i32 d = colBase * 3;
                            for (i32 t = colBase; t < stride; t++) {
                                m_488[d] = gsrc[m_478[t] * 3];
                                m_488[d + 1] = gsrc[m_478[t] * 3 + 1];
                                m_488[d + 2] = gsrc[m_478[t] * 3 + 2];
                                d += 3;
                            }
                        }
                    }
                } else {
                    u8* lut = m_1c->m_08;
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            m_488[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        m_488[t] = lut[(u32)m_48c[t] + (u32)gsrc[m_478[t]] * 0x40];
                    }
                }
                u8* sp = m_488;
                i32 cnt = bpp * stride;
                u8* dp = dstLine;
                i32 n = cnt;
                if (cnt > 0) {
                    do {
                        *dp = *sp;
                        sp++;
                        n--;
                        dp++;
                    } while (n != 0);
                }
                if (m_54 == 0) {
                    if (bpp * arg1 > 0) {
                        memset(dstLine + cnt, 0, bpp * arg1);
                    }
                } else {
                    i32 c2 = bpp * arg1;
                    dstLine -= c2;
                    u8* s2 = (arg0 - arg1) * bpp + m_48[col] + m_480;
                    if (c2 > 0) {
                        do {
                            *dstLine = *s2;
                            dstLine++;
                            s2++;
                            c2--;
                        } while (c2 != 0);
                    }
                }
                col++;
            } while (col < m_64);
        }
    } else if (((m_50 == 1 && m_54 == 0) || (m_50 == 2 && m_54 != 0)) && m_64 > 0) {
        i32 col = 0;
        i32 base = bpp * arg0;
        do {
            u8* dstLine = m_44[col] + base + m_47c;
            u8* gsrc = m_4c[col] + base + m_484;
            u8* ssrc = m_48[col] + base + m_480;
            if (m_5c == 0) {
                if (bpp == 1) {
                    i32 i = 0;
                    i32 t = colBase;
                    i32 e;
                    if (colBase > 0) {
                        do {
                            e = i + 1;
                            m_488[i] = gsrc[m_478[i]];
                            i = e;
                        } while (e < colBase);
                    }
                    for (; t < stride; t++) {
                        m_488[t] = ssrc[t];
                    }
                } else if (bpp == 2) {
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            i32 o = i * 4;
                            i++;
                            m_488[i * 2 - 2] = gsrc[m_478[o / 4] * 2];
                            m_488[i * 2 - 1] = gsrc[m_478[i - 1] * 2 + 1];
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        i32 o = t * 2;
                        m_488[o] = ssrc[o];
                        m_488[o + 1] = ssrc[o + 1];
                    }
                } else if (bpp == 3) {
                    i32 k = 0;
                    if (colBase > 0) {
                        i32 d = 0;
                        do {
                            m_488[d] = gsrc[m_478[k] * 3];
                            m_488[d + 1] = gsrc[m_478[k] * 3 + 1];
                            m_488[d + 2] = gsrc[m_478[k] * 3 + 2];
                            k++;
                            d += 3;
                        } while (k < colBase);
                    }
                    if (colBase < stride) {
                        i32 d = colBase * 3;
                        i32 c = stride - colBase;
                        u8* sp = ssrc + 2 + d;
                        do {
                            m_488[d] = sp[-2];
                            m_488[d + 1] = sp[-1];
                            m_488[d + 2] = *sp;
                            d += 3;
                            c--;
                            sp += 3;
                        } while (c != 0);
                    }
                }
            } else {
                u8* lut = m_1c->m_08;
                i32 i = 0;
                i32 t = colBase;
                i32 e;
                if (colBase > 0) {
                    do {
                        e = i + 1;
                        m_488[i] = lut[(u32)m_48c[i] + (u32)gsrc[m_478[i]] * 0x40];
                        i = e;
                    } while (e < colBase);
                }
                for (; t < stride; t++) {
                    m_488[t] = ssrc[t];
                }
            }
            u8* sp = m_488;
            i32 cnt = bpp * stride;
            u8* dp = dstLine;
            i32 n = cnt;
            if (cnt > 0) {
                do {
                    *dp = *sp;
                    sp++;
                    n--;
                    dp++;
                } while (n != 0);
            }
            if (m_54 == 0) {
                if (bpp * arg1 > 0) {
                    memset(dstLine - bpp * arg1, 0, bpp * arg1);
                }
            } else {
                i32 c2 = bpp * arg1;
                u8* s2 = (arg0 + stride) * bpp + m_48[col] + m_480;
                dstLine += cnt;
                if (c2 > 0) {
                    do {
                        *dstLine = *s2;
                        dstLine++;
                        s2++;
                        c2--;
                    } while (c2 != 0);
                }
            }
            col++;
        } while (col < m_64);
    }
}
