// WarpTextureBlit.cpp - the affine textured-polygon rasterizer @0x146a20 (the
// gouraud/textured sibling of the rotate rasterizer at 0x146550, reached from
// ImageRotate.cpp). __cdecl, 6 args.
//
// Rasterizes a convex polygon (the `n` vertices at `va`/`vb`, 0x1c-byte records
// with float x@0/y@4/u@8/v@0xc) by filling per-scanline left/right edge tables
// (the global arrays at 0x6a2cf0 / 0x6856f8) with 16.16 fixed-point x + the texture
// (u,v) walked via the +65536/-65536 scale trick (the bottom vertex coords are
// multiplied by -65536 so the gradient falls out of a single subtract). Then locks
// the source texture (-> g_warpTexBase) + the dest surface, and for each scanline
// interpolates u/v across the span, sampling the power-of-2 texture by the combined
// index ((V & mask) | U) >> 14, in one of three inner loops: copy-all, skip-zero,
// or skip-colorkey. Field names are placeholders; offsets + code bytes are the
// load-bearing fact.
#include <Ints.h>
#include <DDrawMgr/DDSurface.h>
#include <Image/RasterVtx.h> // ClipVtx (the shared raster vertex) + WarpTextureBlit decl
#include <rva.h>
#include <Globals.h>

// is-power-of-two gate (0x145e00): returns 1 iff exactly one bit of n is set
// (popcount(x) == 1 over all 32 bits). Folded from Stub/BoundaryUpper.cpp
// (PopcountIsOne_145e00); its sole caller is WarpTextureBlit @0x146a20 below.
// @early-stop
// frame-pointer wall (~82%): the popcount loop + parity result are byte-identical,
// but retail is frameless (arg at [esp+4], /Oy applied) while cl keeps the ebp frame
// (push ebp / arg at [ebp+8]) - a whole-function /Oy decision not source-steerable.
RVA(0x00145e00, 0x26)
i32 WarpIsPow2(i32 x) {
    i32 c = 0;
    i32 i = 0x20;
    do {
        if ((x & 1) == 1) {
            c++;
        }
        x >>= 1;
    } while (--i);
    return c == 1;
}

// The polygon vertex is the shared 28-byte ClipVtx (x,y screen; u,v texel coords;
// the c/d/e attrs unused here). Definition in <Image/RasterVtx.h>.

// The locked surface (src texture + dest): m_8 = the lock object (vtable slot 0x80
// = Unlock), m_1c = width, m_20 = row pitch.

// Per-scanline edge tables (left = 0x6a2cf0, right = 0x6856f8): an array of 0x1c-
// byte records {x(16.16), u, v} at +0x10/+0x14/+0x18, indexed by scanline.
DATA(0x002a2cf0)
extern "C" i32 g_rasterEdgeL[]; // 0x6a2cf0
DATA(0x002856f8)
extern "C" i32 g_rasterEdgeR[]; // 0x6856f8

// The rasterizer global scratch (all reloc-masked DATA).
DATA(0x002a2ce8)
extern "C" i32 g_rasterDestRow = 0;
DATA(0x002becf4)
extern "C" i32 g_rasterDestPtr = 0;

// The per-span texture-walk accumulators/steps. Owned by this TU; DEFINED here
// (warptextureblit.obj's .bss, zero-init), DATA()-pinned; reference externs kept in
// <Globals.h>. Ascending retail RVA. (Were extern-only in the Globals.cpp pool.)
DATA(0x002856f0)
i32 g_warpU = 0; // 0x6856f0  (u accumulator)
DATA(0x002856f4)
i32 g_warpV = 0; // 0x6856f4  (v accumulator)
DATA(0x002a16f8)
i32 g_warpTexBase = 0; // 0x6a16f8  (locked texture base)
DATA(0x002a16fc)
i32 g_warpUStep = 0; // 0x6a16fc  (u per-pixel step)
DATA(0x002a1700)
i32 g_warpVStep = 0; // 0x6a1700  (v per-pixel step)
DATA(0x002becf0)
i32 g_warpUMask = 0; // 0x6becf0  (texture index row-mask)
DATA(0x002becfc)
i16 g_warpColorkey = 0; // 0x6becfc

// The fixed-point scale constants (0x5efb18, 0x5efb1c = its negation).
DATA(0x001efb18)
extern "C" const float g_rasterScale = 16384.0f;
DATA(0x001efb1c)
extern "C" const float g_rasterScaleNeg = -16384.0f;

static i32 warpFtol(double v) {
    return (i32)v;
}

// @early-stop
// x87/regalloc scheduling wall (~52%). STRUCTURAL FIX applied: retail keeps the ebp
// frame (push ebp;mov ebp,esp; locals via [ebp-N]) -> this TU is now the `framed`
// (/Oy-) profile, not `base` (was FPO, 50.9->51.6). Residual is regalloc + x87: retail
// keeps minY/maxY MEMORY-resident ([ebp-8]/[ebp-4], immediate stores) under the edge-
// build's x87 register pressure while this /O2 build registers minY in esi, and the
// per-edge fixed-point gradient build is the same MSVC x87 fild/fmul/__ftol pipeline
// interleave + ~15 global-scratch reloc operands as the sibling FP rasterizers - not
// source-steerable from clean C. The three inner pixel loops (copy/skip-zero/skip-
// colorkey), the surface lock/unlock and the span u/v interpolation match by shape.
RVA(0x00146a20, 0x5b7)
i32 WarpTextureBlit(ClipVtx* va, i32 n, CDDSurface* dst, CDDSurface* src, i32 mode, i32 colorkey) {
    i32 minY = 0x1001;
    i32 maxY = -1;
    if (WarpIsPow2(src->m_width) == 0) {
        return 0;
    }
    g_warpColorkey = (i16)colorkey;

    // log2(width): the shift used to fold v into the high texture index bits.
    i32 shift = 0;
    {
        i32 m = 1;
        for (i32 b = 0; b < 0x20; b++) {
            if (src->m_width & m) {
                shift = b;
                break;
            }
            m <<= 1;
        }
    }

    ClipVtx* prev = (ClipVtx*)((char*)va + (7 * n) * 4 - 0x1c); // last vertex
    if (n > 0) {
        ClipVtx* cur = va;
        i32 count = n;
        do {
            i32 prevYi = warpFtol(prev->y);
            i32 curYi = warpFtol(cur->y);
            if (prevYi != curYi) {
                ClipVtx* top;
                ClipVtx* bot;
                i32* table;
                if (prev->y >= cur->y) {
                    top = prev;
                    bot = cur;
                    table = g_rasterEdgeL;
                } else {
                    top = cur;
                    bot = prev;
                    table = g_rasterEdgeR;
                }

                i32 topU = warpFtol((double)top->u * g_rasterScale);
                i32 topV = warpFtol((double)top->v * g_rasterScale);
                i32 topX = warpFtol((double)top->x * g_rasterScale);
                i32 topYi = warpFtol((double)top->y * g_rasterScale) >> 0xe;
                i32 botYi = warpFtol((double)bot->y * g_rasterScale) >> 0xe;
                i32 h = botYi - topYi;

                i32* rec = (i32*)((char*)table + topYi * 0x1c + 0x10);
                i32 dx = (-topX - warpFtol((double)bot->x * g_rasterScaleNeg)) / h;
                i32 du = (-topU - warpFtol((double)bot->u * g_rasterScaleNeg)) / h;
                i32 dv = (-topV - warpFtol((double)bot->v * g_rasterScaleNeg)) / h;

                i32 x = topX;
                i32 u = topU;
                i32 vv = topV;
                for (i32 s = 0; s < h; s++) {
                    rec[0] = x;
                    rec[1] = u;
                    rec[2] = vv;
                    x += dx;
                    u += du;
                    vv += dv;
                    rec = (i32*)((char*)rec + 0x1c);
                }
            }
            i32 vy = warpFtol(prev->y); // [ebp+8] (the current prev vertex) y
            if (vy < minY) {
                minY = vy;
            }
            if (vy > maxY) {
                maxY = vy;
            }
            prev = cur;
            cur = (ClipVtx*)((char*)cur + 0x1c);
        } while (--count);
    }

    g_warpTexBase = src->Lock(0);
    i32 destBase = dst->Lock(0);
    i32 dstPitch = dst->m_pitch;
    g_rasterDestRow = destBase + dstPitch * minY;
    g_warpUMask = ((src->m_width + 0x3ffff) << 0xe) << shift;

    i32 rows = maxY - minY;
    if (mode == 0) {
        // ---- copy-all ----
        i32* lp = (i32*)((char*)g_rasterEdgeL + 0x14 + minY * 0x1c);
        i32* rp = (i32*)((char*)g_rasterEdgeR + 0x14 + minY * 0x1c);
        for (; rows > 0; rows--) {
            i32 rx = rp[-1] >> 0xe;
            i32 lx = lp[-1] >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rp[0];
                i32 vv = rp[1];
                g_warpU = u;
                g_warpV = vv;
                g_warpUStep = (lp[0] - u) / span;
                i32 dv = (lp[1] - vv) / span;
                g_warpV = vv << shift;
                g_warpVStep = dv << shift;
                g_rasterDestPtr = g_rasterDestRow + rx * 2;
                i16* d = (i16*)g_rasterDestPtr;
                i16* tex = (i16*)g_warpTexBase;
                i32 uu = g_warpU;
                i32 va2 = g_warpV;
                for (i32 c = span; c != 0; c--) {
                    i32 idx = ((va2 & g_warpUMask) | uu) >> 0xe;
                    va2 += g_warpVStep;
                    uu += g_warpUStep;
                    *d++ = tex[idx];
                }
            }
            lp = (i32*)((char*)lp + 0x1c);
            rp = (i32*)((char*)rp + 0x1c);
            g_rasterDestRow += dst->m_pitch;
        }
    } else if (g_warpColorkey == 0) {
        // ---- skip-zero ----
        i32* lp = (i32*)((char*)g_rasterEdgeL + 0x14 + minY * 0x1c);
        i32* rp = (i32*)((char*)g_rasterEdgeR + 0x14 + minY * 0x1c);
        for (; rows > 0; rows--) {
            i32 rx = rp[-1] >> 0xe;
            i32 lx = lp[-1] >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rp[0];
                i32 vv = rp[1];
                g_warpU = u;
                g_warpV = vv;
                g_warpUStep = (lp[0] - u) / span;
                i32 dv = (lp[1] - vv) / span;
                g_warpV = vv << shift;
                g_warpVStep = dv << shift;
                g_rasterDestPtr = g_rasterDestRow + rx * 2;
                i16* d = (i16*)g_rasterDestPtr;
                i16* tex = (i16*)g_warpTexBase;
                i32 uu = g_warpU;
                i32 va2 = g_warpV;
                for (i32 c = span; c != 0; c--) {
                    i32 idx = ((va2 & g_warpUMask) | uu) >> 0xe;
                    va2 += g_warpVStep;
                    uu += g_warpUStep;
                    i16 t = tex[idx];
                    if (t != 0) {
                        *d = t;
                    }
                    d++;
                }
            }
            lp = (i32*)((char*)lp + 0x1c);
            rp = (i32*)((char*)rp + 0x1c);
            g_rasterDestRow += dst->m_pitch;
        }
    } else {
        // ---- skip-colorkey ----
        i32* lp = (i32*)((char*)g_rasterEdgeL + 0x14 + minY * 0x1c);
        i32* rp = (i32*)((char*)g_rasterEdgeR + 0x14 + minY * 0x1c);
        for (; rows > 0; rows--) {
            i32 rx = rp[-1] >> 0xe;
            i32 lx = lp[-1] >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rp[0];
                i32 vv = rp[1];
                g_warpU = u;
                g_warpV = vv;
                g_warpUStep = (lp[0] - u) / span;
                i32 dv = (lp[1] - vv) / span;
                g_warpV = vv << shift;
                g_warpVStep = dv << shift;
                g_rasterDestPtr = g_rasterDestRow + rx * 2;
                i16* d = (i16*)g_rasterDestPtr;
                i16* tex = (i16*)g_warpTexBase;
                i32 uu = g_warpU;
                i32 va2 = g_warpV;
                for (i32 c = span; c != 0; c--) {
                    i32 idx = ((va2 & g_warpUMask) | uu) >> 0xe;
                    va2 += g_warpVStep;
                    uu += g_warpUStep;
                    i16 t = tex[idx];
                    if (t != g_warpColorkey) {
                        *d = t;
                    }
                    d++;
                }
            }
            lp = (i32*)((char*)lp + 0x1c);
            rp = (i32*)((char*)rp + 0x1c);
            g_rasterDestRow += dst->m_pitch;
        }
    }

    (*(void (**)(void*, i32))(*(void***)src->m_8 + 0x80 / 4))(src->m_8, 0);
    (*(void (**)(void*, i32))(*(void***)dst->m_8 + 0x80 / 4))(dst->m_8, 0);
    return 1;
}
