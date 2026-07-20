#include <DDrawMgr/DDSurface.h>   // the real CDDSurface (m_pitch @+0x20) the blit reads
#include <Gruntz/FaderSubtypes.h> // the canonical CFaderLight (this method's owner)
#include <Ints.h>
#include <math.h> // (double)int -> fild ; sqrt -> fsqrt ; (int) -> __ftol
#include <rva.h>

// @early-stop
// regalloc-cascade wall (~54%; shared with the sibling fader rasterizers
// CFaderRadial::Build / ImageRotateBlit, both ~30%): the circular-band walk, the
// per-row integer sqrt, the table remap `table[pix*m_spanCount + clippedX]`, the
// horizontal-mirror + vertical-mirror plots and the four bounds-clip loop variants
// are reconstructed in shape/order. Residue cascades from the initial `this`
// register pick: retail pins `this` in ESI + lays a 0x20-byte frame, MSVC5 here
// picks EDI + a 0x24 frame, which re-assigns every spilled stack slot across the
// circle-distance/imul block downstream. No source lever flips the `this` register;
// verified base-vs-target with `gruntz sema disasm 0x00180fb0 --diff`.
// docs/patterns/zero-register-pinning.md.
RVA(0x00180fb0, 0x534)
void CFaderLight::Render(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    i32 R = m_spanCount;
    if (R <= 0) {
        return;
    }
    i32 cx = m_centerY;
    i32 dx = a0 - cx;
    i32 dx2 = dx * dx;
    i32 cy = m_centerX;
    i32 row = cy - static_cast<i32>(sqrt(static_cast<double>((a1 - dx2)))) + 1;
    i32 len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));

    i32 srcpitch = m_surface->m_pitch;
    i32 srcCol = a0 * srcpitch;
    i32 rowLsrc = a4 + row + srcCol;
    i32 dstpitch = m_dstSurface->m_pitch;
    i32 dstCol = a0 * dstpitch;
    i32 rowLdst = a5 + row + dstCol;
    i32 rowRsrc = (a4 - row) + srcCol + 2 * cy;
    i32 rowRdst = (a5 - row) + dstCol + 2 * cy;

    i32 mid = m_surfHeight / 2;
    i32 mirSrc;
    i32 mirDst;
    if (cx >= mid && a0 <= cx) {
        i32 mirCol = 2 * (cx - a0);
        if (mirCol + a0 < m_surfHeight) {
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
                    i32 p = *reinterpret_cast<u8*>(rowLdst);
                    *reinterpret_cast<u8*>(rowLsrc) = *reinterpret_cast<u8*>((a3 + p * R + cl));
                    i32 q = *reinterpret_cast<u8*>((rowLdst + mirDst));
                    *reinterpret_cast<u8*>((rowLsrc + mirSrc)) = *reinterpret_cast<u8*>((a3 + q * m_spanCount + cl));
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_surfWidth) {
                    i32 p = *reinterpret_cast<u8*>(rowRdst);
                    *reinterpret_cast<u8*>(rowRsrc) = *reinterpret_cast<u8*>((a3 + p * m_spanCount + cl));
                    i32 q = *reinterpret_cast<u8*>((rowRdst + mirDst));
                    *reinterpret_cast<u8*>((rowRsrc + mirSrc)) = *reinterpret_cast<u8*>((a3 + q * m_spanCount + cl));
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
            } while (len >= a2 - m_spanCount);
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
                i32 p = *reinterpret_cast<u8*>(rowLdst);
                *reinterpret_cast<u8*>(rowLsrc) = *reinterpret_cast<u8*>((a3 + p * R + cl));
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_surfWidth) {
                i32 p = *reinterpret_cast<u8*>(rowRdst);
                *reinterpret_cast<u8*>(rowRsrc) = *reinterpret_cast<u8*>((a3 + p * m_spanCount + cl));
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
        } while (len >= a2 - m_spanCount);
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
                    i32 p = *reinterpret_cast<u8*>(rowLdst);
                    *reinterpret_cast<u8*>(rowLsrc) = *reinterpret_cast<u8*>((a3 + p * R + cl));
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_surfWidth) {
                    i32 p = *reinterpret_cast<u8*>(rowRdst);
                    *reinterpret_cast<u8*>(rowRsrc) = *reinterpret_cast<u8*>((a3 + p * m_spanCount + cl));
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
            } while (len >= a2 - m_spanCount);
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
                i32 p = *reinterpret_cast<u8*>(rowLdst);
                *reinterpret_cast<u8*>(rowLsrc) = *reinterpret_cast<u8*>((a3 + p * R + cl));
                i32 q = *reinterpret_cast<u8*>((rowLdst - mirDst));
                *reinterpret_cast<u8*>((rowLsrc - mirSrc)) = *reinterpret_cast<u8*>((a3 + q * m_spanCount + cl));
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_surfWidth) {
                i32 p = *reinterpret_cast<u8*>(rowRdst);
                *reinterpret_cast<u8*>(rowRsrc) = *reinterpret_cast<u8*>((a3 + p * m_spanCount + cl));
                i32 q = *reinterpret_cast<u8*>((rowRdst - mirDst));
                *reinterpret_cast<u8*>((rowRsrc - mirSrc)) = *reinterpret_cast<u8*>((a3 + q * m_spanCount + cl));
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
        } while (len >= a2 - m_spanCount);
    }
}
