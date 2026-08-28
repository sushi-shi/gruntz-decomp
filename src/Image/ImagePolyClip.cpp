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
const float g_rasterZero = 0.0f;
DATA(0x001efb14)
const float g_degToRadNeg = -0.01745329238474369f;
DATA(0x001efb18)
const float g_rasterScale = 16384.0f;
DATA(0x001efb1c)
const float g_rasterScaleNeg = -16384.0f;
DATA(0x001efb20)
const float g_wallHalf = 0.5f;
DATA(0x001efb24)
const float g_negativePi = -3.1415927f;

DATA(0x002856f0)
i32 g_warpU = 0;
DATA(0x002856f4)
i32 g_warpV = 0;
DATA(0x002856f8)
ClipVtx g_rasterEdgeR[4096];
DATA(0x002a16f8)

i16* g_warpTexBase = NULL;
DATA(0x002a16fc)
i32 g_warpUStep = 0;
DATA(0x002a1700)
i32 g_warpVStep = 0;
DATA(0x002a1708)
ClipVtx g_rasterOddClipPassBuffer[100];
DATA(0x002a21f8)
ClipVtx g_rasterEvenClipPassBuffer[100];
DATA(0x002a2ce8)
u8* g_rasterDestRow = NULL;
DATA(0x002a2cf0)
ClipVtx g_rasterEdgeL[4096];
DATA(0x002becf0)
i32 g_warpUMask = 0;
DATA(0x002becf4)
i16* g_rasterDestPtr = NULL;
DATA(0x002becf8)
i32 g_rasterVtxCount = 0;
DATA(0x002becfc)
i16 g_warpColorkey = 0;

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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
        float x1 = v1->x;
        float y1 = v1->y;
        float x2 = v2->x;
        float y2 = v2->y;
        float dy2, dx2, dy1, dx1;
        dx1 = x1 - x0;
        dy1 = y1 - y0;
        dx2 = x2 - x0;
        dy2 = y2 - y0;
        float cross = (-dy1) * dx2 + dx1 * dy2;
        if (cross != g_rasterZero) {
            if (cross > g_rasterZero) {
                sign = POLYGON_WINDING_COUNTERCLOCKWISE;
            } else {
                sign = POLYGON_WINDING_CLOCKWISE;
            }
        }
        if (sign != POLYGON_WINDING_UNSET) {
            if (sign != dir && dir != POLYGON_WINDING_UNSET) {
                return 0;
            }
            dir = sign;
        }
    }
    return dir == POLYGON_WINDING_CLOCKWISE;
}

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

    i32 w = src->m_width;
    i32 h = src->m_height;

    i32 sq[4];
    if (pivot != NULL) {
        sq[0] = pivot[0];
        sq[1] = pivot[1];
        sq[2] = pivot[2];
        sq[3] = pivot[3];
    } else {
        sq[0] = 0;
        sq[1] = 0;
        sq[2] = w - 1;
        sq[3] = h - 1;
    }

    float rad = rot * g_degToRadNeg;
    float sn = static_cast<float>(sin(rad));
    float cs = static_cast<float>(cos(rad));

    i32 cx = w >> 1;
    i32 cy = h >> 1;

    float ex0 = static_cast<float>(-cx) * scale;
    float ey0 = static_cast<float>(-cy) * scale;
    float ex1 = static_cast<float>(w - cx) * scale;
    float ey1 = static_cast<float>(h - cy) * scale;

    ClipVtx prod[4];
    prod[0].x = ex0;
    prod[0].y = ey0;
    prod[1].x = ex1;
    prod[1].y = ey0;
    prod[2].x = ex1;
    prod[2].y = ey1;
    prod[3].x = ex0;
    prod[3].y = ey1;

    float tx = static_cast<float>(destX);
    float ty = static_cast<float>(destY);

    ClipVtx mtx[4];
    for (i32 k = 0; k < 4; k++) {
        mtx[k].y = prod[k].y * cs - prod[k].x * sn + ty;
        mtx[k].x = prod[k].x * cs + prod[k].y * sn + tx;
    }

    mtx[0].u = static_cast<float>(sq[0]);
    mtx[0].v = static_cast<float>(sq[1]);
    mtx[1].u = static_cast<float>(sq[2]);
    mtx[1].v = static_cast<float>(sq[1]);
    mtx[2].u = static_cast<float>(sq[2]);
    mtx[2].v = static_cast<float>(sq[3]);
    mtx[3].u = static_cast<float>(sq[0]);
    mtx[3].v = static_cast<float>(sq[3]);

    RotateRasterize(mtx, 4, dst, src, mode, colorkey, -1, -1, -1, -1);
}

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

    ClipVtx* out = g_rasterOddClipPassBuffer;
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
    i32 n1 = static_cast<i32>((out - g_rasterOddClipPassBuffer));
    if (n1 == 0) {
        return 0;
    }

    out = g_rasterEvenClipPassBuffer;
    {
        ClipVtx* prev = &g_rasterOddClipPassBuffer[n1 - 1];
        ClipVtx* cur = g_rasterOddClipPassBuffer;
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
    i32 n2 = static_cast<i32>((out - g_rasterEvenClipPassBuffer));
    if (n2 == 0) {
        return 0;
    }

    out = g_rasterOddClipPassBuffer;
    {
        ClipVtx* prev = &g_rasterEvenClipPassBuffer[n2 - 1];
        ClipVtx* cur = g_rasterEvenClipPassBuffer;
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
    i32 n3 = static_cast<i32>((out - g_rasterOddClipPassBuffer));
    if (n3 == 0) {
        return 0;
    }

    out = g_rasterEvenClipPassBuffer;
    {
        ClipVtx* prev = &g_rasterOddClipPassBuffer[n3 - 1];
        ClipVtx* cur = g_rasterOddClipPassBuffer;
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
    i32 n4 = static_cast<i32>((out - g_rasterEvenClipPassBuffer));
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
    i32 clipLeft,
    i32 clipRight,
    i32 clipTop,
    i32 clipBottom
) {
    float leftBound, topBound, rightBound, bottomBound;
    if (clipLeft == -1) {
        topBound = 0.0f;
        rightBound = static_cast<float>(dst->m_width);
        bottomBound = static_cast<float>(dst->m_height);
        leftBound = g_rasterZero;
    } else {
        leftBound = static_cast<float>(clipLeft);
        rightBound = static_cast<float>(clipRight);
        topBound = static_cast<float>(clipTop);
        bottomBound = static_cast<float>(clipBottom);
    }

    ClipVtx* out = g_rasterOddClipPassBuffer;
    {
        ClipVtx* prev = &verts[n - 1];
        ClipVtx* cur = verts;
        if (n > 0) {
            i32 j = n;
            do {
                if (prev->x >= leftBound) {
                    *out++ = *prev;
                }
                if ((prev->x < leftBound && cur->x >= leftBound)
                    || (prev->x >= leftBound && cur->x < leftBound)) {
                    out->x = leftBound;
                    out->y =
                        prev->y + ((cur->y - prev->y) / (cur->x - prev->x)) * (leftBound - prev->x);
                    out->u =
                        prev->u + ((cur->u - prev->u) / (cur->x - prev->x)) * (leftBound - prev->x);
                    out->v =
                        prev->v + ((cur->v - prev->v) / (cur->x - prev->x)) * (leftBound - prev->x);
                    out++;
                }
                prev = cur;
                cur++;
            } while (--j);
        }
    }
    n = static_cast<i32>((out - g_rasterOddClipPassBuffer));
    if (n == 0) {
        return 0;
    }

    {
        ClipVtx* prev = &g_rasterOddClipPassBuffer[n - 1];
        ClipVtx* cur = g_rasterOddClipPassBuffer;
        out = g_rasterEvenClipPassBuffer;
        if (n > 0) {
            i32 j = n;
            do {
                if (prev->x < rightBound) {
                    *out++ = *prev;
                }
                if ((prev->x < rightBound && cur->x >= rightBound)
                    || (prev->x >= rightBound && cur->x < rightBound)) {
                    out->x = rightBound;
                    out->y = prev->y
                             + ((cur->y - prev->y) / (cur->x - prev->x)) * (rightBound - prev->x);
                    out->u = prev->u
                             + ((cur->u - prev->u) / (cur->x - prev->x)) * (rightBound - prev->x);
                    out->v = prev->v
                             + ((cur->v - prev->v) / (cur->x - prev->x)) * (rightBound - prev->x);
                    out++;
                }
                prev = cur;
                cur++;
            } while (--j);
        }
    }
    n = static_cast<i32>((out - g_rasterEvenClipPassBuffer));
    if (n == 0) {
        return 0;
    }

    out = g_rasterOddClipPassBuffer;
    {
        ClipVtx* prev = &g_rasterEvenClipPassBuffer[n - 1];
        if (n > 0) {
            ClipVtx* cur = g_rasterEvenClipPassBuffer;
            i32 j = n;
            do {
                if (prev->y >= topBound) {
                    *out++ = *prev;
                }
                if ((prev->y >= topBound && cur->y < topBound)
                    || (prev->y < topBound && cur->y >= topBound)) {
                    out->y = topBound;
                    out->x =
                        prev->x + ((cur->x - prev->x) / (cur->y - prev->y)) * (topBound - prev->y);
                    out->u =
                        prev->u + ((cur->u - prev->u) / (cur->y - prev->y)) * (topBound - prev->y);
                    out->v =
                        prev->v + ((cur->v - prev->v) / (cur->y - prev->y)) * (topBound - prev->y);
                    out++;
                }
                prev = cur;
                cur++;
            } while (--j);
        }
    }
    n = static_cast<i32>((out - g_rasterOddClipPassBuffer));
    if (n == 0) {
        return 0;
    }

    out = g_rasterEvenClipPassBuffer;
    {
        ClipVtx* prev = &g_rasterOddClipPassBuffer[n - 1];
        if (n > 0) {
            ClipVtx* cur = g_rasterOddClipPassBuffer;
            i32 j = n;
            do {
                if (prev->y < bottomBound) {
                    *out++ = *prev;
                }
                if ((prev->y < bottomBound && cur->y >= bottomBound)
                    || (prev->y >= bottomBound && cur->y < bottomBound)) {
                    out->y = bottomBound;
                    out->x = prev->x
                             + ((cur->x - prev->x) / (cur->y - prev->y)) * (bottomBound - prev->y);
                    out->u = prev->u
                             + ((cur->u - prev->u) / (cur->y - prev->y)) * (bottomBound - prev->y);
                    out->v = prev->v
                             + ((cur->v - prev->v) / (cur->y - prev->y)) * (bottomBound - prev->y);
                    out++;
                }
                prev = cur;
                cur++;
            } while (--j);
        }
    }
    n = static_cast<i32>((out - g_rasterEvenClipPassBuffer));
    if (n == 0) {
        return 0;
    }

    WarpTextureBlit(g_rasterEvenClipPassBuffer, n, dst, src, mode, colorkey);
    return 1;
}

// @early-stop

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

    i32 shift = 0;
    {
        i32 m = 1;
        while (static_cast<u32>(shift) < 0x20) {
            if ((src->m_width & m) != 0) {
                break;
            }
            m <<= 1;
            shift++;
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
                i32 topYi = static_cast<i32>(static_cast<double>(top->y) * g_rasterScale)
                            >> WARP_TEXTURE_FRACTION_BITS;
                i32 botYi = static_cast<i32>(static_cast<double>(bot->y) * g_rasterScale)
                            >> WARP_TEXTURE_FRACTION_BITS;
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

    ClipVtx* lrow = &g_rasterEdgeL[minY];
    ClipVtx* rrow = &g_rasterEdgeR[minY];
    g_warpTexBase = static_cast<i16*>(src->Lock(NULL));
    u8* destBase = static_cast<u8*>(dst->Lock(NULL));
    i32 dstPitch = dst->m_pitch;
    g_rasterDestRow = destBase + dstPitch * minY;
    g_warpUMask = ((src->m_width + 0x3ffff) << WARP_TEXTURE_FRACTION_BITS) << shift;

    if (mode == 0) {
        if (minY < maxY) {
            i32 rows = maxY - minY;
            do {
                i32 rx = rrow->fx >> WARP_TEXTURE_FRACTION_BITS;
                i32 lx = lrow->fx >> WARP_TEXTURE_FRACTION_BITS;
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
            } while (--rows);
        }
    } else if (g_warpColorkey == 0) {
        if (minY < maxY) {
            i32 rows = maxY - minY;
            do {
                i32 rx = rrow->fx >> WARP_TEXTURE_FRACTION_BITS;
                i32 lx = lrow->fx >> WARP_TEXTURE_FRACTION_BITS;
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
            } while (--rows);
        }
    } else {
        if (minY < maxY) {
            i32 rows = maxY - minY;
            do {
                i32 rx = rrow->fx >> WARP_TEXTURE_FRACTION_BITS;
                i32 lx = lrow->fx >> WARP_TEXTURE_FRACTION_BITS;
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
            } while (--rows);
        }
    }

    src->m_ddSurface->Unlock(NULL);
    dst->m_ddSurface->Unlock(NULL);
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
            i32 prevYi = static_cast<i32>(prev->y);
            i32 curYi = static_cast<i32>(cur->y);
            if (prevYi != curYi) {
                ClipVtx* top = prev;
                ClipVtx* bottom = cur;
                ClipVtx* table;
                if (prev->y < cur->y) {
                    table = g_rasterEdgeL;
                } else {
                    top = cur;
                    bottom = prev;
                    table = g_rasterEdgeR;
                }
                i32 topX = static_cast<i32>((top->x * g_rasterScale));
                i32 topYi = static_cast<i32>((top->y * g_rasterScale));
                i32 botYi = static_cast<i32>((bottom->y * g_rasterScale));
                i32 topRow = topYi >> WARP_TEXTURE_FRACTION_BITS;
                ClipVtx* entry = &table[topRow];
                i32 botRow = botYi >> WARP_TEXTURE_FRACTION_BITS;
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
    ClipVtx* pDesc = &g_rasterEdgeL[minYi];
    ClipVtx* pAsc = &g_rasterEdgeR[minYi];
    i32 stride = surf->m_pitch;
    u8* bits = static_cast<u8*>(surf->Lock(NULL));
    u8* rowPtr = bits + stride * minYi;
    g_rasterDestRow = rowPtr;
    if (minYi < maxYi) {
        i32 rowCount = maxYi - minYi;
        do {
            i32 ascendingX = pAsc->fx >> WARP_TEXTURE_FRACTION_BITS;
            i32 descendingX = pDesc->fx >> WARP_TEXTURE_FRACTION_BITS;
            i32 lo = ascendingX;
            i32 hi = descendingX;
            if (ascendingX > descendingX) {
                lo = descendingX;
                hi = ascendingX;
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
    surf->m_ddSurface->Unlock(NULL);
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
    double ang = atan2(static_cast<double>(dx), static_cast<double>(dy));
    float adx = static_cast<float>(fabs(static_cast<float>(dx)));
    float ady = static_cast<float>(fabs(static_cast<float>(dy)));
    float turn = static_cast<float>(ang - g_negativePi);
    float len = static_cast<float>(sqrt(ady * ady + adx * adx));
    double s = sin(turn);
    double c = cos(turn);
    float hw = static_cast<float>(halfWidth);

    float xLeft = -(hw * g_wallHalf);
    float xRight = xLeft + hw;
    g_rasterEvenClipPassBuffer[0].x = xLeft;
    g_rasterEvenClipPassBuffer[0].y = len;
    g_rasterEvenClipPassBuffer[1].x = xRight;
    g_rasterEvenClipPassBuffer[1].y = len;
    g_rasterEvenClipPassBuffer[2].x = xRight;
    g_rasterEvenClipPassBuffer[2].y = g_rasterZero;
    g_rasterEvenClipPassBuffer[3].x = xLeft;
    g_rasterEvenClipPassBuffer[3].y = g_rasterZero;

    for (i32 i = 0; i < 4; i++) {
        float bx = g_rasterEvenClipPassBuffer[i].x;
        float by = -g_rasterEvenClipPassBuffer[i].y;
        g_rasterEvenClipPassBuffer[i].x = static_cast<float>((by * s - bx * c));
        g_rasterEvenClipPassBuffer[i].y = static_cast<float>((bx * s + by * c));
    }
    for (i32 j = 0; j < 4; j++) {
        g_rasterEvenClipPassBuffer[j].x = static_cast<float>(x0) + g_rasterEvenClipPassBuffer[j].x;
        g_rasterEvenClipPassBuffer[j].y = static_cast<float>(y0) + g_rasterEvenClipPassBuffer[j].y;
    }

    if (ImagePolyClipRect(
            g_rasterEvenClipPassBuffer,
            4,
            clip.left,
            clip.top,
            clip.right,
            clip.bottom
        )
        != 0) {
        FillPolygon(g_rasterEvenClipPassBuffer, g_rasterVtxCount, surface, color);
    }
    return 1;
}
