#include <rva.h>

#include <Mfc.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/MapMgr.h>
#include <Ints.h>
#include <Wap32/Rect.h>

#include <new>
#include <stdlib.h>

#define SCAN_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
        RECT* pb = static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));   \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

// @early-stop

RVA(0x00032ce0, 0x448)
i32 CBattlezMapConfig::ScanRegion(CGrunt* g) {
    if (g->m_stamina >= 0x64) {
        if (g->CoordCount() != 0) {
            Coord* c = static_cast<Coord*>(g->m_coordList.GetTail());
            i32 col = c->m_x;
            i32 row = c->m_y;
            CMapMgr* grid = m_board;
            i32 flags;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
                flags = grid->m_rows[row][col].m_flags;
            } else {
                flags = 1;
            }
            if ((flags & 0x4000) && grid->m_rows[row][col].m_typeCode == 0x99) {
                POSITION pos = g->m_coordList.GetHeadPosition();
                while (pos != 0) {
                    void* coord = g->m_coordList.GetNext(pos);
                    if (coord != 0) {
                        g_coordPool.Push(coord);
                    }
                }
                g->m_coordList.RemoveAll();
                return 1;
            }
        }
        if (g->m_dwell > static_cast<u32>(m_nearbyRouteSearchDelay) && g->CoordCount() == 0) {
            CMapMgr* grid = m_board;
            Coord tp;
            g->GetScreenPos(static_cast<Coord*>(&tp));
            i32 cx = tp.m_x >> 5;
            i32 cy = tp.m_y >> 5;
            RECT box;
            box.left = cx - 5;
            box.top = cy - 5;
            box.right = cx + 5;
            box.bottom = cy + 5;
            RECT gb;
            gb.left = 0;
            gb.top = 0;
            gb.right = grid->m_width;
            gb.bottom = grid->m_height;
            RECT isect;
            if (IntersectRect(&isect, &box, &gb)) {
                i32 hits = 0;
                for (i32 row = isect.top; row < isect.bottom; row++) {
                    if (hits > 4) {
                        break;
                    }
                    BrickzCell* cell = &grid->m_rows[row][isect.left];
                    for (i32 col = isect.left; col < isect.right; col++) {
                        if (hits < 5) {
                            i32 flags = cell->m_flags;
                            if (flags & 0x8000) {
                                if (RouteUnitTo(g, col, row, 0xd87, 0, 0)) {
                                    SCAN_RECT_BOUNDS(grid);
                                    return 1;
                                }
                                hits++;
                            } else if ((flags & 0x4000) && cell->m_typeCode != 0x99) {
                                if (RouteUnitTo(g, col, row, 0xd87, 0, 0)) {
                                    SCAN_RECT_BOUNDS(grid);
                                    return 1;
                                }
                                hits++;
                            }
                        }
                        cell++;
                    }
                }
            }
            SCAN_RECT_BOUNDS(grid);
            if (m_attackWaypoints.GetSize() != 0) {

                Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
                g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            }
            g->m_dwell = 0;
        }
    }
    return 1;
}
