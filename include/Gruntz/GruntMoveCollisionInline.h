#ifndef GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H

#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Wap32/TileGeometry.h>

inline i32 CGrunt::CanCommitMove(i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) const {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 tx = sourceX >> TILE_SHIFT_PX;
    i32 ty = sourceY >> TILE_SHIFT_PX;
    i32 mtx = moveX >> TILE_SHIFT_PX;
    i32 mty = moveY >> TILE_SHIFT_PX;
    i32 arr = m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    if (tx != mtx || ty != mty) {
        if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        BrickzCell* tgt = &board->m_rows[mty][mtx];
        i32 tflags = tgt->m_flags;
        i32 hit = arr & tflags;
        if (hit & BRICKZ_CELL_OCCUPIED) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = m_passableMask | 0x18000482;
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        i32 dx = mtx - tx;
        i32 dy = mty - ty;
        if (dx == 0 || dy == 0) {
            return 1;
        }
        BrickzCell* cur = &board->m_rows[ty][tx];
        BrickzCell* tg = tgt;
        if (dx > 0 && dy > 0) {
            if (((cur + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((cur + board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg - board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy > 0) {
            if (((cur - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((cur + board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg - board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx > 0 && dy < 0) {
            if (((cur + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((cur - board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg + board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy < 0) {
            if (((cur - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((cur - board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((tg + board->m_width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        }
    }
    return 1;
}

#endif // GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H
