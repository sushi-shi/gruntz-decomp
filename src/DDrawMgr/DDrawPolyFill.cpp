#include <DDrawMgr/DirectDrawMgr.h>
#include <Image/RasterVtx.h> // ClipVtx (the shared raster vertex) + FillPolygon decl
#include <Win32.h>           // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>           // real IDirectDrawSurface dispatch (surf->m_8->Unlock)
#include <rva.h>
#include <DDrawMgr/DDrawPolyFill.h> // FillEdgeRow (this TU owns the tables)

DATA(0x002a2cf0)
extern "C" FillEdgeRow g_rasterEdgeL[]; // 0x6a2cf0 (descending-edge table; fill reads +0x10)
DATA(0x002856f8)
extern "C" FillEdgeRow g_rasterEdgeR[]; // 0x6856f8 (ascending-edge table; fill reads +0x10)
DATA(0x002a2ce8)
extern "C" i32 g_rasterDestRow; // 0x6a2ce8  current scanline base (engine scratch)
DATA(0x002becf4)
extern "C" i32 g_rasterDestPtr; // 0x6becf4  current span start (engine scratch)
DATA(0x001efb18)
extern "C" float g_rasterScale; // 0x5efb18  +fixed-point scale
DATA(0x001efb1c)
extern "C" float g_rasterScaleNeg; // 0x5efb1c -fixed-point scale

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
i32 FillPolygon(ClipVtx* verts, i32 count, CDDSurface* surf, i16 color) {
    ClipVtx* prev = reinterpret_cast<ClipVtx*>((reinterpret_cast<char*>(verts) + count * 0x1c - 0x1c));
    ClipVtx* cur = verts;
    i32 minYi = 0x1001;
    i32 maxYi = -1;
    if (count > 0) {
        i32 n = count;
        do {
            if (static_cast<i32>(prev->y) != static_cast<i32>(cur->y)) {
                ClipVtx* top = prev;
                FillEdgeRow* table;
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
                FillEdgeRow* entry = &table[topRow];
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
    i32 rowOff = minYi * 0x1c;
    i32 stride = *reinterpret_cast<i32*>((reinterpret_cast<char*>(surf) + 0x20));
    i32 bits = surf->Lock(0);
    i32 rowPtr = bits + stride * minYi;
    g_rasterDestRow = rowPtr;
    if (minYi < maxYi) {
        i32 rowCount = maxYi - minYi;
        i32* pDesc = reinterpret_cast<i32*>((reinterpret_cast<char*>(g_rasterEdgeL) + rowOff + 0x10));
        i32* pAsc = reinterpret_cast<i32*>((reinterpret_cast<char*>(g_rasterEdgeR) + rowOff + 0x10));
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
                g_rasterDestPtr = rowPtr + lo * 2;
                i16* p = reinterpret_cast<i16*>(g_rasterDestPtr);
                i32 w = width;
                do {
                    *p++ = color;
                } while (--w != 0);
                rowPtr = g_rasterDestRow;
            }
            rowPtr += *reinterpret_cast<i32*>((reinterpret_cast<char*>(surf) + 0x20));
            g_rasterDestRow = rowPtr;
            pAsc = reinterpret_cast<i32*>((reinterpret_cast<char*>(pAsc) + 0x1c));
            pDesc = reinterpret_cast<i32*>((reinterpret_cast<char*>(pDesc) + 0x1c));
        } while (--rowCount != 0);
    }
    surf->m_8->Unlock(0);
    return 1;
}

SIZE_UNKNOWN(FillEdgeRow);
