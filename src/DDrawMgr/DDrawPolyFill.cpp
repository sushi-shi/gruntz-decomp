#include <rva.h>

#include <DDrawMgr/DDrawPolyFill.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Image/RasterVtx.h>
#include <Image/WarpTextureBlit.h>
#include <Pix16.h>

#include <ddraw.h>

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

                Pix16Ptr row;
                row.m_bytes = rowPtr;
                g_rasterDestPtr = row.m_swords + lo;
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
