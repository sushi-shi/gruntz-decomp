// DDrawPolyFill.cpp - the DDrawMgr module's polygon scanline-fill rasterizer
// (C:\Proj\DDrawMgr\). FillPolygon (0x146fe0) is the lone __cdecl free function
// here; unlike the rest of the DDrawMgr group it was compiled with the EBP frame
// pointer RETAINED (/O2 /Oy-, the `framed` profile) - it indexes its locals and
// edge state through `ebp`. Split into its own TU so the frame-pointer flag does
// not perturb the FPO functions in CDirectDrawMgr.cpp.
//
// The active-edge tables, the scanline scratch globals and the fixed-point scales
// are reloc-masked DATA externs; CDDSurface::Lock and the IDirectDrawSurface Unlock
// vtable slot are reloc-masked calls (the DX6 vtable supplies the slot layout).
#include <DDrawMgr/DirectDrawMgr.h>
#include <rva.h>

// A polygon vertex (stride 0x1c): float x@+0, float y@+4 (the rest unused here).
struct FillVert {
    float m_0; // x
    float m_4; // y
    char p[0x1c - 8];
};
// A per-scanline edge record (stride 0x1c); the interpolated fixed-point x lands at
// +0x10. The two active-edge tables hold the left/right span endpoints per row.
struct FillEdgeRow {
    char p0[0x10];
    i32 m_10; // +0x10  interpolated x (14-bit fixed point)
    char p1[0x1c - 0x14];
};
DATA(0x002a2cf0)
extern FillEdgeRow g_edgeDesc[]; // 0x6a2cf0 (descending-edge table; fill reads +0x10)
DATA(0x002856f8)
extern FillEdgeRow g_edgeAsc[]; // 0x6856f8 (ascending-edge table; fill reads +0x10)
DATA(0x002a2ce8)
extern i32 g_fillRowPtr; // 0x6a2ce8  current scanline base (engine scratch)
DATA(0x002becf4)
extern i32 g_fillSpanPtr; // 0x6becf4  current span start (engine scratch)
DATA(0x001efb18)
extern float g_fixScale; // 0x5efb18  +fixed-point scale
DATA(0x001efb1c)
extern float g_fixScaleNeg; // 0x5efb1c -fixed-point scale

// FillPolygon (0x146fe0, __cdecl) - scanline-fill a polygon into a CDDSurface. Pass 1
// walks each edge (prev->cur, wrapping), ftol's the endpoints, picks the asc/desc edge
// table by edge direction and writes the per-row interpolated x (slope = (-topX-botX)/h),
// while tracking the y bounding box. Pass 2 Locks the surface and, for each row minYi..
// maxYi, reads the two edge x's, orders them and `rep stosw`s the span with `color`,
// stepping the row base by the surface pitch. Finally Unlocks the held surface. ret 1.
// @early-stop
// FP-scheduling + stack-slot wall (~67%): logic + offsets + the reloc-masked Lock/Unlock/
// ftol are faithful and the EBP frame now matches (framed profile), but retail reuses the
// two incoming arg slots ([ebp+8]/[ebp+0xc]) as the prev/cur temps where our cl spills to
// fresh negative locals, and the x87 endpoint evaluation + edge-slope idiv schedule
// differently. Not source-steerable; deferred to the final sweep. topic:wall.
RVA(0x00146fe0, 0x1e2)
i32 FillPolygon(FillVert* verts, i32 count, CDDSurface* surf, i16 color) {
    FillVert* prev = (FillVert*)((char*)verts + count * 0x1c - 0x1c);
    FillVert* cur = verts;
    i32 minYi = 0x1001;
    i32 maxYi = -1;
    if (count > 0) {
        i32 n = count;
        do {
            if ((i32)prev->m_4 != (i32)cur->m_4) {
                FillVert* top = prev;
                FillEdgeRow* table;
                FillVert* bottom;
                if (prev->m_4 > cur->m_4) {
                    bottom = cur;
                    table = g_edgeDesc;
                } else {
                    top = cur;
                    bottom = prev;
                    table = g_edgeAsc;
                }
                i32 topX = (i32)(top->m_0 * g_fixScale);
                i32 topYi = (i32)(top->m_4 * g_fixScale);
                i32 botYi = (i32)(bottom->m_4 * g_fixScale);
                i32 topRow = topYi >> 0xe;
                FillEdgeRow* entry = &table[topRow];
                i32 botRow = botYi >> 0xe;
                i32 height = botRow - topRow;
                i32 botX = (i32)(bottom->m_0 * g_fixScaleNeg);
                i32 xSlope = (-topX - botX) / height;
                if (topRow < botRow) {
                    i32 x = topX;
                    do {
                        entry->m_10 = x;
                        x += xSlope;
                        entry++;
                    } while (--height != 0);
                }
            }
            i32 py = (i32)prev->m_4;
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
    i32 rowOff = minYi * 0x1c;
    i32 stride = *(i32*)((char*)surf + 0x20);
    i32 bits = surf->Lock(0);
    i32 rowPtr = bits + stride * minYi;
    g_fillRowPtr = rowPtr;
    if (minYi < maxYi) {
        i32 rowCount = maxYi - minYi;
        i32* pDesc = (i32*)((char*)g_edgeDesc + rowOff + 0x10);
        i32* pAsc = (i32*)((char*)g_edgeAsc + rowOff + 0x10);
        do {
            i32 xB = *pAsc >> 0xe;
            i32 xA = *pDesc >> 0xe;
            i32 lo = xB;
            i32 hi = xA;
            if (xB > xA) {
                lo = xA;
                hi = xB;
            }
            i32 width = hi - lo;
            if (width > 0) {
                g_fillSpanPtr = rowPtr + lo * 2;
                i16* p = (i16*)g_fillSpanPtr;
                i32 w = width;
                do {
                    *p++ = color;
                } while (--w != 0);
                rowPtr = g_fillRowPtr;
            }
            rowPtr += *(i32*)((char*)surf + 0x20);
            g_fillRowPtr = rowPtr;
            pAsc = (i32*)((char*)pAsc + 0x1c);
            pDesc = (i32*)((char*)pDesc + 0x1c);
        } while (--rowCount != 0);
    }
    surf->m_8->Unlock(0);
    return 1;
}

SIZE_UNKNOWN(FillVert);
SIZE_UNKNOWN(FillEdgeRow);
