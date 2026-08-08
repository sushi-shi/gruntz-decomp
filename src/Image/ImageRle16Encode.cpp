#include <rva.h>

#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/PixelShift.h>
#include <Ints.h>
#include <Rez/RezMgr.h>

// @early-stop
// 147/147 instructions on retail's frame (0x20c); the residue is one register
// coloring flip - cl gives srcidx edi and k esi where retail has them the other
// way round - plus `add ecx,2` for the two `outidx++` retail splits into two `inc`s
// (spelling them apart costs more than it saves).
RVA(0x001495d0, 0x1a6)
void* CDDrawShadeBlit::EncodeRle16(const u8* src) {
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
