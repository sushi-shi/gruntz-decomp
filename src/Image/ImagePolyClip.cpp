// ===========================================================================
// The polygon / texture rasterizer object - retail .text 0x145e00..0x147384,
// eight functions, one contiguous run with no foreign body in it:
//   0x145e00 WarpIsPow2   0x145e30 PolyIsConvexCW   0x145f60 ImageRotateBlit
//   0x1461b0 ImagePolyClipRect   0x146550 RotateRasterize   0x146a20 WarpTextureBlit
//   0x146fe0 FillPolygon   0x1471d0 ProjectWallQuad
//
// It used to be SIX .cpp files (ImagePolyClip / ImageRotate / PolyClipRaster /
// WarpTextureBlit / DDrawPolyFill / WallProject). They are one retail object, proven
// by data, not by RVA proximity:
//
//   .rdata 0x1efb10..0x1efb28 is ONE six-float constant pool -
//     0.0 / -0.01745329 (-pi/180, still unclaimed) / 16384.0 / -16384.0 / 0.5 /
//     -3.1415927 - and the ex WallProject.cpp owned floats 1,5,6 while the ex
//     WarpTextureBlit.cpp owned floats 3,4. Interleaved at DWORD granularity.
//
//   .bss  likewise: ...g_warpUMask(0x6becf0) g_rasterDestPtr(0x6becf4)
//     g_rasterVtxCount(0x6becf8) g_warpColorkey(0x6becfc)... where the third dword
//     came from ex ImagePolyClip.cpp and the others from ex WarpTextureBlit.cpp; and
//     the two big vertex arrays nest exactly inside the warp scratch run
//     (g_rasterEdgeR ends AT g_warpTexBase; g_rasterVtxB ends AT g_rasterDestRow).
//
// One .obj contributes ONE contiguous run per section, so a dword of file A between
// two dwords of file B cannot be two objects. All six units also selected `base`
// except warptextureblit / ddrawpolyfill, which selected `framed` (/Oy-) - a per-unit
// flag that retail cannot have used here, because within this one object retail is
// frameless in WarpIsPow2 / ProjectWallQuad and framed in WarpTextureBlit. That is
// cl's per-function FPO heuristic under plain /O2, not a compiland flag; splitting
// the object to give each function its own /Oy setting was the thing to remove.
// ===========================================================================

#include <Mfc.h> // afx-first (this TU pulls MFC downstream; Mfc.h supersets Win32.h)
#include <Ints.h>
#include <math.h>   // atan2/sin/cos/sqrt/fabs (intrinsified below, at ProjectWallQuad)
#include <string.h> // inline rep-movs struct copy
#include <ddraw.h>  // real IDirectDrawSurface dispatch (surf->m_8->Unlock)

#include <DDrawMgr/DDSurface.h>     // CDDSurface - every entry point's surface argument
#include <DDrawMgr/DirectDrawMgr.h> // FillPolygon's manager-side decls
#include <DDrawMgr/DDrawPolyFill.h> // FillPolygon decl
#include <DDrawMgr/WallProject.h>   // ProjectWallQuad decl + the shared float constants
#include <Image/RasterVtx.h>        // ClipVtx + the g_raster* workspace decls
#include <Image/WarpTextureBlit.h>  // WarpIsPow2 / WarpTextureBlit decls
#include <rva.h>
#include <Pix16.h> // the byte-cursor / 16bpp-value pointer pair

// ---------------------------------------------------------------------------
// The object's data, in retail address order.
//
// .rdata 0x1efb10..0x1efb28 - one six-float constant pool. 0x1efb14 (-pi/180, the
// degrees->radians factor) has no reader in the reconstructed bodies yet, so it is
// left UNCLAIMED rather than invented into a global.
// ---------------------------------------------------------------------------
DATA(0x001efb10)
const float g_c10 = 0.0f;
DATA(0x001efb18)
const float g_rasterScale = 16384.0f; // C linkage from WarpTextureBlit.h
DATA(0x001efb1c)
const float g_rasterScaleNeg = -16384.0f; // C linkage from WarpTextureBlit.h
DATA(0x001efb20)
const float g_c20 = 0.5f;
DATA(0x001efb24)
float g_c24 = -3.1415927f; // -pi (len = sqrt(dx*dx + dy*dy - g_c24))

// The raster scratch workspace - one .bss run. g_rasterEdgeR ends exactly at
// g_warpTexBase, g_rasterVtxB exactly at g_rasterDestRow, g_rasterEdgeL exactly at
// g_warpUMask; the sizes are load-bearing, not guessed.
DATA(0x002856f0)
i32 g_warpU = 0; // 0x6856f0  (u accumulator)
DATA(0x002856f4)
i32 g_warpV = 0; // 0x6856f4  (v accumulator)
DATA(0x002856f8)
ClipVtx g_rasterEdgeR[4096]; // C linkage inherited from <Image/RasterVtx.h>
DATA(0x002a16f8)
// this blitter is 16bpp throughout - texture, span and dest are all i16
i16* g_warpTexBase = 0; // 0x6a16f8  (locked texture base)
DATA(0x002a16fc)
i32 g_warpUStep = 0; // 0x6a16fc  (u per-pixel step)
DATA(0x002a1700)
i32 g_warpVStep = 0; // 0x6a1700  (v per-pixel step)
DATA(0x002a1708)
ClipVtx g_rasterVtxA[100]; // C linkage inherited from <Image/RasterVtx.h>
DATA(0x002a21f8)
ClipVtx g_rasterVtxB[100]; // C linkage inherited from <Image/RasterVtx.h>
DATA(0x002a2ce8)
u8* g_rasterDestRow = 0; // decl in Image/RasterVtx.h
DATA(0x002a2cf0)
ClipVtx g_rasterEdgeL[4096]; // C linkage inherited from <Image/RasterVtx.h>
DATA(0x002becf0)
i32 g_warpUMask = 0; // 0x6becf0  (texture index row-mask)
DATA(0x002becf4)
i16* g_rasterDestPtr = 0; // decl in Image/RasterVtx.h
DATA(0x002becf8)
i32 g_rasterVtxCount = 0; // decl in Image/RasterVtx.h
DATA(0x002becfc)
i16 g_warpColorkey = 0; // 0x6becfc

static i32 warpFtol(double v) {
    return static_cast<i32>(v);
}

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

// ===========================================================================
// PolyIsConvexCW (0x145e30, __cdecl) - winding-consistency test over a `count`-
// vertex polygon. For every vertex triple (i, i+1, i+2 mod count) compute the 2D
// cross product of the two edge vectors; classify its sign (0 unchanged / 1 CCW /
// 2 CW), and bail (return 0) the moment two triples disagree. Returns 1 only when
// every non-degenerate triple is clockwise (dir==2). The loop deliberately runs
// i=0..count inclusive (the closing triple re-checks vertex 0's fan).
// ===========================================================================

RVA(0x00145e30, 0x125)
i32 PolyIsConvexCW(ClipVtx* verts, i32 count) {
    i32 sign = 0;
    i32 dir = 0;
    for (i32 i = 0; i <= count; i++) {
        ClipVtx* v0 = &verts[i % count];
        ClipVtx* v1 = &verts[(i + 1) % count];
        ClipVtx* v2 = &verts[(i + 2) % count];
        float dx1 = v1->x - v0->x;
        float dy1 = v1->y - v0->y;
        float dx2 = v2->x - v0->x;
        float dy2 = v2->y - v0->y;
        float cross = dx1 * dy2 - dx2 * dy1;
        if (cross != 0.0f) {
            if (cross < 0.0f) {
                sign = 2;
            } else {
                sign = 1;
            }
        }
        if (sign != 0) {
            if (dir != 0 && dir != sign) {
                return 0;
            }
            dir = sign;
        }
    }
    return dir == 2;
}

// @early-stop
RVA(0x00145f60, 0x242)
void ImageRotateBlit(
    i32 destX,
    i32 destY,
    i32* pivot,
    CDDSurface* dst,
    CDDSurface* src,
    float rot,
    float scale,
    i32 mode,
    i32 colorkey
) {
    // NB retail (0x145f63/0x145f66) takes the x/u extent from the source surface's
    // +0x18 and the y/v extent from +0x1c - i.e. transposed against the embedded
    // DDSURFACEDESC's own dwHeight/dwWidth naming. The offsets are what is
    // load-bearing; `src` IS a CDDSurface (RotateRasterize forwards it into
    // WarpTextureBlit's CDDSurface* src, which WarpIsPow2's the same +0x1c).
    i32 h = src->m_width;  // +0x1c
    i32 w = src->m_height; // +0x18

    // The source quad corners, stored straight into the transform's texel slots.
    i32 sq[4];
    if (pivot != 0) {
        sq[0] = pivot[0];
        sq[1] = pivot[1];
        sq[2] = pivot[2];
        sq[3] = pivot[3];
    } else {
        sq[0] = 0;
        sq[1] = 0;
        sq[2] = h - 1;
        sq[3] = w - 1;
    }

    float rad = rot * 0.01745329238f; // K(0x5efb14)  deg->rad
    float sn = static_cast<float>(sin(rad));
    float cs = static_cast<float>(cos(rad));

    // Centered half-extents of the source box (kept as ints, fild'd per corner).
    i32 hy = h >> 1;
    i32 hx = w >> 1;
    i32 ex[2] = {-hx, w - hx};
    i32 ey[2] = {-hy, h - hy};

    float tx = static_cast<float>(destX);
    float ty = static_cast<float>(destY);

    // Pass 1: the scaled centered corner products.
    ClipVtx prod[4];
    i32 k = 0;
    i32 iy, ix;
    for (iy = 0; iy < 2; iy++) {
        for (ix = 0; ix < 2; ix++) {
            prod[k].x = static_cast<float>(ex[ix]) * scale;
            prod[k].y = static_cast<float>(ey[iy]) * scale;
            k++;
        }
    }

    // Pass 2: rotate + translate into the screen x/y, texel coords into u/v.
    ClipVtx mtx[4];
    k = 0;
    for (iy = 0; iy < 2; iy++) {
        for (ix = 0; ix < 2; ix++) {
            mtx[k].x = prod[k].x * cs - prod[k].y * sn + tx;
            mtx[k].y = prod[k].x * sn + prod[k].y * cs + ty;
            mtx[k].u = static_cast<float>(sq[ix != 0 ? 3 : 0]);
            mtx[k].v = static_cast<float>(sq[iy != 0 ? 2 : 1]);
            k++;
        }
    }

    RotateRasterize(mtx, 4, dst, src, mode, colorkey, -1, -1, -1, -1);
}

// ===========================================================================
// Clip `poly` (n verts) to the rect [destY (left) .. a4 (right)] x
// [a3 (top) .. a5 (bottom)] in four inlined Sutherland-Hodgman passes.
// ===========================================================================
// @early-stop
RVA(0x001461b0, 0x399)
i32 ImagePolyClipRect(
    ClipVtx* poly,
    i32 n,
    i32 clipLeft,
    i32 clipTop,
    i32 clipRight,
    i32 clipBottom
) {
    float left = static_cast<float>(clipLeft);
    float right = static_cast<float>(clipRight);
    float top = static_cast<float>(clipTop);
    float bottom = static_cast<float>(clipBottom);
    i32 i;

    // Pass 1: keep x >= left. poly -> bufA.
    ClipVtx* out = g_rasterVtxA;
    {
        ClipVtx* prev = &poly[n - 1];
        ClipVtx* cur = poly;
        for (i = n; i > 0; i--) {
            if (!(prev->x < left)) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->x < left) {
                emit = !(cur->x < left);
            } else {
                emit = (cur->x < left);
            }
            if (emit) {
                out->x = left;
                out->y = prev->y + (left - prev->x) * (cur->y - prev->y) / (cur->x - prev->x);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n1 = static_cast<i32>((out - g_rasterVtxA));
    if (n1 == 0) {
        return 0;
    }

    // Pass 2: keep x < right. bufA -> bufB.
    out = g_rasterVtxB;
    {
        ClipVtx* prev = &g_rasterVtxA[n1 - 1];
        ClipVtx* cur = g_rasterVtxA;
        for (i = n1; i > 0; i--) {
            if (prev->x < right) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->x < right) {
                emit = !(cur->x < right);
            } else {
                emit = (cur->x < right);
            }
            if (emit) {
                out->x = right;
                out->y = prev->y + (right - prev->x) * (cur->y - prev->y) / (cur->x - prev->x);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n2 = static_cast<i32>((out - g_rasterVtxB));
    if (n2 == 0) {
        return 0;
    }

    // Pass 3: keep y >= top. bufB -> bufA.
    out = g_rasterVtxA;
    {
        ClipVtx* prev = &g_rasterVtxB[n2 - 1];
        ClipVtx* cur = g_rasterVtxB;
        for (i = n2; i > 0; i--) {
            if (!(prev->y < top)) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->y < top) {
                emit = !(cur->y < top);
            } else {
                emit = (cur->y < top);
            }
            if (emit) {
                out->y = top;
                out->x = prev->x + (top - prev->y) * (cur->x - prev->x) / (cur->y - prev->y);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n3 = static_cast<i32>((out - g_rasterVtxA));
    if (n3 == 0) {
        return 0;
    }

    // Pass 4: keep y < bottom. bufA -> bufB.
    out = g_rasterVtxB;
    {
        ClipVtx* prev = &g_rasterVtxA[n3 - 1];
        ClipVtx* cur = g_rasterVtxA;
        for (i = n3; i > 0; i--) {
            if (prev->y < bottom) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->y < bottom) {
                emit = !(cur->y < bottom);
            } else {
                emit = (cur->y < bottom);
            }
            if (emit) {
                out->y = bottom;
                out->x = prev->x + (bottom - prev->y) * (cur->x - prev->x) / (cur->y - prev->y);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n4 = static_cast<i32>((out - g_rasterVtxB));
    if (n4 == 0) {
        return 0;
    }
    g_rasterVtxCount = n4;
    return 1;
}

// @early-stop
RVA(0x00146550, 0x4ca)
i32 RotateRasterize(
    ClipVtx* verts,
    i32 n,
    CDDSurface* dst,
    CDDSurface* src,
    i32 mode,
    i32 colorkey,
    i32 clipFlag,
    i32 clipB,
    i32 clipC,
    i32 clipD
) {
    float bound0, clip0, clip1, clip2;
    if (clipFlag == -1) {
        // clipFlag == -1 means "clip to the whole destination": +0x1c/+0x18 are the
        // dest surface's DDSURFACEDESC dwWidth/dwHeight (the ex RotateSrcImage view).
        clip1 = 0.0f;
        clip0 = static_cast<float>(dst->m_width);
        clip2 = static_cast<float>(dst->m_height);
        bound0 = 0.0f;
    } else {
        bound0 = static_cast<float>(clipFlag);
        clip0 = static_cast<float>(clipB);
        clip1 = static_cast<float>(clipC);
        clip2 = static_cast<float>(clipD);
    }

    // Pass 1: clip x >= bound0   (verts -> g_rasterVtxA)
    ClipVtx* out = g_rasterVtxA;
    if (n > 0) {
        ClipVtx* prev = &verts[n - 1];
        ClipVtx* cur = verts;
        i32 j = n;
        do {
            if (prev->x >= bound0) {
                *out++ = *prev;
            }
            if ((prev->x >= bound0) != (cur->x >= bound0)) {
                float t = (bound0 - prev->x) / (cur->x - prev->x);
                out->x = bound0;
                out->y = prev->y + (cur->y - prev->y) * t;
                out->u = prev->u + (cur->u - prev->u) * t;
                out->v = prev->v + (cur->v - prev->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxA));
    if (n == 0) {
        return 0;
    }

    // Pass 2: clip x < clip0   (g_rasterVtxA -> g_rasterVtxB)
    out = g_rasterVtxB;
    if (n > 0) {
        ClipVtx* prev = &g_rasterVtxA[n - 1];
        ClipVtx* cur = g_rasterVtxA;
        i32 j = n;
        do {
            if (cur->x < clip0) {
                *out++ = *cur;
            }
            if ((cur->x < clip0) != (prev->x < clip0)) {
                float t = (clip0 - cur->x) / (prev->x - cur->x);
                out->x = clip0;
                out->y = cur->y + (prev->y - cur->y) * t;
                out->u = cur->u + (prev->u - cur->u) * t;
                out->v = cur->v + (prev->v - cur->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxB));
    if (n == 0) {
        return 0;
    }

    // Pass 3: clip y >= clip1   (g_rasterVtxB -> g_rasterVtxA)
    out = g_rasterVtxA;
    if (n > 0) {
        ClipVtx* prev = &g_rasterVtxB[n - 1];
        ClipVtx* cur = g_rasterVtxB;
        i32 j = n;
        do {
            if (cur->y >= clip1) {
                *out++ = *cur;
            }
            if ((cur->y >= clip1) != (prev->y >= clip1)) {
                float t = (clip1 - cur->y) / (prev->y - cur->y);
                out->y = clip1;
                out->x = cur->x + (prev->x - cur->x) * t;
                out->u = cur->u + (prev->u - cur->u) * t;
                out->v = cur->v + (prev->v - cur->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxA));
    if (n == 0) {
        return 0;
    }

    // Pass 4: clip y < clip2   (g_rasterVtxA -> g_rasterVtxB)
    out = g_rasterVtxB;
    if (n > 0) {
        ClipVtx* prev = &g_rasterVtxA[n - 1];
        ClipVtx* cur = g_rasterVtxA;
        i32 j = n;
        do {
            if (cur->y < clip2) {
                *out++ = *cur;
            }
            if ((cur->y < clip2) != (prev->y < clip2)) {
                float t = (clip2 - cur->y) / (prev->y - cur->y);
                out->y = clip2;
                out->x = cur->x + (prev->x - cur->x) * t;
                out->u = cur->u + (prev->u - cur->u) * t;
                out->v = cur->v + (prev->v - cur->v) * t;
                out++;
            }
            prev = cur;
            cur++;
        } while (--j);
    }
    n = static_cast<i32>((out - g_rasterVtxB));
    if (n == 0) {
        return 0;
    }

    // retail 0x1469ee pushes a3 THEN a4 (dst, src) - we pushed a4 twice.
    WarpTextureBlit(g_rasterVtxB, n, dst, src, mode, colorkey);
    return 1;
}

// @early-stop

// The row cursor advances by the surface PITCH (bytes) while the span is written as
// 16bpp pixels - the byte-row -> word-span conversion is the surface API's, so it
// lives at this one seam instead of at each of the three span loops below.
static inline i16* Span16(u8* row) {
    Pix16Ptr p;
    p.m_bytes = row;
    return p.m_swords;
}

RVA(0x00146a20, 0x5b7)
i32 WarpTextureBlit(ClipVtx* va, i32 n, CDDSurface* dst, CDDSurface* src, i32 mode, i32 colorkey) {
    i32 minY = 0x1001;
    i32 maxY = -1;
    if (WarpIsPow2(src->m_width) == 0) {
        return 0;
    }
    g_warpColorkey = static_cast<i16>(colorkey);

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

    ClipVtx* prev = &va[n - 1]; // last vertex
    if (n > 0) {
        ClipVtx* cur = va;
        i32 count = n;
        do {
            i32 prevYi = warpFtol(prev->y);
            i32 curYi = warpFtol(cur->y);
            if (prevYi != curYi) {
                ClipVtx* top;
                ClipVtx* bot;
                ClipVtx* table;
                if (prev->y >= cur->y) {
                    top = prev;
                    bot = cur;
                    table = g_rasterEdgeL;
                } else {
                    top = cur;
                    bot = prev;
                    table = g_rasterEdgeR;
                }

                i32 topU = warpFtol(static_cast<double>(top->u) * g_rasterScale);
                i32 topV = warpFtol(static_cast<double>(top->v) * g_rasterScale);
                i32 topX = warpFtol(static_cast<double>(top->x) * g_rasterScale);
                i32 topYi = warpFtol(static_cast<double>(top->y) * g_rasterScale) >> 0xe;
                i32 botYi = warpFtol(static_cast<double>(bot->y) * g_rasterScale) >> 0xe;
                i32 h = botYi - topYi;

                ClipVtx* rec = &table[topYi];
                i32 dx = (-topX - warpFtol(static_cast<double>(bot->x) * g_rasterScaleNeg)) / h;
                i32 du = (-topU - warpFtol(static_cast<double>(bot->u) * g_rasterScaleNeg)) / h;
                i32 dv = (-topV - warpFtol(static_cast<double>(bot->v) * g_rasterScaleNeg)) / h;

                i32 x = topX;
                i32 u = topU;
                i32 vv = topV;
                for (i32 s = 0; s < h; s++) {
                    rec->fx = x;
                    rec->fu = u;
                    rec->fv = vv;
                    x += dx;
                    u += du;
                    vv += dv;
                    rec++;
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
            cur++;
        } while (--count);
    }

    g_warpTexBase = static_cast<i16*>(src->Lock(0));
    u8* destBase = static_cast<u8*>(dst->Lock(0));
    i32 dstPitch = dst->m_pitch;
    g_rasterDestRow = destBase + dstPitch * minY;
    g_warpUMask = ((src->m_width + 0x3ffff) << 0xe) << shift;

    i32 rows = maxY - minY;
    if (mode == 0) {
        // ---- copy-all ----
        ClipVtx* lrow = &g_rasterEdgeL[minY];
        ClipVtx* rrow = &g_rasterEdgeR[minY];
        for (; rows > 0; rows--) {
            i32 rx = rrow->fx >> 0xe;
            i32 lx = lrow->fx >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rrow->fu;
                i32 vv = rrow->fv;
                g_warpU = u;
                g_warpV = vv;
                g_warpUStep = (lrow->fu - u) / span;
                i32 dv = (lrow->fv - vv) / span;
                g_warpV = vv << shift;
                g_warpVStep = dv << shift;
                // the row cursor advances by the surface PITCH (bytes) while the span
                // is written as 16bpp pixels - the u8*->i16* step is that byte-forced seam
                g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
                i16* d = g_rasterDestPtr;
                i16* tex = g_warpTexBase;
                i32 uu = g_warpU;
                i32 va2 = g_warpV;
                for (i32 c = span; c != 0; c--) {
                    i32 idx = ((va2 & g_warpUMask) | uu) >> 0xe;
                    va2 += g_warpVStep;
                    uu += g_warpUStep;
                    *d++ = tex[idx];
                }
            }
            lrow++;
            rrow++;
            g_rasterDestRow += dst->m_pitch;
        }
    } else if (g_warpColorkey == 0) {
        // ---- skip-zero ----
        ClipVtx* lrow = &g_rasterEdgeL[minY];
        ClipVtx* rrow = &g_rasterEdgeR[minY];
        for (; rows > 0; rows--) {
            i32 rx = rrow->fx >> 0xe;
            i32 lx = lrow->fx >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rrow->fu;
                i32 vv = rrow->fv;
                g_warpU = u;
                g_warpV = vv;
                g_warpUStep = (lrow->fu - u) / span;
                i32 dv = (lrow->fv - vv) / span;
                g_warpV = vv << shift;
                g_warpVStep = dv << shift;
                // the row cursor advances by the surface PITCH (bytes) while the span
                // is written as 16bpp pixels - the u8*->i16* step is that byte-forced seam
                g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
                i16* d = g_rasterDestPtr;
                i16* tex = g_warpTexBase;
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
            lrow++;
            rrow++;
            g_rasterDestRow += dst->m_pitch;
        }
    } else {
        // ---- skip-colorkey ----
        ClipVtx* lrow = &g_rasterEdgeL[minY];
        ClipVtx* rrow = &g_rasterEdgeR[minY];
        for (; rows > 0; rows--) {
            i32 rx = rrow->fx >> 0xe;
            i32 lx = lrow->fx >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rrow->fu;
                i32 vv = rrow->fv;
                g_warpU = u;
                g_warpV = vv;
                g_warpUStep = (lrow->fu - u) / span;
                i32 dv = (lrow->fv - vv) / span;
                g_warpV = vv << shift;
                g_warpVStep = dv << shift;
                // the row cursor advances by the surface PITCH (bytes) while the span
                // is written as 16bpp pixels - the u8*->i16* step is that byte-forced seam
                g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
                i16* d = g_rasterDestPtr;
                i16* tex = g_warpTexBase;
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
            lrow++;
            rrow++;
            g_rasterDestRow += dst->m_pitch;
        }
    }

    src->m_ddSurface->Unlock(0);
    dst->m_ddSurface->Unlock(0);
    return 1;
}

// FillPolygon (0x146fe0, __cdecl) - scanline-fill a polygon into a CDDSurface. Pass 1
// walks each edge (prev->cur, wrapping), ftol's the endpoints, picks the asc/desc edge
// table by edge direction and writes the per-row interpolated x (slope = (-topX-botX)/h),
// while tracking the y bounding box. Pass 2 Locks the surface and, for each row minYi..
// maxYi, reads the two edge x's, orders them and `rep stosw`s the span with `color`,
// stepping the row base by the surface pitch. Finally Unlocks the held surface. ret 1.
// @early-stop
RVA(0x00146fe0, 0x1e2)
i32 FillPolygon(ClipVtx* verts, i32 count, CDDSurface* surf, i16 color) {
    ClipVtx* prev = &verts[count - 1];
    ClipVtx* cur = verts;
    i32 minYi = 0x1001;
    i32 maxYi = -1;
    if (count > 0) {
        i32 n = count;
        do {
            if (static_cast<i32>(prev->y) != static_cast<i32>(cur->y)) {
                ClipVtx* top = prev;
                ClipVtx* table;
                ClipVtx* bottom;
                if (prev->y > cur->y) {
                    bottom = cur;
                    table = g_rasterEdgeL;
                } else {
                    top = cur;
                    bottom = prev;
                    table = g_rasterEdgeR;
                }
                i32 topX = static_cast<i32>((top->x * g_rasterScale));
                i32 topYi = static_cast<i32>((top->y * g_rasterScale));
                i32 botYi = static_cast<i32>((bottom->y * g_rasterScale));
                i32 topRow = topYi >> 0xe;
                ClipVtx* entry = &table[topRow];
                i32 botRow = botYi >> 0xe;
                i32 height = botRow - topRow;
                i32 botX = static_cast<i32>((bottom->x * g_rasterScaleNeg));
                i32 xSlope = (-topX - botX) / height;
                if (topRow < botRow) {
                    i32 x = topX;
                    do {
                        entry->fx = x;
                        x += xSlope;
                        entry++;
                    } while (--height != 0);
                }
            }
            i32 py = static_cast<i32>(prev->y);
            if (py < minYi) {
                minYi = py;
            }
            if (py > maxYi) {
                maxYi = py;
            }
            prev = cur;
            cur++;
        } while (--n != 0);
    }
    i32 stride = surf->m_pitch;
    u8* bits = static_cast<u8*>(surf->Lock(0));
    u8* rowPtr = bits + stride * minYi;
    g_rasterDestRow = rowPtr;
    if (minYi < maxYi) {
        i32 rowCount = maxYi - minYi;
        ClipVtx* pDesc = &g_rasterEdgeL[minYi];
        ClipVtx* pAsc = &g_rasterEdgeR[minYi];
        do {
            i32 xB = pAsc->fx >> 0xe;
            i32 xA = pDesc->fx >> 0xe;
            i32 lo = xB;
            i32 hi = xA;
            if (xB > xA) {
                lo = xA;
                hi = xB;
            }
            i32 width = hi - lo;
            if (width > 0) {
                // rowPtr came from Lock() stepped by the BYTE pitch (m_pitch * minYi);
                // the pixels it addresses are 16bpp. Same seam as the warp spans above.
                g_rasterDestPtr = Span16(rowPtr) + lo;
                i16* p = g_rasterDestPtr;
                i32 w = width;
                do {
                    *p++ = color;
                } while (--w != 0);
                rowPtr = g_rasterDestRow;
            }
            rowPtr += surf->m_pitch;
            g_rasterDestRow = rowPtr;
            pAsc++;
            pDesc++;
        } while (--rowCount != 0);
    }
    surf->m_ddSurface->Unlock(0);
    return 1;
}

// Retail inlined the transcendentals (/Oi alone does not intrinsify them in cl5;
// the pragma does). Placed HERE, immediately before their only user, so the
// earlier bodies compile exactly as they did in their own TUs.
#pragma intrinsic(atan2, sin, cos, sqrt, fabs)

// @early-stop
RVA(0x001471d0, 0x1b4)
i32 ProjectWallQuad(
    CDDSurface* surface,
    i32 x0,
    i32 y0,
    i32 x1,
    i32 y1,
    i32 halfWidth,
    i32 color,
    i32 clipLeft,
    i32 clipTop,
    i32 clipRight,
    i32 clipBottom
) {
    i32 dx = x1 - x0;
    i32 dy = y1 - y0;
    double ang = atan2(static_cast<double>(dy), static_cast<double>(dx));
    double adx = fabs(static_cast<double>(dx));
    double ady = fabs(static_cast<double>(dy));
    double len = sqrt(adx * adx + ady * ady - g_c24);
    double s = sin(ang);
    double c = cos(ang);
    double hw = static_cast<double>(halfWidth);

    // The workspace is written as a flat float grid (7 floats == one ClipVtx record),
    // walked from the first record's leading float member - no cast.
    float* w = &g_rasterVtxB[0].x;
    w[0] = static_cast<float>((-s));
    w[1] = static_cast<float>(len);
    w[5] = static_cast<float>(c);
    w[6] = static_cast<float>((c + len));

    // Both passes rewrite each record IN PLACE, and the second STARTS OVER at record 0.
    // Retail's stores land at [eax-0x20]/[eax-0x1c] AFTER `add eax,0x1c`, i.e. at the very
    // (x,y) the iteration just read at [eax-4]/[eax]; and the translate pass re-loads the
    // cursor (`mov eax,g_rasterVtxB+4`) instead of continuing from the rotate pass's end.
    // Writing `v[-8]/v[-7]` stored 32 bytes BEFORE the read - out of bounds on the first
    // iteration - and let the translate pass run over records 3..6 instead of 0..3.
    // The index must also be the ONLY induction variable: retail's guard is the SIGNED
    // `cmp eax,g_rasterVtxB+0x74 / jl` a strength-reduced `i < 4` leaves behind, where a
    // hand-advanced cursor alongside `i` makes cl emit `dec ecx / jne`.
    for (i32 i = 0; i < 4; i++) {
        float* v = &w[i * 7 + 1];
        double bx = static_cast<double>(v[-1]);
        double by = -static_cast<double>(v[0]);
        v[-1] = static_cast<float>((bx * c * hw - by * s * hw));
        v[0] = static_cast<float>((bx * s * hw + by * c * hw));
    }
    for (i32 j = 0; j < 4; j++) {
        float* v = &w[j * 7 + 1];
        v[-1] = static_cast<float>((static_cast<double>(x0) + static_cast<double>(v[-1])));
        v[0] = static_cast<float>((static_cast<double>(y0) + static_cast<double>(v[0])));
    }

    if (ImagePolyClipRect(g_rasterVtxB, 4, clipLeft, clipTop, clipRight, clipBottom) != 0) {
        FillPolygon(g_rasterVtxB, g_rasterVtxCount, surface, static_cast<i16>(color));
    }
    return 1;
}
