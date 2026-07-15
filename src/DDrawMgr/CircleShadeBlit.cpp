// CircleShadeBlit.cpp - the radial shade-remap blit @0x180fb0 (ghidra "Render").
// __thiscall, ret 0x18 (6 args).
//
// IDENTITY (proven): `this` IS a CFaderLight (<Gruntz/FaderSubtypes.h>). The view's
// size 0x206c (m_destWidth @+0x2068, +4) is EXACTLY CFaderLight's SIZE 0x206c - the
// only fader subtype of that size (CFaderSine 0x7d5c, CFaderRadial 0x5c, CFaderShape
// 0x494, CFaderMesh 0x6c, CFaderFlat 0x50) - and this method sits in the fader
// cluster. So this is CFaderLight::Render (0x180fb0), a slot the canonical has not
// modeled yet; the surface members are real CDDSurface (m_pitch @+0x20; ex CsbSurf).
//
// DEFERRED-FOLD (home onto CFaderLight::Render) blocked on a layout reconciliation
// with the canonical - both hops need a disasm cross-check of CFaderLight's OTHER
// methods (ApplyInit 0x1804a0 / v2 0x1814f0) before the canonical can be edited:
//   (1) @+0x3c: Render dereferences it as a CDDSurface* (m_dstSurface->m_pitch, like
//       sibling CFaderRadial's m_dstSurface @+0x3c), but the canonical types it
//       `i32 m_3c` (used elsewhere in Fader.cpp) - retyping ripples into that TU.
//   (2) centre: Render reads the X centre @+0x50 and the Y centre @+0x4c, but the
//       canonical (desc-backed defaults 0x140/0xf0) labels centerX @+0x4c / centerY
//       @+0x50 - the opposite. ApplyInit's stores must arbitrate which is right.
// Kept as a correctly-attributed structural view (offsets load-bearing) until that
// reconciliation lands with the Fader-lane canonical.
//
// For the circular band centered at (m_centerX, m_centerY) in a m_destWidth-wide dest, walk each
// scanline row from the top (cy - sqrt(R2 - dx^2) + 1) down to the center, compute
// the per-row half-length via the integer sqrt (fild/fsqrt/__ftol), and remap the
// boundary pixels through the 2D displacement table `a3` (stride m_tableStride):
//   out = table[ in_pixel * m_tableStride + clippedX ]
// plotting the left point + its horizontal mirror (2*(cx-a0)) and the vertical
// mirror rows (2*cy - row), each gated by the dest bounds (m_destHeightClip / m_destWidth). The
// compiler specializes the single source loop into four near-identical variants by
// the left/right in-bounds clip + the mirror sign. Field names are placeholders;
// offsets + code bytes are the load-bearing fact.
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (m_pitch @+0x20) the blit reads
#include <Ints.h>
#include <math.h> // (double)int -> fild ; sqrt -> fsqrt ; (int) -> __ftol
#include <rva.h>

// The blit surfaces (this->m_srcSurface src, this->m_dstSurface dst) are real
// CDDSurface wrappers; the blit reads only their row pitch (m_pitch @+0x20).
struct CircleShadeBlit {
    void Render(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5);
    char p0[0x38];
    CDDSurface* m_srcSurface; // +0x38  source surface
    CDDSurface* m_dstSurface; // +0x3c  dest surface
    char p40[0x4c - 0x40];
    i32 m_centerY; // +0x4c  center y
    i32 m_centerX; // +0x50  center x
    char p54[0x2060 - 0x54];
    i32 m_tableStride;    // +0x2060  table stride (radius)
    i32 m_destHeightClip; // +0x2064  dest height clip
    i32 m_destWidth;      // +0x2068  dest width
};

// @early-stop
// regalloc-cascade wall (~54%; shared with the sibling fader rasterizers
// CFaderRadial::Build / ImageRotateBlit, both ~30%): the circular-band walk, the
// per-row integer sqrt, the table remap `table[pix*m_tableStride + clippedX]`, the
// horizontal-mirror + vertical-mirror plots and the four bounds-clip loop variants
// are reconstructed in shape/order. Residue cascades from the initial `this`
// register pick: retail pins `this` in ESI + lays a 0x20-byte frame, MSVC5 here
// picks EDI + a 0x24 frame, which re-assigns every spilled stack slot across the
// circle-distance/imul block downstream. No source lever flips the `this` register;
// verified base-vs-target with `gruntz sema disasm 0x00180fb0 --diff`.
// docs/patterns/zero-register-pinning.md.
RVA(0x00180fb0, 0x534)
void CircleShadeBlit::Render(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    i32 R = m_tableStride;
    if (R <= 0) {
        return;
    }
    i32 cx = m_centerX;
    i32 dx = a0 - cx;
    i32 dx2 = dx * dx;
    i32 cy = m_centerY;
    i32 row = cy - (i32)sqrt((double)(a1 - dx2)) + 1;
    i32 len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));

    i32 srcpitch = m_srcSurface->m_pitch;
    i32 srcCol = a0 * srcpitch;
    i32 rowLsrc = a4 + row + srcCol;
    i32 dstpitch = m_dstSurface->m_pitch;
    i32 dstCol = a0 * dstpitch;
    i32 rowLdst = a5 + row + dstCol;
    i32 rowRsrc = (a4 - row) + srcCol + 2 * cy;
    i32 rowRdst = (a5 - row) + dstCol + 2 * cy;

    i32 mid = m_destWidth / 2;
    i32 mirSrc;
    i32 mirDst;
    if (cx >= mid && a0 <= cx) {
        i32 mirCol = 2 * (cx - a0);
        if (mirCol + a0 < m_destWidth) {
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
                    *(u8*)(rowLsrc + mirSrc) = *(u8*)(a3 + q * m_tableStride + cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_destHeightClip) {
                    i32 p = *(u8*)rowRdst;
                    *(u8*)rowRsrc = *(u8*)(a3 + p * m_tableStride + cl);
                    i32 q = *(u8*)(rowRdst + mirDst);
                    *(u8*)(rowRsrc + mirSrc) = *(u8*)(a3 + q * m_tableStride + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
            } while (len >= a2 - m_tableStride);
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
            if (2 * cy - row < m_destHeightClip) {
                i32 p = *(u8*)rowRdst;
                *(u8*)rowRsrc = *(u8*)(a3 + p * m_tableStride + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
        } while (len >= a2 - m_tableStride);
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
                if (2 * cy - row < m_destHeightClip) {
                    i32 p = *(u8*)rowRdst;
                    *(u8*)rowRsrc = *(u8*)(a3 + p * m_tableStride + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
            } while (len >= a2 - m_tableStride);
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
                *(u8*)(rowLsrc - mirSrc) = *(u8*)(a3 + q * m_tableStride + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_destHeightClip) {
                i32 p = *(u8*)rowRdst;
                *(u8*)rowRsrc = *(u8*)(a3 + p * m_tableStride + cl);
                i32 q = *(u8*)(rowRdst - mirDst);
                *(u8*)(rowRsrc - mirSrc) = *(u8*)(a3 + q * m_tableStride + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = (i32)sqrt((double)((row - cy) * (row - cy) + dx2));
        } while (len >= a2 - m_tableStride);
    }
}

SIZE_UNKNOWN(CircleShadeBlit);
