#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>

#include <stddef.h>

// @early-stop
RVA(0x0006f2f0, 0x5227)
GruntDirectionCell __stdcall TmDeflectStep(
    CGrunt* g,
    i32 goalX,
    i32 goalY,
    i32 unusedX,
    i32 unusedY,
    GruntDirection dir,
    Coord* pCell,
    i32* pFlags
) {

    Coord entrance = g->EntrancePx();
    Coord goal(goalX, goalY);
    if (entrance == goal) {
        return s_gruntDirCenter;
    }
    Coord delta = goal - entrance;
    Coord distance = delta.GetAbs();
    Coord tile;
    i32 mask = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    Coord last = g->m_lastTilePx;
    i32 pass = g->m_passableMask;
    switch (dir) {
        case DIR_NORTH:
            if (entrance.m_x < goal.m_x) {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->m_tileGrid->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->m_tileGrid->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            } else if (entrance.m_x > goal.m_x) {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        case DIR_SOUTH:
            if (entrance.m_x < goal.m_x) {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            } else if (entrance.m_x > goal.m_x) {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
            } else {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
            }
            break;
        case DIR_EAST:
            if (entrance.m_y < goal.m_y) {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            } else if (entrance.m_y > goal.m_y) {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        case DIR_WEST:
            if (entrance.m_y < goal.m_y) {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->m_tileGrid->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            } else if (entrance.m_y > goal.m_y) {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
            } else {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
            }
            break;
        case DIR_NORTHEAST: {
            if (distance.m_x < distance.m_y) {
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
            } else if (distance.m_x > distance.m_y) {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->m_tileGrid->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);

                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
            }
            break;
        }
        case DIR_SOUTHEAST: {
            if (distance.m_x < distance.m_y) {
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else if (distance.m_x > distance.m_y) {
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
            }
            break;
        }
        case DIR_NORTHWEST: {
            if (distance.m_x < distance.m_y) {
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
            } else if (distance.m_x > distance.m_y) {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = last + Coord(0, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        }
        case DIR_SOUTHWEST: {
            if (distance.m_x < distance.m_y) {
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else if (distance.m_x > distance.m_y) {
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = last + Coord(0, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, 0);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = last + Coord(TILE_SIZE_PX, TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = last + Coord(-TILE_SIZE_PX, -TILE_SIZE_PX);
                }
                {
                    tile = *pCell;
                    ScreenTile(&tile);
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(tile.m_x, tile.m_y);
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = last + Coord(0, -TILE_SIZE_PX);
                        Coord sideXStep = last + Coord(-TILE_SIZE_PX, 0);
                        ScreenTile(&sideYStep);
                        ScreenTile(&sideXStep);
                        i32 sideYFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideYStep.m_x, sideYStep.m_y);
                        i32 sideXFlags =
                            g_gameReg->GetTileGrid()->CellFlagsAt(sideXStep.m_x, sideXStep.m_y);
                        if ((sideYFlags & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideXFlags & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return s_gruntDirCenter;
}

// @early-stop
RVA(0x00075af0, 0x111)
CGrunt* CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outPlayerIndex, i32* outUnitIndex, i32 exact) {
    Coord position(x, y);
    Coord tile = position;
    ScreenTile(&tile);
    CMapMgr* plane = g_gameReg->m_tileGrid;
    i32 attr;
    if (tile.m_x >= plane->m_width || tile.m_y >= plane->m_height) {
        attr = -1;
    } else {
        attr = plane->m_rows[tile.m_y][tile.m_x].m_occupantId;
    }
    if (attr == -1) {
        return NULL;
    }
    i32 playerIndex = (attr >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
    i32 unitIndex = attr & GRUNT_IDENTITY_COMPONENT_MASK;
    CGrunt* cell = m_units[unitIndex + playerIndex * TM_UNITS_PER_PLAYER];
    if (cell == NULL || cell->m_entranceCommitted == false) {
        return NULL;
    }

    if (exact == 0) {
        CGameObject* o = cell->m_object;
        RECT box;
        SetRect(&box, position.m_x - 7, position.m_y - 7, position.m_x + 7, position.m_y + 7);
        Coord objectLo = o->ScreenPos();
        objectLo -= Coord(7, 7);
        if (box.left > objectLo.m_x + 14 || box.right < objectLo.m_x || box.top > objectLo.m_y + 14
            || box.bottom < objectLo.m_y) {
            return NULL;
        }
        *outPlayerIndex = playerIndex;
        *outUnitIndex = unitIndex;
        return cell;
    }
    CGameObject* o = cell->m_object;
    if (o->ScreenPos() != position) {
        return NULL;
    }
    *outPlayerIndex = playerIndex;
    *outUnitIndex = unitIndex;
    return cell;
}

// @early-stop
RVA(0x00075c60, 0x1ba)
CGrunt* CTriggerMgr::FindGruntAt(
    i32 px,
    i32 py,
    RECT* span,
    i32* outPlayerIndex,
    i32* outUnitIndex,
    RECT* src
) {
    Coord position(px, py);
    Coord tile = position;
    ScreenTile(&tile);
    CRect rc;
    if (src) {
        rc = *src;
    } else {
        Coord nearExtent(span->left, span->top);
        nearExtent *= TILE_SIZE_PX;
        Coord farExtent(span->right, span->bottom);
        farExtent *= TILE_SIZE_PX;
        Coord margin(7, 7);
        Coord low = position - nearExtent - margin;
        Coord high = position + farExtent + margin;
        rc.SetRect(low.m_x, low.m_y, high.m_x, high.m_y);
    }
    i32 x = tile.m_x - span->left - 1;
    i32 xEnd = span->right + tile.m_x + 1;

    if (static_cast<u32>(x) <= static_cast<u32>(xEnd)) {
        do {
            i32 yEnd = span->bottom + tile.m_y + 1;
            for (i32 y = tile.m_y - span->top - 1; static_cast<u32>(y) <= static_cast<u32>(yEnd);
                 y++) {
                if (static_cast<u32>(x) >= static_cast<u32>(g_gameReg->m_tileGrid->m_width)) {
                    continue;
                }
                if (static_cast<u32>(y) >= static_cast<u32>(g_gameReg->m_tileGrid->m_height)) {
                    continue;
                }
                CMapMgr* grid = g_gameReg->m_tileGrid;
                i32 val;
                if (static_cast<u32>(x) < static_cast<u32>(grid->m_width)
                    && static_cast<u32>(y) < static_cast<u32>(grid->m_height)) {
                    val = grid->m_rows[y][x].m_occupantId;
                } else {
                    val = -1;
                }
                if (val == -1) {
                    continue;
                }
                i32 playerIndex =
                    (val >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
                i32 unitIndex = val & GRUNT_IDENTITY_COMPONENT_MASK;
                CGrunt* g = m_units[unitIndex + playerIndex * TM_UNITS_PER_PLAYER];
                if (!g) {
                    continue;
                }
                if (!g->m_entranceCommitted) {
                    continue;
                }
                Coord spriteLo = g->m_object->ScreenPos();
                spriteLo -= Coord(7, 7);
                Coord spriteHi = spriteLo + Coord(0xe, 0xe);
                if (rc.left <= spriteHi.m_x && rc.right >= spriteLo.m_x && rc.top <= spriteHi.m_y
                    && rc.bottom >= spriteLo.m_y) {
                    *outPlayerIndex = playerIndex;
                    *outUnitIndex = unitIndex;
                    return g;
                }
            }
            x++;
        } while (static_cast<u32>(x) <= static_cast<u32>(xEnd));
    }
    return NULL;
}
