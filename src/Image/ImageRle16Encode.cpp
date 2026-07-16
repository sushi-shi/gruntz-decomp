// ImageRle16Encode.cpp - EncodeRle16 (0x1495d0, __thiscall ret 4). A two-pass
// run-length encoder that converts an 8-bit token/count source (token stream =
// arg0, count stream = m_rleData) into a 16bpp RLE buffer: pass 1 walks the rows to
// size the output (m_rleLen), pass 2 RezAllocs it and emits, expanding each literal
// run's indices through a 256-entry palette->16bpp lookup table (built from the
// m_palette palette with the live screen RGB shift table g_rUp/g_gUp/g_rDown/g_gDown/
// g_bDown, the same table CFileImage::SaveRle16 uses). Returns the encoded buffer.
//
// This is CDDrawShadeBlit::EncodeRle16 (0x1495d0), proven by its sole caller
// CDDrawShadeBlit::Build @0x1490d0 (which passes m_rleData as the token stream and
// uses the result as the srcBpp==2 remap). Fields map 1:1 onto CDDrawShadeBlit:
// m_width/m_height/m_rleData(+0x0c)/m_rleLen(+0x10)/m_palette(+0x20).
#include <rva.h>
#include <DDrawMgr/PixelShift.h> // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown

#include <Ints.h>
#include <Rez/RezMgr.h>              // RezAlloc (_RezAlloc 0x1b9b46)
#include <DDrawMgr/DDrawShadeBlit.h> // the real owner (ex the fake CImageRle16 view)

// @early-stop
// FPU-free but heavy: the palette->16bpp table build, the two-pass row walk (size
// then emit), the literal-run table expansion and the byte-granular token copies are
// byte-faithful. Residual is the MSVC5 8-bit shift narrowing (movb vs movzx) on the
// palette pack + the scratch-index scheduling across the two passes - the optimizer
// register-allocation coin-flip, not source-steerable.
RVA(0x001495d0, 0x1a6)
void* CDDrawShadeBlit::EncodeRle16(const u8* src) {
    u16 table[256];
    {
        const u8* pal = m_palette;
        u16* t = table;
        for (i32 i = 0x100; i != 0; i--) {
            u8 g = static_cast<u8>((static_cast<u8>(pal[1]) >> g_gDown));
            u8 r = static_cast<u8>((static_cast<u8>(pal[0]) >> g_rDown));
            pal += 4;
            u8 b = static_cast<u8>((static_cast<u8>(pal[-2]) >> g_bDown));
            *t++ = static_cast<u16>(((static_cast<u32>(g) << g_gUp) | (static_cast<u32>(r) << g_rUp) | static_cast<u32>(b)));
        }
    }

    // pass 1: size the output into m_rleLen.
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

    // pass 2: allocate + emit.
    u8* out = (u8*)RezAlloc(m_rleLen);
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
                    i32 n = src[srcidx];
                    outidx++;
                    if (n > 0) {
                        const u8* run = src + srcidx + 1;
                        for (i32 k = 0; k < n; k++) {
                            u16 px = table[run[k]];
                            out[outidx] = static_cast<u8>(px);
                            out[outidx + 1] = static_cast<u8>((px >> 8));
                            outidx += 2;
                        }
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

