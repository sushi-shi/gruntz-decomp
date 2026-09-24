#ifndef GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H

#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Wap32/TileGeometry.h>

static inline i32 DiagonalRouteBlocked(CMapMgr* board, const Coord& source, const Coord& target) {
    Coord horizontalStep(target.m_x > source.m_x ? 1 : -1, 0);
    Coord verticalStep(0, target.m_y > source.m_y ? 1 : -1);
    Coord sourceHorizontal = source + horizontalStep;
    Coord sourceVertical = source + verticalStep;
    Coord targetHorizontal = target - horizontalStep;
    Coord targetVertical = target - verticalStep;
    return (board->CellFlagsAt(sourceHorizontal.m_x, sourceHorizontal.m_y)
            & BRICKZ_CELL_ROUTE_MASKB)
           || (board->CellFlagsAt(sourceVertical.m_x, sourceVertical.m_y) & BRICKZ_CELL_ROUTE_MASKB)
           || (board->CellFlagsAt(targetHorizontal.m_x, targetHorizontal.m_y)
               & BRICKZ_CELL_ROUTE_MASKB)
           || (board->CellFlagsAt(targetVertical.m_x, targetVertical.m_y)
               & BRICKZ_CELL_ROUTE_MASKB);
}

inline i32 CGrunt::CanCommitMove(i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) const {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    Coord source(sourceX, sourceY);
    ScreenTile(&source);
    Coord move(moveX, moveY);
    ScreenTile(&move);
    i32 arr = m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    if (source != move) {
        if (static_cast<u32>(move.m_x) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(move.m_y) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        BrickzCell* targetCell = &board->m_rows[move.m_y][move.m_x];
        i32 tflags = targetCell->m_flags;
        i32 hit = arr & tflags;
        if (hit & BRICKZ_CELL_OCCUPIED) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = m_passableMask
                       | IDX(
                           CELL_FLAG_SPECIAL | CELL_FLAG_ARROW | CELL_FLAG_SPIKES
                           | CELL_FLAG_STATIC_HAZARD | CELL_FLAG_ROLLING_BALL
                       );
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        Coord delta = move - source;
        if (delta.m_x == 0 || delta.m_y == 0) {
            return 1;
        }
        if (DiagonalRouteBlocked(board, source, move)) {
            return 0;
        }
    }
    return 1;
}

#endif // GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H
