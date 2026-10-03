#ifndef GRUNTZ_MAPTRAVERSALINLINE_H
#define GRUNTZ_MAPTRAVERSALINLINE_H

#include <Gruntz/Brickz.h>

inline i32 CMapMgr::CanStepBetween(
    i32 sourceX,
    i32 sourceY,
    i32 targetX,
    i32 targetY,
    i32 blockedMask,
    i32 passableMask
) const {
    if (sourceX == targetX && sourceY == targetY) {
        return 1;
    }
    u32 width = m_width;
    if (static_cast<u32>(targetX) >= width || static_cast<u32>(targetY) >= m_height) {
        return 0;
    }
    BrickzCell** rows = m_rows;
    BrickzCell* target = &rows[targetY][targetX];
    i32 hit = blockedMask & target->m_flags;
    if (hit & BRICKZ_CELL_OCCUPIED) {
        return 0;
    }
    if (hit != 0 && (target->m_flags & passableMask) == 0) {
        return 0;
    }
    BrickzCell* source = &rows[sourceY][sourceX];
    i32 dx = targetX - sourceX;
    i32 dy = targetY - sourceY;
    if (dx != 0 && dy != 0) {
        if (dx > 0 && dy > 0) {
            if (((source + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((source + width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target - width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy > 0) {
            if (((source - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((source + width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target - width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx > 0 && dy < 0) {
            if (((source + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((source - width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target + width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        } else if (dx < 0 && dy < 0) {
            if (((source - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((source - width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                || ((target + width)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                return 0;
            }
        }
    }
    return 1;
}

#endif // GRUNTZ_MAPTRAVERSALINLINE_H
