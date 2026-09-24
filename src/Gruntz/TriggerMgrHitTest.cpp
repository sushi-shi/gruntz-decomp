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
    Coord northEastGoalNorthWestStep;
    Coord northEastGoalWestStep;
    Coord northEastGoalNorthEastStep;
    Coord northEastGoalEastStep;
    Coord northWestGoalNorthEastStep;
    Coord northWestGoalEastStep;
    Coord northWestGoalNorthWestStep;
    Coord northWestGoalWestStep;
    Coord northAlignedGoalNorthWestStep;
    Coord northAlignedGoalNorthEastStep;
    Coord northAlignedGoalWestStep;
    Coord northAlignedGoalEastStep;
    Coord southEastGoalSouthWestStep;
    Coord southEastGoalWestStep;
    Coord southEastGoalSouthEastStep;
    Coord southEastGoalEastStep;
    Coord southWestGoalSouthEastStep;
    Coord southWestGoalEastStep;
    Coord southWestGoalSouthWestStep;
    Coord southWestGoalWestStep;
    Coord southAlignedGoalSouthWestStep;
    Coord southAlignedGoalSouthEastStep;
    Coord southAlignedGoalWestStep;
    Coord southAlignedGoalEastStep;
    Coord eastSouthGoalNorthEastStep;
    Coord eastSouthGoalNorthStep;
    Coord eastSouthGoalSouthEastStep;
    Coord eastSouthGoalSouthStep;
    Coord eastNorthGoalSouthEastStep;
    Coord eastNorthGoalSouthStep;
    Coord eastNorthGoalNorthEastStep;
    Coord eastNorthGoalNorthStep;
    Coord eastAlignedGoalNorthEastStep;
    Coord eastAlignedGoalSouthEastStep;
    Coord eastAlignedGoalNorthStep;
    Coord eastAlignedGoalSouthStep;
    Coord westSouthGoalNorthWestStep;
    Coord westSouthGoalNorthStep;
    Coord westSouthGoalSouthWestStep;
    Coord westSouthGoalSouthStep;
    Coord westNorthGoalSouthWestStep;
    Coord westNorthGoalSouthStep;
    Coord westNorthGoalNorthWestStep;
    Coord westNorthGoalNorthStep;
    Coord westAlignedGoalNorthWestStep;
    Coord westAlignedGoalSouthWestStep;
    Coord westAlignedGoalNorthStep;
    Coord westAlignedGoalSouthStep;
    Coord northEastXDeltaLessNorthStep;
    Coord northEastXDeltaLessNorthWestStep;
    Coord northEastXDeltaLessEastStep;
    Coord northEastXDeltaLessSouthEastStep;
    Coord northEastXDeltaGreaterEastStep;
    Coord northEastXDeltaGreaterSouthEastStep;
    Coord northEastXDeltaGreaterNorthStep;
    Coord northEastXDeltaGreaterNorthWestStep;
    Coord northEastEqualDeltasNorthStep;
    Coord northEastEqualDeltasEastStep;
    Coord northEastEqualDeltasNorthWestStep;
    Coord northEastEqualDeltasSouthEastStep;
    Coord southEastXDeltaLessSouthStep;
    Coord southEastXDeltaLessSouthWestStep;
    Coord southEastXDeltaLessEastStep;
    Coord southEastXDeltaLessNorthEastStep;
    Coord southEastXDeltaGreaterEastStep;
    Coord southEastXDeltaGreaterNorthEastStep;
    Coord southEastXDeltaGreaterSouthStep;
    Coord southEastXDeltaGreaterSouthWestStep;
    Coord southEastEqualDeltasSouthStep;
    Coord southEastEqualDeltasEastStep;
    Coord southEastEqualDeltasSouthWestStep;
    Coord southEastEqualDeltasNorthEastStep;
    Coord northWestXDeltaLessNorthStep;
    Coord northWestXDeltaLessNorthEastStep;
    Coord northWestXDeltaLessWestStep;
    Coord northWestXDeltaLessSouthWestStep;
    Coord northWestXDeltaGreaterWestStep;
    Coord northWestXDeltaGreaterSouthWestStep;
    Coord northWestXDeltaGreaterNorthStep;
    Coord northWestXDeltaGreaterNorthEastStep;
    Coord northWestEqualDeltasNorthStep;
    Coord northWestEqualDeltasWestStep;
    Coord northWestEqualDeltasNorthEastStep;
    Coord northWestEqualDeltasSouthWestStep;
    Coord southWestXDeltaLessSouthStep;
    Coord southWestXDeltaLessSouthEastStep;
    Coord southWestXDeltaLessWestStep;
    Coord southWestXDeltaLessNorthWestStep;
    Coord southWestXDeltaGreaterWestStep;
    Coord southWestXDeltaGreaterNorthWestStep;
    Coord southWestXDeltaGreaterSouthStep;
    Coord southWestXDeltaGreaterSouthEastStep;
    Coord southWestEqualDeltasSouthStep;
    Coord southWestEqualDeltasWestStep;
    Coord southWestEqualDeltasSouthEastStep;
    Coord southWestEqualDeltasNorthWestStep;
    Coord northEastGoalNorthWestNorthSide;
    Coord northEastGoalNorthWestWestSide;
    Coord northEastGoalNorthEastNorthSide;
    Coord northEastGoalNorthEastEastSide;
    Coord northWestGoalNorthEastNorthSide;
    Coord northWestGoalNorthEastEastSide;
    Coord northWestGoalNorthWestNorthSide;
    Coord northWestGoalNorthWestWestSide;
    Coord northAlignedGoalNorthWestNorthSide;
    Coord northAlignedGoalNorthWestWestSide;
    Coord northAlignedGoalNorthEastNorthSide;
    Coord northAlignedGoalNorthEastEastSide;
    Coord southEastGoalSouthWestSouthSide;
    Coord southEastGoalSouthWestWestSide;
    Coord southEastGoalSouthEastSouthSide;
    Coord southEastGoalSouthEastEastSide;
    Coord southWestGoalSouthEastSouthSide;
    Coord southWestGoalSouthEastEastSide;
    Coord southWestGoalSouthWestSouthSide;
    Coord southWestGoalSouthWestWestSide;
    Coord southAlignedGoalSouthWestSouthSide;
    Coord southAlignedGoalSouthWestWestSide;
    Coord southAlignedGoalSouthEastSouthSide;
    Coord southAlignedGoalSouthEastEastSide;
    Coord eastSouthGoalNorthEastNorthSide;
    Coord eastSouthGoalNorthEastEastSide;
    Coord eastSouthGoalSouthEastSouthSide;
    Coord eastSouthGoalSouthEastEastSide;
    Coord eastNorthGoalSouthEastSouthSide;
    Coord eastNorthGoalSouthEastEastSide;
    Coord eastNorthGoalNorthEastNorthSide;
    Coord eastNorthGoalNorthEastEastSide;
    Coord eastAlignedGoalNorthEastNorthSide;
    Coord eastAlignedGoalNorthEastEastSide;
    Coord eastAlignedGoalSouthEastSouthSide;
    Coord eastAlignedGoalSouthEastEastSide;
    Coord westSouthGoalNorthWestNorthSide;
    Coord westSouthGoalNorthWestWestSide;
    Coord westSouthGoalSouthWestSouthSide;
    Coord westSouthGoalSouthWestWestSide;
    Coord westNorthGoalSouthWestSouthSide;
    Coord westNorthGoalSouthWestWestSide;
    Coord westNorthGoalNorthWestNorthSide;
    Coord westNorthGoalNorthWestWestSide;
    Coord westAlignedGoalNorthWestNorthSide;
    Coord westAlignedGoalNorthWestWestSide;
    Coord westAlignedGoalSouthWestSouthSide;
    Coord westAlignedGoalSouthWestWestSide;
    Coord northEastXDeltaLessNorthWestNorthSide;
    Coord northEastXDeltaLessNorthWestWestSide;
    Coord northEastXDeltaLessSouthEastSouthSide;
    Coord northEastXDeltaLessSouthEastEastSide;
    Coord northEastXDeltaGreaterSouthEastSouthSide;
    Coord northEastXDeltaGreaterSouthEastEastSide;
    Coord northEastXDeltaGreaterNorthWestNorthSide;
    Coord northEastXDeltaGreaterNorthWestWestSide;
    Coord northEastEqualDeltasNorthWestNorthSide;
    Coord northEastEqualDeltasNorthWestWestSide;
    Coord northEastEqualDeltasSouthEastSouthSide;
    Coord northEastEqualDeltasSouthEastEastSide;
    Coord southEastXDeltaLessSouthWestSouthSide;
    Coord southEastXDeltaLessSouthWestWestSide;
    Coord southEastXDeltaLessNorthEastNorthSide;
    Coord southEastXDeltaLessNorthEastEastSide;
    Coord southEastXDeltaGreaterNorthEastNorthSide;
    Coord southEastXDeltaGreaterNorthEastEastSide;
    Coord southEastXDeltaGreaterSouthWestSouthSide;
    Coord southEastXDeltaGreaterSouthWestWestSide;
    Coord southEastEqualDeltasSouthWestSouthSide;
    Coord southEastEqualDeltasSouthWestWestSide;
    Coord southEastEqualDeltasNorthEastNorthSide;
    Coord southEastEqualDeltasNorthEastEastSide;
    Coord northWestXDeltaLessNorthEastNorthSide;
    Coord northWestXDeltaLessNorthEastEastSide;
    Coord northWestXDeltaLessSouthWestSouthSide;
    Coord northWestXDeltaLessSouthWestWestSide;
    Coord northWestXDeltaGreaterSouthWestSouthSide;
    Coord northWestXDeltaGreaterSouthWestWestSide;
    Coord northWestXDeltaGreaterNorthEastNorthSide;
    Coord northWestXDeltaGreaterNorthEastEastSide;
    Coord northWestEqualDeltasNorthEastNorthSide;
    Coord northWestEqualDeltasNorthEastEastSide;
    Coord northWestEqualDeltasSouthWestSouthSide;
    Coord northWestEqualDeltasSouthWestWestSide;
    Coord southWestXDeltaLessSouthEastSouthSide;
    Coord southWestXDeltaLessSouthEastEastSide;
    Coord southWestXDeltaLessNorthWestNorthSide;
    Coord southWestXDeltaLessNorthWestWestSide;
    Coord southWestXDeltaGreaterNorthWestNorthSide;
    Coord southWestXDeltaGreaterNorthWestWestSide;
    Coord southWestXDeltaGreaterSouthEastSouthSide;
    Coord southWestXDeltaGreaterSouthEastEastSide;
    Coord southWestEqualDeltasSouthEastSouthSide;
    Coord southWestEqualDeltasSouthEastEastSide;
    Coord southWestEqualDeltasNorthWestNorthSide;
    Coord southWestEqualDeltasNorthWestWestSide;

    i32 sideY;
    Coord entrance = g->EntrancePx();
    if (COORD_EQUALS_COMPONENTS(entrance, goalX, goalY)) {
        return s_gruntDirCenter;
    }
    i32 mask = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    i32 lastX = g->m_lastTilePx.m_x;
    i32 lastY = g->m_lastTilePx.m_y;
    i32 pass = g->m_passableMask;
    switch (dir) {
        case DIR_NORTH:
            if (g->EntrancePx().m_x < goalX) {
                {
                    *pCell =
                        *northEastGoalNorthWestStep.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastGoalNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastGoalNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->m_tileGrid->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastGoalNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastGoalNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *northEastGoalWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell =
                        *northEastGoalNorthEastStep.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->m_tileGrid->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastGoalNorthEastNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastGoalNorthEastNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastGoalNorthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastGoalNorthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *northEastGoalEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_x > goalX) {
                {
                    *pCell =
                        *northWestGoalNorthEastStep.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northWestGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northWestGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northWestGoalNorthEastNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northWestGoalNorthEastNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northWestGoalNorthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(northWestGoalNorthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *northWestGoalEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell =
                        *northWestGoalNorthWestStep.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northWestGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northWestGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northWestGoalNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northWestGoalNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northWestGoalNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(northWestGoalNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *northWestGoalWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *northAlignedGoalNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northAlignedGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northAlignedGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *northAlignedGoalNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northAlignedGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northAlignedGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthEastNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthEastNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(northAlignedGoalNorthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *northAlignedGoalWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *northAlignedGoalEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        case DIR_SOUTH:
            if (g->EntrancePx().m_x < goalX) {
                {
                    *pCell =
                        *southEastGoalSouthWestStep.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southEastGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southEastGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southEastGoalSouthWestSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(southEastGoalSouthWestSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southEastGoalSouthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(southEastGoalSouthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *southEastGoalWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell =
                        *southEastGoalSouthEastStep.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southEastGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southEastGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southEastGoalSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(southEastGoalSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southEastGoalSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(southEastGoalSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *southEastGoalEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_x > goalX) {
                {
                    *pCell =
                        *southWestGoalSouthEastStep.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southWestGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southWestGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southWestGoalSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(southWestGoalSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southWestGoalSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(southWestGoalSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *southWestGoalEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell =
                        *southWestGoalSouthWestStep.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southWestGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southWestGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southWestGoalSouthWestSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(southWestGoalSouthWestSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southWestGoalSouthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(southWestGoalSouthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *southWestGoalWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
            } else {
                {
                    *pCell = *southAlignedGoalSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southAlignedGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southAlignedGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthWestSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthWestSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *southAlignedGoalSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southAlignedGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southAlignedGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(southAlignedGoalSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *southAlignedGoalWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *southAlignedGoalEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
            }
            break;
        case DIR_EAST:
            if (g->EntrancePx().m_y < goalY) {
                {
                    *pCell =
                        *eastSouthGoalNorthEastStep.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastSouthGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        eastSouthGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastSouthGoalNorthEastNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(eastSouthGoalNorthEastNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastSouthGoalNorthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(eastSouthGoalNorthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *eastSouthGoalNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell =
                        *eastSouthGoalSouthEastStep.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastSouthGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        eastSouthGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastSouthGoalSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(eastSouthGoalSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastSouthGoalSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(eastSouthGoalSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *eastSouthGoalSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_y > goalY) {
                {
                    *pCell =
                        *eastNorthGoalSouthEastStep.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastNorthGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        eastNorthGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastNorthGoalSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(eastNorthGoalSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastNorthGoalSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(eastNorthGoalSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *eastNorthGoalSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell =
                        *eastNorthGoalNorthEastStep.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastNorthGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        eastNorthGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastNorthGoalNorthEastNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(eastNorthGoalNorthEastNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastNorthGoalNorthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(eastNorthGoalNorthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *eastNorthGoalNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *eastAlignedGoalNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastAlignedGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        eastAlignedGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastAlignedGoalNorthEastNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(eastAlignedGoalNorthEastNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastAlignedGoalNorthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(eastAlignedGoalNorthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *eastAlignedGoalSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastAlignedGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        eastAlignedGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastAlignedGoalSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(eastAlignedGoalSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(eastAlignedGoalSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(eastAlignedGoalSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *eastAlignedGoalNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *eastAlignedGoalSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        case DIR_WEST:
            if (g->EntrancePx().m_y < goalY) {
                {
                    *pCell =
                        *westSouthGoalNorthWestStep.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westSouthGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        westSouthGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westSouthGoalNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(westSouthGoalNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westSouthGoalNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(westSouthGoalNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *westSouthGoalNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell =
                        *westSouthGoalSouthWestStep.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westSouthGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        westSouthGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westSouthGoalSouthWestSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(westSouthGoalSouthWestSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->m_tileGrid->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westSouthGoalSouthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(westSouthGoalSouthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *westSouthGoalSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_y > goalY) {
                {
                    *pCell =
                        *westNorthGoalSouthWestStep.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westNorthGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        westNorthGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westNorthGoalSouthWestSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(westNorthGoalSouthWestSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westNorthGoalSouthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(westNorthGoalSouthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *westNorthGoalSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell =
                        *westNorthGoalNorthWestStep.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westNorthGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        westNorthGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westNorthGoalNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(westNorthGoalNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westNorthGoalNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(westNorthGoalNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *westNorthGoalNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
            } else {
                {
                    *pCell = *westAlignedGoalNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westAlignedGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        westAlignedGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westAlignedGoalNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(westAlignedGoalNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westAlignedGoalNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(westAlignedGoalNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *westAlignedGoalSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westAlignedGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        westAlignedGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westAlignedGoalSouthWestSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(westAlignedGoalSouthWestSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(westAlignedGoalSouthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(westAlignedGoalSouthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *westAlignedGoalNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *westAlignedGoalSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
            }
            break;
        case DIR_NORTHEAST: {
            i32 deltaY = goalY - g->EntrancePx().m_y;
            i32 deltaX = g->EntrancePx().m_x - goalX;
            if (deltaX < deltaY) {
                {
                    *pCell = *northEastXDeltaLessNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *northEastXDeltaLessNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaLessNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastXDeltaLessNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *northEastXDeltaLessEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *northEastXDeltaLessSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaLessSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        northEastXDeltaLessSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaLessSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
            } else if (deltaX > deltaY) {
                {
                    *pCell = *northEastXDeltaGreaterEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *northEastXDeltaGreaterSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaGreaterSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        northEastXDeltaGreaterSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterSouthEastSouthSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterSouthEastSouthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterSouthEastEastSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterSouthEastEastSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *northEastXDeltaGreaterNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *northEastXDeltaGreaterNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaGreaterNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastXDeltaGreaterNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterNorthWestNorthSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterNorthWestNorthSide.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterNorthWestWestSide.m_x),
                            SCREEN_TILE_COMPONENT(northEastXDeltaGreaterNorthWestWestSide.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *northEastEqualDeltasNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *northEastEqualDeltasEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *northEastEqualDeltasNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord* sideYStep = northEastEqualDeltasNorthWestNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord* sideXStep = northEastEqualDeltasNorthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        i32 sideY = g_gameReg->m_tileGrid->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep->m_x),
                            SCREEN_TILE_COMPONENT(sideYStep->m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep->m_x),
                            SCREEN_TILE_COMPONENT(sideXStep->m_y)
                        );

                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *northEastEqualDeltasSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northEastEqualDeltasSouthEastSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northEastEqualDeltasSouthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
            }
            break;
        }
        case DIR_SOUTHEAST: {
            i32 deltaY = g->EntrancePx().m_y - goalY;
            i32 deltaX = g->EntrancePx().m_x - goalX;
            if (deltaX < deltaY) {
                {
                    *pCell = *southEastXDeltaLessSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *southEastXDeltaLessSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southEastXDeltaLessSouthWestSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southEastXDeltaLessSouthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *southEastXDeltaLessEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *southEastXDeltaLessNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southEastXDeltaLessNorthEastNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southEastXDeltaLessNorthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else if (deltaX > deltaY) {
                {
                    *pCell = *southEastXDeltaGreaterEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *southEastXDeltaGreaterNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southEastXDeltaGreaterNorthEastNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southEastXDeltaGreaterNorthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *southEastXDeltaGreaterSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *southEastXDeltaGreaterSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southEastXDeltaGreaterSouthWestSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southEastXDeltaGreaterSouthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *southEastEqualDeltasSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *southEastEqualDeltasEastStep.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *southEastEqualDeltasSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southEastEqualDeltasSouthWestSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southEastEqualDeltasSouthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *southEastEqualDeltasNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southEastEqualDeltasNorthEastNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southEastEqualDeltasNorthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
            }
            break;
        }
        case DIR_NORTHWEST: {
            i32 deltaY = goalY - g->EntrancePx().m_y;
            i32 deltaX = goalX - g->EntrancePx().m_x;
            if (deltaX < deltaY) {
                {
                    *pCell = *northWestXDeltaLessNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *northWestXDeltaLessNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northWestXDeltaLessNorthEastNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northWestXDeltaLessNorthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *northWestXDeltaLessWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *northWestXDeltaLessSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northWestXDeltaLessSouthWestSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northWestXDeltaLessSouthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
            } else if (deltaX > deltaY) {
                {
                    *pCell = *northWestXDeltaGreaterWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *northWestXDeltaGreaterSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northWestXDeltaGreaterSouthWestSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northWestXDeltaGreaterSouthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *northWestXDeltaGreaterNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *northWestXDeltaGreaterNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northWestXDeltaGreaterNorthEastNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northWestXDeltaGreaterNorthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *northWestEqualDeltasNorthStep.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *northWestEqualDeltasWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *northWestEqualDeltasNorthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northWestEqualDeltasNorthEastNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northWestEqualDeltasNorthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *northWestEqualDeltasSouthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *northWestEqualDeltasSouthWestSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *northWestEqualDeltasSouthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        }
        case DIR_SOUTHWEST: {
            i32 deltaY = g->EntrancePx().m_y - goalY;
            i32 deltaX = goalX - g->EntrancePx().m_x;
            if (deltaX < deltaY) {
                {
                    *pCell = *southWestXDeltaLessSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *southWestXDeltaLessSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southWestXDeltaLessSouthEastSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southWestXDeltaLessSouthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *southWestXDeltaLessWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *southWestXDeltaLessNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southWestXDeltaLessNorthWestNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southWestXDeltaLessNorthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else if (deltaX > deltaY) {
                {
                    *pCell = *southWestXDeltaGreaterWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *southWestXDeltaGreaterNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southWestXDeltaGreaterNorthWestNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southWestXDeltaGreaterNorthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *southWestXDeltaGreaterSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *southWestXDeltaGreaterSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southWestXDeltaGreaterSouthEastSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southWestXDeltaGreaterSouthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *southWestEqualDeltasSouthStep.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *southWestEqualDeltasWestStep.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *southWestEqualDeltasSouthEastStep.Set(
                        lastX + TILE_SIZE_PX,
                        lastY + TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southWestEqualDeltasSouthEastSouthSide.Set(
                            lastX,
                            (lastY + TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southWestEqualDeltasSouthEastEastSide.Set(
                            (lastX + TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *southWestEqualDeltasNorthWestStep.Set(
                        lastX - TILE_SIZE_PX,
                        lastY - TILE_SIZE_PX
                    );
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        SCREEN_TILE_COMPONENT(pCell->m_x),
                        SCREEN_TILE_COMPONENT(pCell->m_y)
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *southWestEqualDeltasNorthWestNorthSide.Set(
                            lastX,
                            (lastY - TILE_SIZE_PX)
                        );
                        Coord sideXStep = *southWestEqualDeltasNorthWestWestSide.Set(
                            (lastX - TILE_SIZE_PX),
                            lastY
                        );
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideYStep.m_x),
                            SCREEN_TILE_COMPONENT(sideYStep.m_y)
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            SCREEN_TILE_COMPONENT(sideXStep.m_x),
                            SCREEN_TILE_COMPONENT(sideXStep.m_y)
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
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
        CRect box(position.m_x - 7, position.m_y - 7, position.m_x + 7, position.m_y + 7);
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
    CRect tileBounds(
        tile.m_x - span->left - 1,
        tile.m_y - span->top - 1,
        span->right + tile.m_x + 1,
        span->bottom + tile.m_y + 1
    );
    i32 x = tileBounds.left;

    if (static_cast<u32>(x) <= static_cast<u32>(tileBounds.right)) {
        do {
            for (i32 y = tileBounds.top; static_cast<u32>(y) <= static_cast<u32>(tileBounds.bottom);
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
        } while (static_cast<u32>(x) <= static_cast<u32>(tileBounds.right));
    }
    return NULL;
}
