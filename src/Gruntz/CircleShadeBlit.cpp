// CircleShadeBlit.cpp - the radial shade-remap blit @0x180fb0 (sits in the fader
// effect cluster near CFaderSine/CFaderRadial). __thiscall, ret 0x18 (6 args).
//
// For the circular band centered at (m_50, m_4c) in a m_2068-wide dest, walk each
// scanline row from the top (cy - sqrt(R2 - dx^2) + 1) down to the center, compute
// the per-row half-length via the integer sqrt (fild/fsqrt/__ftol), and remap the
// boundary pixels through the 2D displacement table `a3` (stride m_2060):
//   out = table[ in_pixel * m_2060 + clippedX ]
// plotting the left point + its horizontal mirror (2*(cx-a0)) and the vertical
// mirror rows (2*cy - row), each gated by the dest bounds (m_2064 / m_2068). The
// compiler specializes the single source loop into four near-identical variants by
// the left/right in-bounds clip + the mirror sign. Field names are placeholders;
// offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <math.h> // (double)int -> fild ; sqrt -> fsqrt ; (int) -> __ftol
#include <rva.h>

// The blit surfaces (this->m_38 src, this->m_3c dst): row pitch at +0x20.
struct CsbSurf {
    char p0[0x20];
    i32 m_20; // +0x20  row pitch
};

struct CircleShadeBlit {
    void Render(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5);
    char p0[0x38];
    CsbSurf* m_38; // +0x38  source surface
    CsbSurf* m_3c; // +0x3c  dest surface
    char p40[0x4c - 0x40];
    i32 m_4c; // +0x4c  center y
    i32 m_50; // +0x50  center x
    char p54[0x2060 - 0x54];
    i32 m_2060; // +0x2060  table stride (radius)
    i32 m_2064; // +0x2064  dest height clip
    i32 m_2068; // +0x2068  dest width
};

// @early-stop
// x87/stack-temp scheduling wall (shared with the sibling fader rasterizers
// CFaderRadial::Build / ImageRotateBlit, both ~30%): the circular-band walk, the
// per-row integer sqrt, the table remap `table[pix*m_2060 + clippedX]`, the
// horizontal-mirror + vertical-mirror plots and the four bounds-clip loop variants
// are reconstructed in shape/order. Residue is the MSVC /O2 stack-temp allocation
// over the ~13 spilled locals + the arg-slot reuse, not source-steerable.
RVA(0x00180fb0, 0x534)
void CircleShadeBlit::Render(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    i32 R = m_2060;
    if (R <= 0) {
        return;
    }
    i32 cx = m_50;
    i32 dx = a0 - cx;
    i32 dx2 = dx * dx;
    i32 cy = m_4c;
    i32 row = cy - (i32)sqrt((double)(a1 - dx2)) + 1;
    i32 len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));

    i32 srcpitch = m_38->m_20;
    i32 srcCol = a0 * srcpitch;
    i32 rowLsrc = a4 + row + srcCol;
    i32 dstpitch = m_3c->m_20;
    i32 dstCol = a0 * dstpitch;
    i32 rowLdst = a5 + row + dstCol;
    i32 rowRsrc = (a4 - row) + srcCol + 2 * cy;
    i32 rowRdst = (a5 - row) + dstCol + 2 * cy;

    i32 mid = m_2068 / 2;
    i32 mirSrc;
    i32 mirDst;
    if (cx >= mid && a0 <= cx) {
        i32 mirCol = 2 * (cx - a0);
        if (mirCol + a0 < m_2068) {
            // ---- both-halves variant (LOOP A) ----
            mirSrc = mirCol * srcpitch;
            mirDst = mirCol * dstpitch;
            if (len < a2 - R) {
                return;
            }
            do {
                if (row > cy) {
                    return;
                }
                i32 cl = len - a2 + R;
                if (row >= 0) {
                    i32 p = *(u8*)rowLdst;
                    *(u8*)rowLsrc = *(u8*)(a3 + p * R + cl);
                    i32 q = *(u8*)(rowLdst + mirDst);
                    *(u8*)(rowLsrc + mirSrc) = *(u8*)(a3 + q * m_2060 + cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_2064) {
                    i32 p = *(u8*)rowRdst;
                    *(u8*)rowRsrc = *(u8*)(a3 + p * m_2060 + cl);
                    i32 q = *(u8*)(rowRdst + mirDst);
                    *(u8*)(rowRsrc + mirSrc) = *(u8*)(a3 + q * m_2060 + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
            } while (len >= a2 - m_2060);
            return;
        }
        // ---- left-only variant (LOOP B) ----
        if (len < a2 - R) {
            return;
        }
        do {
            if (row > cy) {
                return;
            }
            i32 cl = len - a2 + R;
            if (row >= 0) {
                i32 p = *(u8*)rowLdst;
                *(u8*)rowLsrc = *(u8*)(a3 + p * R + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_2064) {
                i32 p = *(u8*)rowRdst;
                *(u8*)rowRsrc = *(u8*)(a3 + p * m_2060 + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
        } while (len >= a2 - m_2060);
        return;
    }

    // ---- cx < mid (or a0 > cx) path ----
    if (cx >= mid) { // a0 > cx
        if (a0 >= mid) {
            return;
        }
    }
    {
        i32 mirCol = 2 * dx;
        i32 right = len - mirCol;
        if (right < 0) {
            // ---- left-only mirrored-negative variant (LOOP D) ----
            if (len < a2 - R) {
                return;
            }
            do {
                if (row > cy) {
                    return;
                }
                i32 cl = len - a2 + R;
                if (row >= 0) {
                    i32 p = *(u8*)rowLdst;
                    *(u8*)rowLsrc = *(u8*)(a3 + p * R + cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_2064) {
                    i32 p = *(u8*)rowRdst;
                    *(u8*)rowRsrc = *(u8*)(a3 + p * m_2060 + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
            } while (len >= a2 - m_2060);
            return;
        }
        // ---- both-halves shifted variant (LOOP C) ----
        mirSrc = mirCol * srcpitch;
        mirDst = mirCol * dstpitch;
        if (len < a2 - R) {
            return;
        }
        do {
            if (row > cy) {
                return;
            }
            i32 cl = len - a2 + R;
            if (row >= 0) {
                i32 p = *(u8*)rowLdst;
                *(u8*)rowLsrc = *(u8*)(a3 + p * R + cl);
                i32 q = *(u8*)(rowLdst - mirDst);
                *(u8*)(rowLsrc - mirSrc) = *(u8*)(a3 + q * m_2060 + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_2064) {
                i32 p = *(u8*)rowRdst;
                *(u8*)rowRsrc = *(u8*)(a3 + p * m_2060 + cl);
                i32 q = *(u8*)(rowRdst - mirDst);
                *(u8*)(rowRsrc - mirSrc) = *(u8*)(a3 + q * m_2060 + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
        } while (len >= a2 - m_2060);
    }
}
