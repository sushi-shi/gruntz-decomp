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
// row offsets come from m_44 / m_48 / m_4c, and the per-pixel source taps from the
// m_478 table. Field names are placeholders; offsets + code bytes are load-bearing.
#include <Ints.h>

#include <rva.h>
#include <string.h> // rep-stos memset of the dest strip

// m_38 target: a surface descriptor whose +0xb0 is the bytes-per-pixel (1/2/3).
struct TileSurf {
    char pad[0xb0];
    i32 m_b0; // bytes per pixel
};

// m_1c target: a palette/LUT descriptor whose +0x8 is the 2D remap table base.
struct TileLut {
    char pad[0x8];
    u8* m_08;
};

class CFaderTileRender {
public:
    void RenderTile(i32 arg0, i32 arg1); // 0x182610

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
    char m_60[0x64 - 0x60];
    i32 m_64; // +0x64 column count
    char m_68[0x478 - 0x68];
    u8** m_478; // +0x478 per-pixel source-tap table (pointers / indices)
    u8* m_47c;  // +0x47c straight src base
    u8* m_480;  // +0x480 dest base
    u8* m_484;  // +0x484 gather src base
    u8* m_488;  // +0x488 scratch line
    u8* m_48c;  // +0x48c per-pixel LUT selector
};

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
    i32 srcB = (i32)m_484 + (arg0 - x0) * bpp;
    if (m_64 <= 0) {
        return;
    }

    for (i32 j = 0; j < m_64; j++) {
        u8* rowSrcA = srcA + m_44[j];
        i32 rowSrcB = srcB + m_4c[j];

        if (m_5c) {
            u8* lut = m_1c->m_08;
            for (i32 k = 0; k < stride; k++) {
                u8 b = *(u8*)((i32)m_478[k] + rowSrcB);
                m_488[x0 + k] = lut[(b << 6) + m_48c[k]];
            }
        } else if (bpp == 1) {
            for (i32 k = 0; k < stride; k++) {
                m_488[x0 + k] = *(u8*)((i32)m_478[k] + rowSrcB);
            }
        } else if (bpp == 2) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = (u8*)(rowSrcB + (i32)m_478[k] * 2);
                u8* d = m_488 + (x0 + k) * 2;
                d[0] = s[0];
                d[1] = s[1];
            }
        } else if (bpp == 3) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = (u8*)(rowSrcB + (i32)m_478[k] * 3);
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
