// ImageRle16Encode.cpp - EncodeRle16 (0x1495d0, __thiscall ret 4). A two-pass
// run-length encoder that converts an 8-bit token/count source (token stream =
// arg0, count stream = m_countStream) into a 16bpp RLE buffer: pass 1 walks the rows to
// size the output (m_outputSize), pass 2 RezAllocs it and emits, expanding each literal
// run's indices through a 256-entry palette->16bpp lookup table (built from the
// m_palette palette with the live screen RGB shift table g_rUp/g_gUp/g_rDown/g_gDown/
// g_bDown, the same table CFileImage::SaveRle16 uses). Returns the encoded buffer.
//
// Modeled with an offset-faithful local image view (m_width, m_height, m_countStream
// count stream, m_outputSize out size, m_palette palette); the real home is the CFileImage/
// CImage save family. Offsets + bytes are load-bearing.
#include <rva.h>

#include <Ints.h>
#include <Rez/RezMgr.h> // RezAlloc (_RezAlloc 0x1b9b46)

// The live screen RGB-format unpack/pack shift table (0x683ea0..0x683eb4).
DATA(0x00283ea0)
extern i32 g_rUp;
DATA(0x00283ea4)
extern i32 g_gUp;
DATA(0x00283eac)
extern i32 g_rDown;
DATA(0x00283eb0)
extern i32 g_gDown;
DATA(0x00283eb4)
extern i32 g_bDown;

struct CImageRle16 {
    void* EncodeRle16(const u8* src); // 0x1495d0

    char _00[0x4];
    i32 m_width;       // +0x04
    i32 m_height;      // +0x08
    u8* m_countStream; // +0x0c
    i32 m_outputSize;  // +0x10
    char _14[0x20 - 0x14];
    const u8* m_palette; // +0x20  source palette (256 x 4 = {R,G,B,pad})
};

// @early-stop
// FPU-free but heavy: the palette->16bpp table build, the two-pass row walk (size
// then emit), the literal-run table expansion and the byte-granular token copies are
// byte-faithful. Residual is the MSVC5 8-bit shift narrowing (movb vs movzx) on the
// palette pack + the scratch-index scheduling across the two passes - the optimizer
// register-allocation coin-flip, not source-steerable.
RVA(0x001495d0, 0x1a6)
void* CImageRle16::EncodeRle16(const u8* src) {
    u16 table[256];
    {
        const u8* pal = m_palette;
        u16* t = table;
        for (i32 i = 0x100; i != 0; i--) {
            u8 g = (u8)((u8)pal[1] >> g_gDown);
            u8 r = (u8)((u8)pal[0] >> g_rDown);
            pal += 4;
            u8 b = (u8)((u8)pal[-2] >> g_bDown);
            *t++ = (u16)(((u32)g << g_gUp) | ((u32)r << g_rUp) | (u32)b);
        }
    }

    // pass 1: size the output into m_outputSize.
    m_outputSize = 0;
    {
        i32 x = 0, row = 0, idx = 0;
        if (m_height > 0) {
            i32 rowEndX = m_width - 1;
            do {
                if (src[idx] & 0x80) {
                    m_outputSize++;
                    idx++;
                    x += (i32)m_countStream[idx - 1] - 0x80;
                } else {
                    m_outputSize++;
                    m_outputSize += (i32)src[idx] * 2;
                    x += (i32)m_countStream[idx];
                    idx += (i32)m_countStream[idx] + 1;
                }
                if (x >= rowEndX) {
                    row++;
                    x = 0;
                }
            } while (row < m_height);
        }
    }

    // pass 2: allocate + emit.
    u8* out = (u8*)RezAlloc(m_outputSize);
    {
        i32 outidx = 0, srcidx = 0;
        i32 x2 = 0, row2 = 0;
        if (m_height > 0) {
            do {
                u8 tk = src[srcidx];
                out[outidx] = tk;
                if (tk & 0x80) {
                    outidx++;
                    x2 += (i32)m_countStream[srcidx] - 0x80;
                    srcidx++;
                } else {
                    i32 n = src[srcidx];
                    outidx++;
                    if (n > 0) {
                        const u8* run = src + srcidx + 1;
                        for (i32 k = 0; k < n; k++) {
                            u16 px = table[run[k]];
                            out[outidx] = (u8)px;
                            out[outidx + 1] = (u8)(px >> 8);
                            outidx += 2;
                        }
                    }
                    x2 += (i32)m_countStream[srcidx];
                    srcidx += (i32)m_countStream[srcidx] + 1;
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
