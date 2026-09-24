#ifndef GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H

#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Pix16.h>
#include <Wap32/TileGeometry.h>

static inline i32 TileFlags(const char* rec) {

    Pix16CPtr r;
    r.m_chars = rec;
    return *r.m_dwords;
}

static __inline i32 s_CanCommitToyMove(CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 tx = sourceX >> TILE_SHIFT_PX;
    i32 ty = sourceY >> TILE_SHIFT_PX;
    i32 mtx = moveX >> TILE_SHIFT_PX;
    i32 mty = moveY >> TILE_SHIFT_PX;
    i32 arr = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    if (tx != mtx || ty != mty) {
        if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        i32* tgt = &board->m_rowInts[mty][mtx * 7];
        i32 tflags = *tgt;
        i32 hit = arr & tflags;
        if (hit & BRICKZ_CELL_OCCUPIED) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = g->m_passableMask | 0x18000482;
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        i32 dx = mtx - tx;
        i32 dy = mty - ty;
        if (dx == 0 || dy == 0) {
            return 1;
        }
        char* cur = board->m_rowBytes[ty] + tx * 7 * 4;

        Pix16Ptr row;
        row.m_dwords = tgt;
        char* tg = row.m_chars;
        if (dx > 0 && dy > 0) {
            if ((cur[0x1d] & 0x20) || (cur[board->m_width * 7 * 4 + 1] & 0x20)
                || (TileFlags(tg - 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy > 0) {
            if ((cur[-0x1b] & 0x20) || (cur[board->m_width * 7 * 4 + 1] & 0x20)
                || (TileFlags(tg + 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx > 0 && dy < 0) {
            if ((cur[0x1d] & 0x20)
                || (TileFlags(cur - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy < 0) {
            if ((cur[-0x1b] & 0x20)
                || (TileFlags(cur - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        }
    }
    return 1;
}

static __inline i32 s_CanCommitBagMove(CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 tx = sourceX >> TILE_SHIFT_PX;
    i32 ty = sourceY >> TILE_SHIFT_PX;
    i32 mtx = moveX >> TILE_SHIFT_PX;
    i32 mty = moveY >> TILE_SHIFT_PX;
    i32 arr = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    if (tx != mtx || ty != mty) {
        if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
            || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
            return 0;
        }
        i32* tgt = &board->m_rowInts[mty][mtx * 7];
        i32 tflags = *tgt;
        i32 hit = arr & tflags;
        if (hit & BRICKZ_CELL_OCCUPIED) {
            return 0;
        }
        if (hit != 0) {
            i32 mask = g->m_passableMask | 0x18000482;
            if ((tflags & mask) == 0) {
                return 0;
            }
        }
        i32 dx = mtx - tx;
        i32 dy = mty - ty;
        if (dx == 0 || dy == 0) {
            return 1;
        }
        char* cur = board->m_rowBytes[ty] + tx * 7 * 4;

        Pix16Ptr row;
        row.m_dwords = tgt;
        char* tg = row.m_chars;
        if (dx > 0 && dy > 0) {
            if ((TileFlags(cur + 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(cur + board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy > 0) {
            if ((TileFlags(cur - 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(cur + board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx > 0 && dy < 0) {
            if ((TileFlags(cur + 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(cur - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg - 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy < 0) {
            if ((TileFlags(cur - 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(cur - board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + 0x1c) & BRICKZ_CELL_ROUTE_MASKB)
                || (TileFlags(tg + board->m_width * 7 * 4) & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        }
    }
    return 1;
}

#endif // GRUNTZ_GRUNTZ_GRUNTMOVECOLLISIONINLINE_H
