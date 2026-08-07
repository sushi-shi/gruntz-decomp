#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>

#include <stdlib.h>

#define SCAN_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        CRect clip(0, 0, (grid)->m_width, (grid)->m_height);                                       \
        RECT full = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                \
        if (!IntersectRect(&(grid)->m_bounds, &full, &clip)) {                                     \
            (grid)->m_bounds = full;                                                               \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

static inline i32 ScanCellX(CGrunt* g) {
    Coord t;
    g->GetScreenPos(&t);
    t.m_x >>= TILE_SHIFT_PX;
    t.m_y >>= TILE_SHIFT_PX;
    return t.m_x;
}

static inline i32 ScanCellY(CGrunt* g) {
    Coord t;
    g->GetScreenPos(&t);
    t.m_x >>= TILE_SHIFT_PX;
    t.m_y >>= TILE_SHIFT_PX;
    return t.m_y;
}

// @early-stop

RVA(0x00032ce0, 0x448)
i32 CBattlezMapConfig::ScanRegion(CGrunt* g) {
    if (g->m_stamina >= STAMINA_FULL) {
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
            if ((flags & 0x4000)
                && grid->m_rows[row][col].m_typeCode == TILEKIND_GAUNTLET_BRICK_C) {
                POSITION pos = g->m_coordList.GetHeadPosition();
                while (pos != NULL) {
                    void* coord = g->m_coordList.GetNext(pos);
                    if (coord != NULL) {
                        g_coordPool.Push(coord);
                    }
                }
                g->m_coordList.RemoveAll();
                return 1;
            }
        }
        if (g->m_dwell > static_cast<u32>(m_nearbyRouteSearchDelay) && g->CoordCount() == 0) {
            CMapMgr* grid = m_board;
            CRect box(
                ScanCellX(g) - 5,
                ScanCellY(g) - 5,
                ScanCellX(g) + 5,
                (g->m_object->m_screenY >> TILE_SHIFT_PX) + 5
            );
            CRect gb(0, 0, m_board->m_width, m_board->m_height);
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
                            } else if ((flags & 0x4000)
                                       && cell->m_typeCode != TILEKIND_GAUNTLET_BRICK_C) {
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
