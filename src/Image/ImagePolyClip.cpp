#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawPolyFill.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/WallProject.h>
#include <Image/PolygonWinding.h>
#include <Image/RasterVtx.h>
#include <Image/WarpTextureBlit.h>
#include <Ints.h>
#include <Pix16.h>

#include <ddraw.h>
#include <math.h>
#include <string.h>

DATA(0x001efb10)
const float g_c10 = 0.0f;
DATA(0x001efb18)
const float g_rasterScale = 16384.0f;
DATA(0x001efb1c)
const float g_rasterScaleNeg = -16384.0f;
DATA(0x001efb20)
const float g_c20 = 0.5f;
DATA(0x001efb24)
float g_c24 = -3.1415927f;

DATA(0x002856f0)
i32 g_warpU = 0;
DATA(0x002856f4)
i32 g_warpV = 0;
DATA(0x002856f8)
ClipVtx g_rasterEdgeR[4096];
DATA(0x002a16f8)

i16* g_warpTexBase = 0;
DATA(0x002a16fc)
i32 g_warpUStep = 0;
DATA(0x002a1700)
i32 g_warpVStep = 0;
DATA(0x002a1708)
ClipVtx g_rasterVtxA[100];
DATA(0x002a21f8)
ClipVtx g_rasterVtxB[100];
DATA(0x002a2ce8)
u8* g_rasterDestRow = 0;
DATA(0x002a2cf0)
ClipVtx g_rasterEdgeL[4096];
DATA(0x002becf0)
i32 g_warpUMask = 0;
DATA(0x002becf4)
i16* g_rasterDestPtr = 0;
DATA(0x002becf8)
i32 g_rasterVtxCount = 0;
DATA(0x002becfc)
i16 g_warpColorkey = 0;

DATA(0x001efb14)
const float g_val_1efb14 = -0.01745329238474369f;

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

// @early-stop
// Retail spills dx1/dy1 as float temps and keeps v0->x on the x87 stack across
// the two idivs; cl computes the cross product entirely in x87 registers.
RVA(0x00145e30, 0x125)
i32 PolyIsConvexCW(ClipVtx* verts, i32 count) {
    PolygonWinding sign = POLYGON_WINDING_UNSET;
    PolygonWinding dir = POLYGON_WINDING_UNSET;
    for (i32 i = 0; i <= count; i++) {
        ClipVtx* v0 = &verts[i % count];
        ClipVtx* v1 = &verts[(i + 1) % count];
        ClipVtx* v2 = &verts[(i + 2) % count];
        float x0 = v0->x;
        float y0 = v0->y;
        float dx1 = v1->x - x0;
        float dy1 = v1->y - y0;
        float dx2 = v2->x - x0;
        float dy2 = v2->y - y0;
        float cross = dx1 * dy2 - dx2 * dy1;
        if (cross != 0.0f) {
            if (cross > 0.0f) {
                sign = POLYGON_WINDING_COUNTERCLOCKWISE;
            } else {
                sign = POLYGON_WINDING_CLOCKWISE;
            }
        }
        if (sign != POLYGON_WINDING_UNSET) {
            if (dir != sign && dir != POLYGON_WINDING_UNSET) {
                return 0;
            }
            dir = sign;
        }
    }
    return dir == POLYGON_WINDING_CLOCKWISE;
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

    i32 h = src->m_width;
    i32 w = src->m_height;

    i32 sq[4];
    if (pivot != NULL) {
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

    float rad = rot * 0.01745329238f;
    float sn = static_cast<float>(sin(rad));
    float cs = static_cast<float>(cos(rad));

    i32 hy = h >> 1;
    i32 hx = w >> 1;
    i32 ex[2] = {-hx, w - hx};
    i32 ey[2] = {-hy, h - hy};

    float tx = static_cast<float>(destX);
    float ty = static_cast<float>(destY);

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

    ClipVtx* out = g_rasterVtxA;
    {
        ClipVtx* prev = &poly[n - 1];
        ClipVtx* cur = poly;
        for (i = n; i > 0; i--) {
            if (!(prev->x < left)) {
                *out++ = *prev;
            }
            if ((prev->x < left && !(cur->x < left)) || (!(prev->x < left) && cur->x < left)) {
                out->x = left;
                out->y = prev->y + (left - prev->x) * ((cur->y - prev->y) / (cur->x - prev->x));
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

    out = g_rasterVtxB;
    {
        ClipVtx* prev = &g_rasterVtxA[n1 - 1];
        ClipVtx* cur = g_rasterVtxA;
        for (i = n1; i > 0; i--) {
            if (prev->x < right) {
                *out++ = *prev;
            }
            if ((prev->x < right && !(cur->x < right)) || (!(prev->x < right) && cur->x < right)) {
                out->x = right;
                out->y = prev->y + (right - prev->x) * ((cur->y - prev->y) / (cur->x - prev->x));
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

    out = g_rasterVtxA;
    {
        ClipVtx* prev = &g_rasterVtxB[n2 - 1];
        ClipVtx* cur = g_rasterVtxB;
        for (i = n2; i > 0; i--) {
            if (!(prev->y < top)) {
                *out++ = *prev;
            }
            if ((!(prev->y < top) && cur->y < top) || (prev->y < top && !(cur->y < top))) {
                out->y = top;
                out->x = prev->x + (top - prev->y) * ((cur->x - prev->x) / (cur->y - prev->y));
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

    out = g_rasterVtxB;
    {
        ClipVtx* prev = &g_rasterVtxA[n3 - 1];
        ClipVtx* cur = g_rasterVtxA;
        for (i = n3; i > 0; i--) {
            if (prev->y < bottom) {
                *out++ = *prev;
            }
            if ((prev->y < bottom && !(cur->y < bottom))
                || (!(prev->y < bottom) && cur->y < bottom)) {
                out->y = bottom;
                out->x = prev->x + (bottom - prev->y) * ((cur->x - prev->x) / (cur->y - prev->y));
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

    WarpTextureBlit(g_rasterVtxB, n, dst, src, mode, colorkey);
    return 1;
}

// @early-stop

static inline i16* Span16(u8* row) {
    Pix16Ptr p;
    p.m_bytes = row;
    return p.m_swords;
}

// @early-stop
// Frame/spill residue: retail's ebp frame is 0x28 (two more spill slots) and the
// pow2-shift loop is not peeled; cl peels the first `w & 1` test to `test al,cl`.
RVA(0x00146a20, 0x5b7)
i32 WarpTextureBlit(ClipVtx* va, i32 n, CDDSurface* dst, CDDSurface* src, i32 mode, i32 colorkey) {
    i32 minY = 0x1001;
    i32 maxY = -1;
    if (WarpIsPow2(src->m_width) == 0) {
        return 0;
    }
    g_warpColorkey = static_cast<i16>(colorkey);

    i32 shift = 0;
    {
        i32 m = 1;
        for (;;) {
            if ((src->m_width & m) != 0) {
                break;
            }
            m <<= 1;
            shift++;
            if (static_cast<u32>(shift) >= 0x20) {
                break;
            }
        }
    }

    ClipVtx* prev = &va[n - 1];
    if (n > 0) {
        ClipVtx* cur = va;
        i32 count = n;
        do {
            i32 prevYi = static_cast<i32>(prev->y);
            i32 curYi = static_cast<i32>(cur->y);
            if (prevYi != curYi) {
                ClipVtx* top;
                ClipVtx* bot;
                ClipVtx* table;
                if (cur->y < prev->y) {
                    top = cur;
                    bot = prev;
                    table = g_rasterEdgeL;
                } else {
                    top = prev;
                    bot = cur;
                    table = g_rasterEdgeR;
                }

                i32 topU = static_cast<i32>(static_cast<double>(top->u) * g_rasterScale);
                i32 topV = static_cast<i32>(static_cast<double>(top->v) * g_rasterScale);
                i32 topX = static_cast<i32>(static_cast<double>(top->x) * g_rasterScale);
                i32 topYi = static_cast<i32>(static_cast<double>(top->y) * g_rasterScale) >> 0xe;
                i32 botYi = static_cast<i32>(static_cast<double>(bot->y) * g_rasterScale) >> 0xe;
                i32 h = botYi - topYi;

                ClipVtx* rec = &table[topYi];
                i32 dx =
                    (-topX - static_cast<i32>(static_cast<double>(bot->x) * g_rasterScaleNeg)) / h;
                i32 du =
                    (-topU - static_cast<i32>(static_cast<double>(bot->u) * g_rasterScaleNeg)) / h;
                i32 dv =
                    (-topV - static_cast<i32>(static_cast<double>(bot->v) * g_rasterScaleNeg)) / h;

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
            i32 vy = static_cast<i32>(prev->y);
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

        ClipVtx* lrow = &g_rasterEdgeL[minY];
        ClipVtx* rrow = &g_rasterEdgeR[minY];
        for (; rows > 0; rows--) {
            i32 rx = rrow->fx >> 0xe;
            i32 lx = lrow->fx >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rrow->fu;
                g_warpU = u;
                g_warpV = rrow->fv;
                g_warpUStep = (lrow->fu - u) / span;
                i32 dv = (lrow->fv - g_warpV) / span;
                g_warpV = g_warpV << shift;
                g_warpVStep = dv << shift;

                g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
                __asm {
                    mov  edi, g_rasterDestPtr
                    mov  esi, g_warpTexBase
                    mov  ebx, g_warpU
                    mov  edx, g_warpV
                    mov  ecx, span
                    push ebp
                    mov  ebp, g_warpVStep
                lpOpaque:
                    mov  eax, edx
                    add  edx, ebp
                    and  eax, g_warpUMask
                    or   eax, ebx
                    shr  eax, 0eh
                    add  ebx, g_warpUStep
                    mov  ax, word ptr [esi + eax * 2]
                    mov  word ptr [edi], ax
                    inc  edi
                    inc  edi
                    dec  ecx
                    jne  lpOpaque
                    pop  ebp
                }
            }
            lrow++;
            rrow++;
            g_rasterDestRow += dst->m_pitch;
        }
    } else if (g_warpColorkey == 0) {

        ClipVtx* lrow = &g_rasterEdgeL[minY];
        ClipVtx* rrow = &g_rasterEdgeR[minY];
        for (; rows > 0; rows--) {
            i32 rx = rrow->fx >> 0xe;
            i32 lx = lrow->fx >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rrow->fu;
                g_warpU = u;
                g_warpV = rrow->fv;
                g_warpUStep = (lrow->fu - u) / span;
                i32 dv = (lrow->fv - g_warpV) / span;
                g_warpV = g_warpV << shift;
                g_warpVStep = dv << shift;

                g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
                __asm {
                    mov  edi, g_rasterDestPtr
                    mov  esi, g_warpTexBase
                    mov  ebx, g_warpU
                    mov  edx, g_warpV
                    mov  ecx, span
                    push ebp
                    mov  ebp, g_warpVStep
                lpZeroKey:
                    mov  eax, edx
                    add  edx, ebp
                    and  eax, g_warpUMask
                    or   eax, ebx
                    shr  eax, 0eh
                    add  ebx, g_warpUStep
                    mov  ax, word ptr [esi + eax * 2]
                    or   ax, ax
                    je   nextZeroKey
                    mov  word ptr [edi], ax
                nextZeroKey:
                    inc  edi
                    inc  edi
                    dec  ecx
                    jne  lpZeroKey
                    pop  ebp
                }
            }
            lrow++;
            rrow++;
            g_rasterDestRow += dst->m_pitch;
        }
    } else {

        ClipVtx* lrow = &g_rasterEdgeL[minY];
        ClipVtx* rrow = &g_rasterEdgeR[minY];
        for (; rows > 0; rows--) {
            i32 rx = rrow->fx >> 0xe;
            i32 lx = lrow->fx >> 0xe;
            i32 span = lx - rx;
            if (span > 0) {
                i32 u = rrow->fu;
                g_warpU = u;
                g_warpV = rrow->fv;
                g_warpUStep = (lrow->fu - u) / span;
                i32 dv = (lrow->fv - g_warpV) / span;
                g_warpV = g_warpV << shift;
                g_warpVStep = dv << shift;

                g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
                __asm {
                    mov  edi, g_rasterDestPtr
                    mov  esi, g_warpTexBase
                    mov  ebx, g_warpU
                    mov  edx, g_warpV
                    mov  ecx, span
                    push ebp
                    mov  ebp, g_warpVStep
                lpColorKey:
                    mov  eax, edx
                    add  edx, ebp
                    and  eax, g_warpUMask
                    or   eax, ebx
                    shr  eax, 0eh
                    add  ebx, g_warpUStep
                    mov  ax, word ptr [esi + eax * 2]
                    cmp  ax, g_warpColorkey
                    je   nextColorKey
                    mov  word ptr [edi], ax
                nextColorKey:
                    inc  edi
                    inc  edi
                    dec  ecx
                    jne  lpColorKey
                    pop  ebp
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

// @early-stop
RVA(0x00146fe0, 0x1e2)
i32 FillPolygon(ClipVtx* verts, i32 count, CDDSurface* surf, i16 color) {
    i32 minYi = 0x1001;
    i32 maxYi = -1;
    ClipVtx* prev = &verts[count - 1];
    ClipVtx* cur = verts;
    if (count > 0) {
        i32 n = count;
        do {
            if (static_cast<i32>(cur->y) != static_cast<i32>(prev->y)) {
                ClipVtx* top = prev;
                ClipVtx* table;
                ClipVtx* bottom;
                if (prev->y < cur->y) {
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

                g_rasterDestPtr = Span16(rowPtr) + lo;
                __asm {
                    xor eax, eax
                    mov ax, color
                    mov ecx, width
                    mov edi, g_rasterDestPtr
                    rep stosw
                }
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

// @early-stop
RVA(0x001471d0, 0x1b4)
i32 ProjectWallQuad(
    CDDSurface* surface,
    i32 x0,
    i32 y0,
    i32 x1,
    i32 y1,
    i32 halfWidth,
    i16 color,
    RECT clip
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

    float* w = &g_rasterVtxB[0].x;
    w[0] = static_cast<float>((-s));
    w[1] = static_cast<float>(len);
    w[5] = static_cast<float>(c);
    w[6] = static_cast<float>((c + len));

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

    if (ImagePolyClipRect(g_rasterVtxB, 4, clip.left, clip.top, clip.right, clip.bottom) != 0) {
        FillPolygon(g_rasterVtxB, g_rasterVtxCount, surface, color);
    }
    return 1;
}
