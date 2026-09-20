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
    if (entrance.m_x == goalX && entrance.m_y == goalY) {
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastGoalNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            northEastGoalNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->m_tileGrid->CellFlagsAt(
                            northEastGoalNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            northEastGoalNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastGoalNorthEastNorthSide.m_x >> TILE_SHIFT_PX,
                            northEastGoalNorthEastNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastGoalNorthEastEastSide.m_x >> TILE_SHIFT_PX,
                            northEastGoalNorthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northWestGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northWestGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northWestGoalNorthEastNorthSide.m_x >> TILE_SHIFT_PX,
                            northWestGoalNorthEastNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northWestGoalNorthEastEastSide.m_x >> TILE_SHIFT_PX,
                            northWestGoalNorthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northWestGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northWestGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northWestGoalNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            northWestGoalNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northWestGoalNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            northWestGoalNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northAlignedGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northAlignedGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northAlignedGoalNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            northAlignedGoalNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northAlignedGoalNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            northAlignedGoalNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northAlignedGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northAlignedGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northAlignedGoalNorthEastNorthSide.m_x >> TILE_SHIFT_PX,
                            northAlignedGoalNorthEastNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northAlignedGoalNorthEastEastSide.m_x >> TILE_SHIFT_PX,
                            northAlignedGoalNorthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southEastGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southEastGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southEastGoalSouthWestSouthSide.m_x >> TILE_SHIFT_PX,
                            southEastGoalSouthWestSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southEastGoalSouthWestWestSide.m_x >> TILE_SHIFT_PX,
                            southEastGoalSouthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southEastGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southEastGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southEastGoalSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            southEastGoalSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southEastGoalSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            southEastGoalSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southWestGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southWestGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southWestGoalSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            southWestGoalSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southWestGoalSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            southWestGoalSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southWestGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southWestGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southWestGoalSouthWestSouthSide.m_x >> TILE_SHIFT_PX,
                            southWestGoalSouthWestSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southWestGoalSouthWestWestSide.m_x >> TILE_SHIFT_PX,
                            southWestGoalSouthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southAlignedGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southAlignedGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southAlignedGoalSouthWestSouthSide.m_x >> TILE_SHIFT_PX,
                            southAlignedGoalSouthWestSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southAlignedGoalSouthWestWestSide.m_x >> TILE_SHIFT_PX,
                            southAlignedGoalSouthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        southAlignedGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        southAlignedGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southAlignedGoalSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            southAlignedGoalSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            southAlignedGoalSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            southAlignedGoalSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastSouthGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        eastSouthGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastSouthGoalNorthEastNorthSide.m_x >> TILE_SHIFT_PX,
                            eastSouthGoalNorthEastNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastSouthGoalNorthEastEastSide.m_x >> TILE_SHIFT_PX,
                            eastSouthGoalNorthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastSouthGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        eastSouthGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastSouthGoalSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            eastSouthGoalSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastSouthGoalSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            eastSouthGoalSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastNorthGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        eastNorthGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastNorthGoalSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            eastNorthGoalSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastNorthGoalSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            eastNorthGoalSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastNorthGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        eastNorthGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastNorthGoalNorthEastNorthSide.m_x >> TILE_SHIFT_PX,
                            eastNorthGoalNorthEastNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastNorthGoalNorthEastEastSide.m_x >> TILE_SHIFT_PX,
                            eastNorthGoalNorthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastAlignedGoalNorthEastNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        eastAlignedGoalNorthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastAlignedGoalNorthEastNorthSide.m_x >> TILE_SHIFT_PX,
                            eastAlignedGoalNorthEastNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastAlignedGoalNorthEastEastSide.m_x >> TILE_SHIFT_PX,
                            eastAlignedGoalNorthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        eastAlignedGoalSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        eastAlignedGoalSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastAlignedGoalSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            eastAlignedGoalSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            eastAlignedGoalSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            eastAlignedGoalSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westSouthGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        westSouthGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westSouthGoalNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            westSouthGoalNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westSouthGoalNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            westSouthGoalNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westSouthGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        westSouthGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westSouthGoalSouthWestSouthSide.m_x >> TILE_SHIFT_PX,
                            westSouthGoalSouthWestSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->m_tileGrid->CellFlagsAt(
                            westSouthGoalSouthWestWestSide.m_x >> TILE_SHIFT_PX,
                            westSouthGoalSouthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westNorthGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        westNorthGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westNorthGoalSouthWestSouthSide.m_x >> TILE_SHIFT_PX,
                            westNorthGoalSouthWestSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westNorthGoalSouthWestWestSide.m_x >> TILE_SHIFT_PX,
                            westNorthGoalSouthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westNorthGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        westNorthGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westNorthGoalNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            westNorthGoalNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westNorthGoalNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            westNorthGoalNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westAlignedGoalNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        westAlignedGoalNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westAlignedGoalNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            westAlignedGoalNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westAlignedGoalNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            westAlignedGoalNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        westAlignedGoalSouthWestSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        westAlignedGoalSouthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westAlignedGoalSouthWestSouthSide.m_x >> TILE_SHIFT_PX,
                            westAlignedGoalSouthWestSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            westAlignedGoalSouthWestWestSide.m_x >> TILE_SHIFT_PX,
                            westAlignedGoalSouthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaLessNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastXDeltaLessNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaLessNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaLessNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaLessNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaLessNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaLessSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        northEastXDeltaLessSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaLessSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaLessSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaLessSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaLessSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaGreaterSouthEastSouthSide.Set(lastX, (lastY + TILE_SIZE_PX));
                        northEastXDeltaGreaterSouthEastEastSide.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaGreaterSouthEastSouthSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaGreaterSouthEastSouthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaGreaterSouthEastEastSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaGreaterSouthEastEastSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        northEastXDeltaGreaterNorthWestNorthSide.Set(lastX, (lastY - TILE_SIZE_PX));
                        northEastXDeltaGreaterNorthWestWestSide.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaGreaterNorthWestNorthSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaGreaterNorthWestNorthSide.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            northEastXDeltaGreaterNorthWestWestSide.m_x >> TILE_SHIFT_PX,
                            northEastXDeltaGreaterNorthWestWestSide.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep->m_x >> TILE_SHIFT_PX,
                            sideYStep->m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep->m_x >> TILE_SHIFT_PX,
                            sideXStep->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
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
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
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
    i32 ix = x >> TILE_SHIFT_PX;
    i32 iy = y >> TILE_SHIFT_PX;
    CMapMgr* plane = g_gameReg->m_tileGrid;
    i32 attr;
    if (ix >= plane->m_width || iy >= plane->m_height) {
        attr = -1;
    } else {
        attr = plane->m_rowInts[iy][ix * 7 + 1];
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
        box.top = y - 7;
        box.bottom = y + 7;
        box.left = x - 7;
        box.right = x + 7;
        i32 ox = o->m_screenX - 7;
        i32 oy = o->m_screenY - 7;
        if (box.left > ox + 14 || box.right < ox || box.top > oy + 14 || box.bottom < oy) {
            return NULL;
        }
        *outPlayerIndex = playerIndex;
        *outUnitIndex = unitIndex;
        return cell;
    }
    CGameObject* o = cell->m_object;
    if (o->m_screenX != x || o->m_screenY != y) {
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
    i32 tcol = px >> TILE_SHIFT_PX;
    i32 trow = py >> TILE_SHIFT_PX;
    RECT rc;
    if (src) {
        CopyRect(&rc, src);
    } else {
        SetRect(
            &rc,
            px - span->left * TILE_SIZE_PX - 7,
            py - span->top * TILE_SIZE_PX - 7,
            span->right * TILE_SIZE_PX + px + 7,
            span->bottom * TILE_SIZE_PX + py + 7
        );
    }
    i32 x = tcol - span->left - 1;
    i32 xEnd = span->right + tcol + 1;

    if (static_cast<u32>(x) <= static_cast<u32>(xEnd)) {
        do {
            i32 yEnd = span->bottom + trow + 1;
            for (i32 y = trow - span->top - 1; static_cast<u32>(y) <= static_cast<u32>(yEnd); y++) {
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
                i32 sx = g->m_object->m_screenX - 7;
                i32 sy = g->m_object->m_screenY - 7;
                i32 sx2 = sx + 0xe;
                i32 sy2 = sy + 0xe;
                if (rc.left <= sx2 && rc.right >= sx && rc.top <= sy2 && rc.bottom >= sy) {
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
