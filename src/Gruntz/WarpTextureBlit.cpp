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
#include <rva.h>
#include <Globals.h>

extern "C" int __ftol(); // 0x11f570 ((int) of an x87 value)

// is-power-of-two gate (0x145e00): returns 1 iff exactly one bit of n is set.
extern i32 WarpIsPow2(i32 n); // 0x145e00

// A polygon vertex (0x1c bytes): x, y, u, v as floats.
struct WarpVtx {
    float x; // +0x00
    float y; // +0x04
    float u; // +0x08
    float v; // +0x0c
    char pad[0x1c - 0x10];
};

// The locked surface (src texture + dest): m_8 = the lock object (vtable slot 0x80
// = Unlock), m_1c = width, m_20 = row pitch.
struct WarpSurf {
    i32 Lock(i32 mode); // 0x13e6d0 (__thiscall, returns the locked base)
    char p0[0x8];
    void* m_8; // +0x08  the lock owner (Unlock via vtable +0x80)
    char p0c[0x1c - 0xc];
    i32 m_1c; // +0x1c  width
    i32 m_20; // +0x20  row pitch
};

// Per-scanline edge tables (left = 0x6a2cf0, right = 0x6856f8): an array of 0x1c-
// byte records {x(16.16), u, v} at +0x10/+0x14/+0x18, indexed by scanline.
DATA(0x002a2cf0)
extern i32 g_warpEdgeL[]; // 0x6a2cf0
DATA(0x002856f8)
extern i32 g_warpEdgeR[]; // 0x6856f8

// The rasterizer global scratch (all reloc-masked DATA).
DATA(0x002a2ce8)
extern i32 g_warpDestRow; // 0x6a2ce8  (current dest scanline base)
DATA(0x002becf4)
extern i32 g_warpDestPtr; // 0x6becf4  (current dest pixel ptr)

// The fixed-point scale constants (0x5efb18, 0x5efb1c = its negation).
DATA(0x001efb18)
extern const float g_warpScale; // 0x5efb18
DATA(0x001efb1c)
extern const float g_warpScaleNeg; // 0x5efb1c

static i32 warpFtol(double v) {
    return (i32)v;
}

// @early-stop
// x87/global-scratch scheduling wall (shared with the sibling FP rasterizers): the
// power-of-2 gate, the per-edge fixed-point gradient build into the left/right edge
// tables, the surface lock/unlock, the span u/v interpolation and the three inner
// pixel loops (copy / skip-zero / skip-colorkey) are reconstructed in shape/order.
// Residue is the MSVC /O2 x87 fld/fmul/__ftol pipeline interleave + the ~15 global
// scratch reloc operands, not source-steerable from clean C.
RVA(0x00146a20, 0x5b7)
i32 WarpTextureBlit(WarpVtx* va, i32 n, WarpSurf* dst, WarpSurf* src, i32 mode, i32 colorkey) {
    i32 minY = 0x1001;
    i32 maxY = -1;
    if (WarpIsPow2(src->m_1c) == 0) {
        return 0;
    }
    g_warpColorkey = (i16)colorkey;

    // log2(width): the shift used to fold v into the high texture index bits.
    i32 shift = 0;
    {
        i32 m = 1;
        for (i32 b = 0; b < 0x20; b++) {
            if (src->m_1c & m) {
                shift = b;
                break;
            }
            m <<= 1;
        }
    }

    WarpVtx* prev = (WarpVtx*)((char*)va + (7 * n) * 4 - 0x1c); // last vertex
    if (n > 0) {
        WarpVtx* cur = va;
        i32 count = n;
        do {
            i32 prevYi = warpFtol(prev->y);
            i32 curYi = warpFtol(cur->y);
            if (prevYi != curYi) {
                WarpVtx* top;
                WarpVtx* bot;
                i32* table;
                if (prev->y >= cur->y) {
                    top = prev;
                    bot = cur;
                    table = g_warpEdgeL;
                } else {
                    top = cur;
                    bot = prev;
                    table = g_warpEdgeR;
                }

                i32 topU = warpFtol((double)top->u * g_warpScale);
                i32 topV = warpFtol((double)top->v * g_warpScale);
                i32 topX = warpFtol((double)top->x * g_warpScale);
                i32 topYi = warpFtol((double)top->y * g_warpScale) >> 0xe;
                i32 botYi = warpFtol((double)bot->y * g_warpScale) >> 0xe;
                i32 h = botYi - topYi;

                i32* rec = (i32*)((char*)table + topYi * 0x1c + 0x10);
                i32 dx = (-topX - warpFtol((double)bot->x * g_warpScaleNeg)) / h;
                i32 du = (-topU - warpFtol((double)bot->u * g_warpScaleNeg)) / h;
                i32 dv = (-topV - warpFtol((double)bot->v * g_warpScaleNeg)) / h;

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
            cur = (WarpVtx*)((char*)cur + 0x1c);
        } while (--count);
    }

    g_warpTexBase = src->Lock(0);
    i32 destBase = dst->Lock(0);
    i32 dstPitch = dst->m_20;
    g_warpDestRow = destBase + dstPitch * minY;
    g_warpUMask = ((src->m_1c + 0x3ffff) << 0xe) << shift;

    i32 rows = maxY - minY;
    if (mode == 0) {
        // ---- copy-all ----
        i32* lp = (i32*)((char*)g_warpEdgeL + 0x14 + minY * 0x1c);
        i32* rp = (i32*)((char*)g_warpEdgeR + 0x14 + minY * 0x1c);
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
                g_warpDestPtr = g_warpDestRow + rx * 2;
                i16* d = (i16*)g_warpDestPtr;
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
            g_warpDestRow += dst->m_20;
        }
    } else if (g_warpColorkey == 0) {
        // ---- skip-zero ----
        i32* lp = (i32*)((char*)g_warpEdgeL + 0x14 + minY * 0x1c);
        i32* rp = (i32*)((char*)g_warpEdgeR + 0x14 + minY * 0x1c);
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
                g_warpDestPtr = g_warpDestRow + rx * 2;
                i16* d = (i16*)g_warpDestPtr;
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
            g_warpDestRow += dst->m_20;
        }
    } else {
        // ---- skip-colorkey ----
        i32* lp = (i32*)((char*)g_warpEdgeL + 0x14 + minY * 0x1c);
        i32* rp = (i32*)((char*)g_warpEdgeR + 0x14 + minY * 0x1c);
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
                g_warpDestPtr = g_warpDestRow + rx * 2;
                i16* d = (i16*)g_warpDestPtr;
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
            g_warpDestRow += dst->m_20;
        }
    }

    (*(void (**)(void*, i32))(*(void***)src->m_8 + 0x80 / 4))(src->m_8, 0);
    (*(void (**)(void*, i32))(*(void***)dst->m_8 + 0x80 / 4))(dst->m_8, 0);
    return 1;
}
SIZE_UNKNOWN(WarpSurf);
SIZE_UNKNOWN(WarpVtx);
