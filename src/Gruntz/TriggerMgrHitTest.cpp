#include <StdAfx.h>

#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>

#include <stddef.h>

#define TM_TRY_STEP(step, x, y, direction)                                                         \
    {                                                                                              \
        *pCell = *step.Set(x, y);                                                                  \
    }                                                                                              \
    {                                                                                              \
        i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(                                          \
            pCell->m_x >> TILE_SHIFT_PX,                                                           \
            pCell->m_y >> TILE_SHIFT_PX                                                            \
        );                                                                                         \
        *pFlags = cell;                                                                            \
        if (TmFlagsAllow(cell, mask, pass)) {                                                      \
            return direction;                                                                      \
        }                                                                                          \
    }

#define TM_TRY_DIAGONAL(step, ySide, xSide, x, y, direction)                                       \
    {                                                                                              \
        *pCell = *step.Set(x, y);                                                                  \
    }                                                                                              \
    {                                                                                              \
        i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(                                          \
            pCell->m_x >> TILE_SHIFT_PX,                                                           \
            pCell->m_y >> TILE_SHIFT_PX                                                            \
        );                                                                                         \
        *pFlags = cell;                                                                            \
        if (TmFlagsAllow(cell, mask, pass)) {                                                      \
            ySide.Set(lastX, y);                                                                   \
            xSide.Set(x, lastY);                                                                   \
            sideY = g_gameReg->GetTileGrid()->CellFlagsAt(                                         \
                ySide.m_x >> TILE_SHIFT_PX,                                                        \
                ySide.m_y >> TILE_SHIFT_PX                                                         \
            );                                                                                     \
            i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(                                     \
                xSide.m_x >> TILE_SHIFT_PX,                                                        \
                xSide.m_y >> TILE_SHIFT_PX                                                         \
            );                                                                                     \
            if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0                                             \
                && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {                                       \
                return direction;                                                                  \
            }                                                                                      \
        }                                                                                          \
    }

#define TM_TRY_DIAGONAL_COPY(step, ySide, xSide, x, y, direction)                                  \
    {                                                                                              \
        *pCell = *step.Set(x, y);                                                                  \
    }                                                                                              \
    {                                                                                              \
        i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(                                          \
            pCell->m_x >> TILE_SHIFT_PX,                                                           \
            pCell->m_y >> TILE_SHIFT_PX                                                            \
        );                                                                                         \
        *pFlags = cell;                                                                            \
        if (TmFlagsAllow(cell, mask, pass)) {                                                      \
            Coord sideYStep = *ySide.Set(lastX, y);                                                \
            Coord sideXStep = *xSide.Set(x, lastY);                                                \
            sideY = g_gameReg->GetTileGrid()->CellFlagsAt(                                         \
                sideYStep.m_x >> TILE_SHIFT_PX,                                                    \
                sideYStep.m_y >> TILE_SHIFT_PX                                                     \
            );                                                                                     \
            i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(                                     \
                sideXStep.m_x >> TILE_SHIFT_PX,                                                    \
                sideXStep.m_y >> TILE_SHIFT_PX                                                     \
            );                                                                                     \
            if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0                                             \
                && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {                                       \
                return direction;                                                                  \
            }                                                                                      \
        }                                                                                          \
    }

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
                TM_TRY_STEP(northEastGoalWestStep, lastX - TILE_SIZE_PX, lastY, s_gruntDirWest);
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
                TM_TRY_STEP(northEastGoalEastStep, lastX + TILE_SIZE_PX, lastY, s_gruntDirEast);
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_x > goalX) {
                TM_TRY_DIAGONAL(
                    northWestGoalNorthEastStep,
                    northWestGoalNorthEastNorthSide,
                    northWestGoalNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_STEP(northWestGoalEastStep, lastX + TILE_SIZE_PX, lastY, s_gruntDirEast);
                TM_TRY_DIAGONAL(
                    northWestGoalNorthWestStep,
                    northWestGoalNorthWestNorthSide,
                    northWestGoalNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_STEP(northWestGoalWestStep, lastX - TILE_SIZE_PX, lastY, s_gruntDirWest);
                return s_gruntDirCenter;
            } else {
                TM_TRY_DIAGONAL(
                    northAlignedGoalNorthWestStep,
                    northAlignedGoalNorthWestNorthSide,
                    northAlignedGoalNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_DIAGONAL(
                    northAlignedGoalNorthEastStep,
                    northAlignedGoalNorthEastNorthSide,
                    northAlignedGoalNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_STEP(northAlignedGoalWestStep, lastX - TILE_SIZE_PX, lastY, s_gruntDirWest);
                TM_TRY_STEP(northAlignedGoalEastStep, lastX + TILE_SIZE_PX, lastY, s_gruntDirEast);
                return s_gruntDirCenter;
            }
            break;
        case DIR_SOUTH:
            if (g->EntrancePx().m_x < goalX) {
                TM_TRY_DIAGONAL(
                    southEastGoalSouthWestStep,
                    southEastGoalSouthWestSouthSide,
                    southEastGoalSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_STEP(southEastGoalWestStep, lastX - TILE_SIZE_PX, lastY, s_gruntDirWest);
                TM_TRY_DIAGONAL(
                    southEastGoalSouthEastStep,
                    southEastGoalSouthEastSouthSide,
                    southEastGoalSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(southEastGoalEastStep, lastX + TILE_SIZE_PX, lastY, s_gruntDirEast);
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_x > goalX) {
                TM_TRY_DIAGONAL(
                    southWestGoalSouthEastStep,
                    southWestGoalSouthEastSouthSide,
                    southWestGoalSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(southWestGoalEastStep, lastX + TILE_SIZE_PX, lastY, s_gruntDirEast);
                TM_TRY_DIAGONAL(
                    southWestGoalSouthWestStep,
                    southWestGoalSouthWestSouthSide,
                    southWestGoalSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_STEP(southWestGoalWestStep, lastX - TILE_SIZE_PX, lastY, s_gruntDirWest);
            } else {
                TM_TRY_DIAGONAL(
                    southAlignedGoalSouthWestStep,
                    southAlignedGoalSouthWestSouthSide,
                    southAlignedGoalSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_DIAGONAL(
                    southAlignedGoalSouthEastStep,
                    southAlignedGoalSouthEastSouthSide,
                    southAlignedGoalSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(southAlignedGoalWestStep, lastX - TILE_SIZE_PX, lastY, s_gruntDirWest);
                TM_TRY_STEP(southAlignedGoalEastStep, lastX + TILE_SIZE_PX, lastY, s_gruntDirEast);
            }
            break;
        case DIR_EAST:
            if (g->EntrancePx().m_y < goalY) {
                TM_TRY_DIAGONAL(
                    eastSouthGoalNorthEastStep,
                    eastSouthGoalNorthEastNorthSide,
                    eastSouthGoalNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_STEP(eastSouthGoalNorthStep, lastX, lastY - TILE_SIZE_PX, s_gruntDirNorth);
                TM_TRY_DIAGONAL(
                    eastSouthGoalSouthEastStep,
                    eastSouthGoalSouthEastSouthSide,
                    eastSouthGoalSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(eastSouthGoalSouthStep, lastX, lastY + TILE_SIZE_PX, s_gruntDirSouth);
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_y > goalY) {
                TM_TRY_DIAGONAL(
                    eastNorthGoalSouthEastStep,
                    eastNorthGoalSouthEastSouthSide,
                    eastNorthGoalSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(eastNorthGoalSouthStep, lastX, lastY + TILE_SIZE_PX, s_gruntDirSouth);
                TM_TRY_DIAGONAL(
                    eastNorthGoalNorthEastStep,
                    eastNorthGoalNorthEastNorthSide,
                    eastNorthGoalNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_STEP(eastNorthGoalNorthStep, lastX, lastY - TILE_SIZE_PX, s_gruntDirNorth);
                return s_gruntDirCenter;
            } else {
                TM_TRY_DIAGONAL(
                    eastAlignedGoalNorthEastStep,
                    eastAlignedGoalNorthEastNorthSide,
                    eastAlignedGoalNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_DIAGONAL(
                    eastAlignedGoalSouthEastStep,
                    eastAlignedGoalSouthEastSouthSide,
                    eastAlignedGoalSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(eastAlignedGoalNorthStep, lastX, lastY - TILE_SIZE_PX, s_gruntDirNorth);
                TM_TRY_STEP(eastAlignedGoalSouthStep, lastX, lastY + TILE_SIZE_PX, s_gruntDirSouth);
                return s_gruntDirCenter;
            }
            break;
        case DIR_WEST:
            if (g->EntrancePx().m_y < goalY) {
                TM_TRY_DIAGONAL(
                    westSouthGoalNorthWestStep,
                    westSouthGoalNorthWestNorthSide,
                    westSouthGoalNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_STEP(westSouthGoalNorthStep, lastX, lastY - TILE_SIZE_PX, s_gruntDirNorth);
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
                TM_TRY_STEP(westSouthGoalSouthStep, lastX, lastY + TILE_SIZE_PX, s_gruntDirSouth);
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_y > goalY) {
                TM_TRY_DIAGONAL(
                    westNorthGoalSouthWestStep,
                    westNorthGoalSouthWestSouthSide,
                    westNorthGoalSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_STEP(westNorthGoalSouthStep, lastX, lastY + TILE_SIZE_PX, s_gruntDirSouth);
                TM_TRY_DIAGONAL(
                    westNorthGoalNorthWestStep,
                    westNorthGoalNorthWestNorthSide,
                    westNorthGoalNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_STEP(westNorthGoalNorthStep, lastX, lastY - TILE_SIZE_PX, s_gruntDirNorth);
            } else {
                TM_TRY_DIAGONAL(
                    westAlignedGoalNorthWestStep,
                    westAlignedGoalNorthWestNorthSide,
                    westAlignedGoalNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_DIAGONAL(
                    westAlignedGoalSouthWestStep,
                    westAlignedGoalSouthWestSouthSide,
                    westAlignedGoalSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_STEP(westAlignedGoalNorthStep, lastX, lastY - TILE_SIZE_PX, s_gruntDirNorth);
                TM_TRY_STEP(westAlignedGoalSouthStep, lastX, lastY + TILE_SIZE_PX, s_gruntDirSouth);
            }
            break;
        case DIR_NORTHEAST: {
            i32 deltaY = goalY - g->EntrancePx().m_y;
            i32 deltaX = g->EntrancePx().m_x - goalX;
            if (deltaX < deltaY) {
                TM_TRY_STEP(
                    northEastXDeltaLessNorthStep,
                    lastX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorth
                );
                TM_TRY_DIAGONAL(
                    northEastXDeltaLessNorthWestStep,
                    northEastXDeltaLessNorthWestNorthSide,
                    northEastXDeltaLessNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_STEP(
                    northEastXDeltaLessEastStep,
                    lastX + TILE_SIZE_PX,
                    lastY,
                    s_gruntDirEast
                );
                TM_TRY_DIAGONAL(
                    northEastXDeltaLessSouthEastStep,
                    northEastXDeltaLessSouthEastSouthSide,
                    northEastXDeltaLessSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
            } else if (deltaX > deltaY) {
                TM_TRY_STEP(
                    northEastXDeltaGreaterEastStep,
                    lastX + TILE_SIZE_PX,
                    lastY,
                    s_gruntDirEast
                );
                TM_TRY_DIAGONAL(
                    northEastXDeltaGreaterSouthEastStep,
                    northEastXDeltaGreaterSouthEastSouthSide,
                    northEastXDeltaGreaterSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(
                    northEastXDeltaGreaterNorthStep,
                    lastX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorth
                );
                TM_TRY_DIAGONAL(
                    northEastXDeltaGreaterNorthWestStep,
                    northEastXDeltaGreaterNorthWestNorthSide,
                    northEastXDeltaGreaterNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                return s_gruntDirCenter;
            } else {
                TM_TRY_STEP(
                    northEastEqualDeltasNorthStep,
                    lastX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorth
                );
                TM_TRY_STEP(
                    northEastEqualDeltasEastStep,
                    lastX + TILE_SIZE_PX,
                    lastY,
                    s_gruntDirEast
                );
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
                TM_TRY_DIAGONAL_COPY(
                    northEastEqualDeltasSouthEastStep,
                    northEastEqualDeltasSouthEastSouthSide,
                    northEastEqualDeltasSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
            }
            break;
        }
        case DIR_SOUTHEAST: {
            i32 deltaY = g->EntrancePx().m_y - goalY;
            i32 deltaX = g->EntrancePx().m_x - goalX;
            if (deltaX < deltaY) {
                TM_TRY_STEP(
                    southEastXDeltaLessSouthStep,
                    lastX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouth
                );
                TM_TRY_DIAGONAL_COPY(
                    southEastXDeltaLessSouthWestStep,
                    southEastXDeltaLessSouthWestSouthSide,
                    southEastXDeltaLessSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_STEP(
                    southEastXDeltaLessEastStep,
                    lastX + TILE_SIZE_PX,
                    lastY,
                    s_gruntDirEast
                );
                TM_TRY_DIAGONAL_COPY(
                    southEastXDeltaLessNorthEastStep,
                    southEastXDeltaLessNorthEastNorthSide,
                    southEastXDeltaLessNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                return s_gruntDirCenter;
            } else if (deltaX > deltaY) {
                TM_TRY_STEP(
                    southEastXDeltaGreaterEastStep,
                    lastX + TILE_SIZE_PX,
                    lastY,
                    s_gruntDirEast
                );
                TM_TRY_DIAGONAL_COPY(
                    southEastXDeltaGreaterNorthEastStep,
                    southEastXDeltaGreaterNorthEastNorthSide,
                    southEastXDeltaGreaterNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_STEP(
                    southEastXDeltaGreaterSouthStep,
                    lastX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouth
                );
                TM_TRY_DIAGONAL_COPY(
                    southEastXDeltaGreaterSouthWestStep,
                    southEastXDeltaGreaterSouthWestSouthSide,
                    southEastXDeltaGreaterSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                return s_gruntDirCenter;
            } else {
                TM_TRY_STEP(
                    southEastEqualDeltasSouthStep,
                    lastX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouth
                );
                TM_TRY_STEP(
                    southEastEqualDeltasEastStep,
                    lastX + TILE_SIZE_PX,
                    lastY,
                    s_gruntDirEast
                );
                TM_TRY_DIAGONAL_COPY(
                    southEastEqualDeltasSouthWestStep,
                    southEastEqualDeltasSouthWestSouthSide,
                    southEastEqualDeltasSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_DIAGONAL_COPY(
                    southEastEqualDeltasNorthEastStep,
                    southEastEqualDeltasNorthEastNorthSide,
                    southEastEqualDeltasNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
            }
            break;
        }
        case DIR_NORTHWEST: {
            i32 deltaY = goalY - g->EntrancePx().m_y;
            i32 deltaX = goalX - g->EntrancePx().m_x;
            if (deltaX < deltaY) {
                TM_TRY_STEP(
                    northWestXDeltaLessNorthStep,
                    lastX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorth
                );
                TM_TRY_DIAGONAL_COPY(
                    northWestXDeltaLessNorthEastStep,
                    northWestXDeltaLessNorthEastNorthSide,
                    northWestXDeltaLessNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_STEP(
                    northWestXDeltaLessWestStep,
                    lastX - TILE_SIZE_PX,
                    lastY,
                    s_gruntDirWest
                );
                TM_TRY_DIAGONAL_COPY(
                    northWestXDeltaLessSouthWestStep,
                    northWestXDeltaLessSouthWestSouthSide,
                    northWestXDeltaLessSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
            } else if (deltaX > deltaY) {
                TM_TRY_STEP(
                    northWestXDeltaGreaterWestStep,
                    lastX - TILE_SIZE_PX,
                    lastY,
                    s_gruntDirWest
                );
                TM_TRY_DIAGONAL_COPY(
                    northWestXDeltaGreaterSouthWestStep,
                    northWestXDeltaGreaterSouthWestSouthSide,
                    northWestXDeltaGreaterSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                TM_TRY_STEP(
                    northWestXDeltaGreaterNorthStep,
                    lastX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorth
                );
                TM_TRY_DIAGONAL_COPY(
                    northWestXDeltaGreaterNorthEastStep,
                    northWestXDeltaGreaterNorthEastNorthSide,
                    northWestXDeltaGreaterNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                return s_gruntDirCenter;
            } else {
                TM_TRY_STEP(
                    northWestEqualDeltasNorthStep,
                    lastX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorth
                );
                TM_TRY_STEP(
                    northWestEqualDeltasWestStep,
                    lastX - TILE_SIZE_PX,
                    lastY,
                    s_gruntDirWest
                );
                TM_TRY_DIAGONAL_COPY(
                    northWestEqualDeltasNorthEastStep,
                    northWestEqualDeltasNorthEastNorthSide,
                    northWestEqualDeltasNorthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthEast
                );
                TM_TRY_DIAGONAL_COPY(
                    northWestEqualDeltasSouthWestStep,
                    northWestEqualDeltasSouthWestSouthSide,
                    northWestEqualDeltasSouthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthWest
                );
                return s_gruntDirCenter;
            }
            break;
        }
        case DIR_SOUTHWEST: {
            i32 deltaY = g->EntrancePx().m_y - goalY;
            i32 deltaX = goalX - g->EntrancePx().m_x;
            if (deltaX < deltaY) {
                TM_TRY_STEP(
                    southWestXDeltaLessSouthStep,
                    lastX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouth
                );
                TM_TRY_DIAGONAL_COPY(
                    southWestXDeltaLessSouthEastStep,
                    southWestXDeltaLessSouthEastSouthSide,
                    southWestXDeltaLessSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_STEP(
                    southWestXDeltaLessWestStep,
                    lastX - TILE_SIZE_PX,
                    lastY,
                    s_gruntDirWest
                );
                TM_TRY_DIAGONAL_COPY(
                    southWestXDeltaLessNorthWestStep,
                    southWestXDeltaLessNorthWestNorthSide,
                    southWestXDeltaLessNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                return s_gruntDirCenter;
            } else if (deltaX > deltaY) {
                TM_TRY_STEP(
                    southWestXDeltaGreaterWestStep,
                    lastX - TILE_SIZE_PX,
                    lastY,
                    s_gruntDirWest
                );
                TM_TRY_DIAGONAL_COPY(
                    southWestXDeltaGreaterNorthWestStep,
                    southWestXDeltaGreaterNorthWestNorthSide,
                    southWestXDeltaGreaterNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
                TM_TRY_STEP(
                    southWestXDeltaGreaterSouthStep,
                    lastX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouth
                );
                TM_TRY_DIAGONAL_COPY(
                    southWestXDeltaGreaterSouthEastStep,
                    southWestXDeltaGreaterSouthEastSouthSide,
                    southWestXDeltaGreaterSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                return s_gruntDirCenter;
            } else {
                TM_TRY_STEP(
                    southWestEqualDeltasSouthStep,
                    lastX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouth
                );
                TM_TRY_STEP(
                    southWestEqualDeltasWestStep,
                    lastX - TILE_SIZE_PX,
                    lastY,
                    s_gruntDirWest
                );
                TM_TRY_DIAGONAL_COPY(
                    southWestEqualDeltasSouthEastStep,
                    southWestEqualDeltasSouthEastSouthSide,
                    southWestEqualDeltasSouthEastEastSide,
                    lastX + TILE_SIZE_PX,
                    lastY + TILE_SIZE_PX,
                    s_gruntDirSouthEast
                );
                TM_TRY_DIAGONAL_COPY(
                    southWestEqualDeltasNorthWestStep,
                    southWestEqualDeltasNorthWestNorthSide,
                    southWestEqualDeltasNorthWestWestSide,
                    lastX - TILE_SIZE_PX,
                    lastY - TILE_SIZE_PX,
                    s_gruntDirNorthWest
                );
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
    CGrunt* cell = UnitAt(playerIndex, unitIndex);
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
                i32 val = g_gameReg->m_tileGrid->OccupantAt(x, y);
                if (val == -1) {
                    continue;
                }
                i32 playerIndex =
                    (val >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
                i32 unitIndex = val & GRUNT_IDENTITY_COMPONENT_MASK;
                CGrunt* g = UnitAt(playerIndex, unitIndex);
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
