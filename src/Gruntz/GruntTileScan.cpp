#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>
#include <RectMacros.h>
#include <Wap32/TileGeometry.h>

#include <stdlib.h>

// @early-stop
RVA(0x00032ce0, 0x448)
i32 CBattlezMapConfig::ScanRegion(CGrunt* g) {
    if (g->m_stamina >= STAMINA_FULL) {
        if (g->CoordCount() != 0) {
            Coord* c = g->GetTailCoord();
            i32 col = c->m_x;
            i32 row = c->m_y;
            CMapMgr* grid = m_board;
            i32 flags = grid->CellFlagsAt(col, row);
            if ((flags & IDX(CELL_FLAG_GAUNTLET_BRICK))
                && grid->m_rows[row][col].m_typeCode == TILEKIND_GAUNTLET_BRICK_C) {
                RECYCLE_GRUNT_COORDS(g)
                return 1;
            }
        }
        if (g->m_dwell > static_cast<u32>(m_nearbyRouteSearchDelay) && g->CoordCount() == 0) {
            CMapMgr* grid = m_board;
            RECT box;
            SET_RECT_COMPONENTS(
                box,
                g->ScanCell().m_x - 5,
                g->ScanCell().m_y - 5,
                g->ScanCell().m_x + 5,
                (g->m_object->m_screenY >> TILE_SHIFT_PX) + 5
            );
            RECT gb;
            SET_RECT_COMPONENTS(gb, 0, 0, m_board->m_width, m_board->m_height);
            RECT isect;
            if (IntersectRect(&isect, &box, &gb)) {
                u32 hits = 0;
                for (i32 row = isect.top; row < isect.bottom; row++) {
                    if (hits > 4) {
                        break;
                    }
                    BrickzCell* cell = &grid->m_rows[row][isect.left];
                    for (i32 col = isect.left; col < isect.right; col++) {
                        if (hits < 5) {
                            i32 flags = cell->m_flags;
                            if (flags & IDX(CELL_FLAG_HIDDEN_POWERUP)) {
                                if (RouteUnitTo(g, col, row, 0xd87, 0, 0)) {
                                    SCAN_BOUNDS_PLAINCLIP(grid);
                                    return 1;
                                }
                                hits++;
                            } else if ((flags & IDX(CELL_FLAG_GAUNTLET_BRICK))
                                       && cell->m_typeCode != TILEKIND_GAUNTLET_BRICK_C) {
                                if (RouteUnitTo(g, col, row, 0xd87, 0, 0)) {
                                    SCAN_BOUNDS_PLAINCLIP(grid);
                                    return 1;
                                }
                                hits++;
                            }
                        }
                        cell++;
                    }
                }
            }
            {
                GRID_CLIP_NULL(grid);
            }
            if (m_attackWaypoints.GetSize() != 0) {

                Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
                g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            }
            g->m_dwell = 0;
        }
    }
    return 1;
}
