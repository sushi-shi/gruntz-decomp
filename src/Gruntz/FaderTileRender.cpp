// CFaderTileRender.cpp - the per-column tile gather/remap scanline compositor of
// the big CFader subtype (the 0x7d5c-byte fader allocated by CFaderMgr::Add case
// 2; its Apply path 0x1817e0 calls this). __thiscall, two args (arg0 = base
// pixel row, arg1 = a leading X offset). For each of m_colCount columns it gathers a
// (2*m_halfWidth)-pixel source line into the scratch line m_lineBuf - either straight
// (per-bpp 1/2/3) or through a 64-wide 2D LUT (m_lut->table, keyed by the gathered
// byte and the per-pixel selector m_shadeIndices) when m_useLut is set - then optionally
// copies/zeros a destination strip (gated on m_stripCopy) and finally writes the scratch
// line back into the destination buffer. The gather source (m_gatherBase, tapped
// via m_tapTable) and the straight/edge-strip source (m_straightBase) are read; the
// assembled line is written to m_dstBase - all addressed in pixel*bpp units. The
// per-column row offsets come from m_gatherRowOffsets / m_straightRowOffsets /
// m_dstRowOffsets respectively. Offsets + code bytes are load-bearing.
#include <Ints.h>

#include <rva.h>
#include <string.h> // rep-stos memset of the dest strip

// m_surf's type: a surface descriptor whose +0xb0 is the bytes-per-pixel (1/2/3).
struct TileSurf {
    char pad[0xb0];
    i32 bytesPerPixel; // bytes per pixel
};
SIZE_UNKNOWN(TileSurf);

// m_lut's type: a palette/LUT descriptor whose +0x8 is the 2D remap table base.
struct TileLut {
    char pad[0x8];
    u8* table;
};
SIZE_UNKNOWN(TileLut);

class CFaderTileRender {
public:
    void RenderTile(i32 arg0, i32 arg1);     // 0x182610
    void RenderWarpTile(i32 arg0, i32 arg1); // 0x181e50

    char m_00[0x1c];
    TileLut* m_lut; // +0x1c remap-table descriptor
    char m_20[0x38 - 0x20];
    TileSurf* m_surf; // +0x38 surface (->bytesPerPixel = bytes/pixel)
    char m_3c[0x44 - 0x3c];
    i32* m_dstRowOffsets;      // +0x44 per-column write-back row offset (into m_dstBase)
    i32* m_straightRowOffsets; // +0x48 per-column straight/strip-source row offset
    i32* m_gatherRowOffsets;   // +0x4c per-column gather-source row offset (into m_gatherBase)
    i32 m_placement;           // +0x50 placement mode (1 = leading edge, 2 = trailing edge)
    i32 m_stripCopy;           // +0x54 edge strip: nonzero = copy underlying pixels, 0 = zero-fill
    i32 m_halfWidth; // +0x58 half line-width (line is 2*m_halfWidth px; arc = PI*m_halfWidth)
    i32 m_useLut;    // +0x5c shade-LUT gather flag
    i32 m_span;      // +0x60 total wrap span (used by the PI-scaled warp)
    i32 m_colCount;  // +0x64 column count
    char m_68[0x478 - 0x68];
    i32* m_tapTable;    // +0x478 per-pixel source-tap table (pixel indices into the gather source)
    u8* m_dstBase;      // +0x47c write-back destination line base
    u8* m_straightBase; // +0x480 straight-copy / edge-strip source base
    u8* m_gatherBase;   // +0x484 tap-sampled gather source base
    u8* m_lineBuf;      // +0x488 scratch line assembled before write-back
    u8* m_shadeIndices; // +0x48c per-pixel shade selector (low 6 bits of the LUT index)
};
SIZE(CFaderTileRender, 0x7d5c);

// ===========================================================================
// 0x182610 - RenderTile: assemble + write back one (2*m_halfWidth)-wide line per column.
// ===========================================================================
// @early-stop
// Regalloc / loop-scheduling wall: this is a deep nested gather with ~16 live
// member bases, four per-bpp inner variants and two byte-copy tails. The logic
// (column loop, the m_useLut LUT gather vs the 1/2/3-byte straight copies, the m_stripCopy
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
    i32 stride = m_halfWidth * 2; // inner pixel count
    i32 rowBytes = stride + arg1;
    i32 bpp = m_surf->bytesPerPixel;

    i32 x0;
    u8* src2base;
    u8* destBase;
    if (m_placement == 1) {
        src2base = m_lineBuf;
        x0 = arg1;
        destBase = m_straightBase + (arg0 - arg1) * bpp;
    } else if (m_placement == 2) {
        src2base = m_lineBuf + bpp * stride;
        x0 = 0;
        destBase = m_straightBase + (arg0 + stride) * bpp;
    } else {
        return;
    }

    u8* srcA = m_dstBase + (arg0 - x0) * bpp;
    u8* srcB = m_gatherBase + (arg0 - x0) * bpp;
    if (m_colCount <= 0) {
        return;
    }

    for (i32 j = 0; j < m_colCount; j++) {
        u8* rowSrcA = srcA + m_dstRowOffsets[j];
        u8* rowSrcB = srcB + m_gatherRowOffsets[j];

        if (m_useLut) {
            u8* lut = m_lut->table;
            for (i32 k = 0; k < stride; k++) {
                u8 b = rowSrcB[m_tapTable[k]];
                m_lineBuf[x0 + k] = lut[(b << 6) + m_shadeIndices[k]];
            }
        } else if (bpp == 1) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[x0 + k] = rowSrcB[m_tapTable[k]];
            }
        } else if (bpp == 2) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_tapTable[k] * 2;
                u8* d = m_lineBuf + (x0 + k) * 2;
                d[0] = s[0];
                d[1] = s[1];
            }
        } else if (bpp == 3) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_tapTable[k] * 3;
                u8* d = m_lineBuf + (x0 + k) * 3;
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
        }

        if (m_stripCopy) {
            u8* s = destBase + m_straightRowOffsets[j];
            u8* d = src2base;
            for (i32 n = bpp * arg1; n > 0; n--) {
                *d++ = *s++;
            }
        } else {
            memset(src2base, 0, bpp * arg1);
        }

        u8* s = m_lineBuf;
        u8* d = rowSrcA;
        for (i32 n = bpp * rowBytes; n > 0; n--) {
            *d++ = *s++;
        }
    }
}

// ===========================================================================
// 0x181e50 - RenderWarpTile: the PI-scaled counterpart of RenderTile. Computes a
// per-tile column split point from a circular (m_halfWidth * PI) arc scaling, then for
// each of m_colCount columns gathers a (2*m_halfWidth) line (straight bytes + the m_tapTable-tapped
// remainder, or the m_useLut LUT path) and writes it back, with the m_stripCopy copy/zero
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
    i32 stride = m_halfWidth * 2;
    if (arg1 <= 0) {
        return;
    }
    i32 arc = (i32)((double)m_halfWidth * 3.14159);
    i32 bpp = m_surf->bytesPerPixel;

    i32 colBase;
    if ((m_placement == 1 && m_stripCopy != 0) || (m_placement == 2 && m_stripCopy == 0)) {
        colBase = stride - (i32)((double)stride / (arc - m_halfWidth) * (m_span - arg0 - stride));
    } else {
        colBase = arg0;
    }
    if ((m_placement == 1 && m_stripCopy == 0) || (m_placement == 2 && m_stripCopy != 0)) {
        colBase = (i32)((double)stride / (arc - m_halfWidth) * arg0);
    }

    if ((m_placement == 1 && m_stripCopy != 0) || (m_placement == 2 && m_stripCopy == 0)) {
        i32 col = 0;
        if (m_colCount > 0) {
            i32 base = bpp * arg0;
            do {
                u8* dstLine = m_dstRowOffsets[col] + base + m_dstBase;
                u8* gsrc = m_gatherRowOffsets[col] + base + m_gatherBase;
                u8* ssrc = m_straightRowOffsets[col] + base + m_straightBase;
                if (m_useLut == 0) {
                    if (bpp == 1) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                m_lineBuf[i] = ssrc[i];
                                i++;
                            } while (i < colBase);
                        }
                        for (; t < stride; t++) {
                            m_lineBuf[t] = gsrc[m_tapTable[t]];
                        }
                    } else if (bpp == 2) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                i32 o = i * 2;
                                m_lineBuf[o] = ssrc[o];
                                m_lineBuf[o + 1] = ssrc[o + 1];
                                i++;
                            } while (i < colBase);
                        }
                        while (t < stride) {
                            i32 e = t + 1;
                            m_lineBuf[e * 2 - 2] = gsrc[m_tapTable[t] * 2];
                            m_lineBuf[e * 2 - 1] = gsrc[m_tapTable[t] * 2 + 1];
                            t = e;
                        }
                    } else if (bpp == 3) {
                        if (colBase > 0) {
                            i32 d = 0;
                            u8* sp = ssrc + 2;
                            i32 c = colBase;
                            do {
                                m_lineBuf[d] = sp[-2];
                                m_lineBuf[d + 1] = sp[-1];
                                m_lineBuf[d + 2] = *sp;
                                d += 3;
                                c--;
                                sp += 3;
                            } while (c != 0);
                        }
                        if (colBase < stride) {
                            i32 d = colBase * 3;
                            for (i32 t = colBase; t < stride; t++) {
                                m_lineBuf[d] = gsrc[m_tapTable[t] * 3];
                                m_lineBuf[d + 1] = gsrc[m_tapTable[t] * 3 + 1];
                                m_lineBuf[d + 2] = gsrc[m_tapTable[t] * 3 + 2];
                                d += 3;
                            }
                        }
                    }
                } else {
                    u8* lut = m_lut->table;
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] =
                            lut[(u32)m_shadeIndices[t] + (u32)gsrc[m_tapTable[t]] * 0x40];
                    }
                }
                u8* sp = m_lineBuf;
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
                if (m_stripCopy == 0) {
                    if (bpp * arg1 > 0) {
                        memset(dstLine + cnt, 0, bpp * arg1);
                    }
                } else {
                    i32 c2 = bpp * arg1;
                    dstLine -= c2;
                    u8* s2 = (arg0 - arg1) * bpp + m_straightRowOffsets[col] + m_straightBase;
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
            } while (col < m_colCount);
        }
    } else if (((m_placement == 1 && m_stripCopy == 0) || (m_placement == 2 && m_stripCopy != 0))
               && m_colCount > 0) {
        i32 col = 0;
        i32 base = bpp * arg0;
        do {
            u8* dstLine = m_dstRowOffsets[col] + base + m_dstBase;
            u8* gsrc = m_gatherRowOffsets[col] + base + m_gatherBase;
            u8* ssrc = m_straightRowOffsets[col] + base + m_straightBase;
            if (m_useLut == 0) {
                if (bpp == 1) {
                    i32 i = 0;
                    i32 t = colBase;
                    i32 e;
                    if (colBase > 0) {
                        do {
                            e = i + 1;
                            m_lineBuf[i] = gsrc[m_tapTable[i]];
                            i = e;
                        } while (e < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] = ssrc[t];
                    }
                } else if (bpp == 2) {
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            i32 o = i * 4;
                            i++;
                            m_lineBuf[i * 2 - 2] = gsrc[m_tapTable[o / 4] * 2];
                            m_lineBuf[i * 2 - 1] = gsrc[m_tapTable[i - 1] * 2 + 1];
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        i32 o = t * 2;
                        m_lineBuf[o] = ssrc[o];
                        m_lineBuf[o + 1] = ssrc[o + 1];
                    }
                } else if (bpp == 3) {
                    i32 k = 0;
                    if (colBase > 0) {
                        i32 d = 0;
                        do {
                            m_lineBuf[d] = gsrc[m_tapTable[k] * 3];
                            m_lineBuf[d + 1] = gsrc[m_tapTable[k] * 3 + 1];
                            m_lineBuf[d + 2] = gsrc[m_tapTable[k] * 3 + 2];
                            k++;
                            d += 3;
                        } while (k < colBase);
                    }
                    if (colBase < stride) {
                        i32 d = colBase * 3;
                        i32 c = stride - colBase;
                        u8* sp = ssrc + 2 + d;
                        do {
                            m_lineBuf[d] = sp[-2];
                            m_lineBuf[d + 1] = sp[-1];
                            m_lineBuf[d + 2] = *sp;
                            d += 3;
                            c--;
                            sp += 3;
                        } while (c != 0);
                    }
                }
            } else {
                u8* lut = m_lut->table;
                i32 i = 0;
                i32 t = colBase;
                i32 e;
                if (colBase > 0) {
                    do {
                        e = i + 1;
                        m_lineBuf[i] =
                            lut[(u32)m_shadeIndices[i] + (u32)gsrc[m_tapTable[i]] * 0x40];
                        i = e;
                    } while (e < colBase);
                }
                for (; t < stride; t++) {
                    m_lineBuf[t] = ssrc[t];
                }
            }
            u8* sp = m_lineBuf;
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
            if (m_stripCopy == 0) {
                if (bpp * arg1 > 0) {
                    memset(dstLine - bpp * arg1, 0, bpp * arg1);
                }
            } else {
                i32 c2 = bpp * arg1;
                u8* s2 = (arg0 + stride) * bpp + m_straightRowOffsets[col] + m_straightBase;
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
        } while (col < m_colCount);
    }
}
