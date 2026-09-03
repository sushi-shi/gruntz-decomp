#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Wap32/TileGeometry.h>

#include <stdlib.h>

static inline Coord ScanCell(CGrunt* g) {
    Coord t;
    g->GetScreenTile(&t);
    return t;
}

// @early-stop
RVA(0x00032ce0, 0x448)
i32 CBattlezMapConfig::ScanRegion(CGrunt* g) {
    if (g->m_stamina >= STAMINA_FULL) {
        if (g->CoordCount() != 0) {
            Coord cell = *static_cast<Coord*>(g->m_coordList.GetTail());
            CMapMgr* grid = m_board;
            i32 flags = grid->CellFlagsAt(cell.m_x, cell.m_y);
            if ((flags & IDX(CELL_FLAG_GAUNTLET_BRICK))
                && grid->m_rows[cell.m_y][cell.m_x].m_typeCode == TILEKIND_GAUNTLET_BRICK_C) {
                RECYCLE_GRUNT_COORDS(g)
                return 1;
            }
        }
        if (g->m_dwell > static_cast<u32>(m_nearbyRouteSearchDelay) && g->CoordCount() == 0) {
            CMapMgr* grid = m_board;
            Coord scanCell = ScanCell(g);
            CRect box(scanCell.m_x - 5, scanCell.m_y - 5, scanCell.m_x + 5, scanCell.m_y + 5);
            CRect gridBounds(0, 0, m_board->m_width, m_board->m_height);
            CRect isect;
            if (isect.IntersectRect(&box, &gridBounds)) {
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
                                if (RouteUnitTo(
                                        g,
                                        col,
                                        row,
                                        IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER
                                            | CELL_FLAG_ARROW | CELL_FLAG_WATER | CELL_FLAG_SPIKES
                                            | CELL_FLAG_SINK_HAZARD),
                                        0,
                                        0
                                    )) {
                                    grid->Clip(NULL);
                                    return 1;
                                }
                                hits++;
                            } else if ((flags & IDX(CELL_FLAG_GAUNTLET_BRICK))
                                       && cell->m_typeCode != TILEKIND_GAUNTLET_BRICK_C) {
                                if (RouteUnitTo(
                                        g,
                                        col,
                                        row,
                                        IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER
                                            | CELL_FLAG_ARROW | CELL_FLAG_WATER | CELL_FLAG_SPIKES
                                            | CELL_FLAG_SINK_HAZARD),
                                        0,
                                        0
                                    )) {
                                    grid->Clip(NULL);
                                    return 1;
                                }
                                hits++;
                            }
                        }
                        cell++;
                    }
                }
            }
            grid->Clip(NULL);
            if (m_attackWaypoints.GetSize() != 0) {

                Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
                g->TileSwitch(
                    e->m_x,
                    e->m_y,
                    0,
                    IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_ARROW | CELL_FLAG_WATER
                        | CELL_FLAG_SINK_HAZARD),
                    0,
                    0
                );
            }
            g->m_dwell = 0;
        }
    }
    return 1;
}
