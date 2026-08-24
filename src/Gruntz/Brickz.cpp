#include <rva.h>

#include <Gruntz/Brickz.h>

#include <Gruntz/BattlezData.h>
#include <Gruntz/SerialArchive.h>

#include <stdlib.h>

// @early-stop
RVA(0x00081e10, 0x1a7)
i32 CMapMgr::SearchEdge(
    i32 xA,
    i32 yA,
    i32 xB,
    i32 yB,
    CPtrList* list,
    i32 clearFlag,
    i32 maskA,
    i32 maskC
) {
    CMapMgr* self = this;
    if (static_cast<u32>(xA) >= self->m_width || static_cast<u32>(yA) >= self->m_height
        || static_cast<u32>(xB) >= self->m_width || static_cast<u32>(yB) >= self->m_height) {
        return 0;
    }
    BrickzCell* cellB = &self->m_rows[yB][xB];
    BrickzCell* cellA = &self->m_rows[yA][xA];
    i32 savedB0 = cellB->m_flags;
    i32 savedA4 = cellA->m_occupantId;
    i32 savedA0 = cellA->m_flags;
    i32 wasOccupied = (static_cast<u32>(savedB0) >> 29) & 1;
    i32 savedB4 = cellB->m_occupantId;
    if (wasOccupied != 0) {
        cellB->m_flags = savedB0 & BRICKZ_CELL_UNOCCUPIED_MASK;
    }

    m_rows[yA][xA].m_occupantId = -1;
    m_rows[yB][xB].m_occupantId = -1;
    m_edgeMask = maskA & BRICKZ_CELL_OCCUPIED;
    if (clearFlag != 0) {
        m_rows[yA][xA].m_flags = 0;
        m_rows[yB][xB].m_flags = 0;
    }
    i32 ret = CMapMgr::Search(xA, yA, xB, yB, list, maskA, BRICKZ_CELL_ROUTE_MASKB, maskC);
    m_edgeMask = 0;
    m_rows[yA][xA].m_occupantId = savedA4;
    m_rows[yB][xB].m_occupantId = savedB4;
    if (clearFlag != 0) {
        m_rows[yA][xA].m_flags = savedA0;
        m_rows[yB][xB].m_flags = savedB0;
    }
    if (wasOccupied != 0) {
        BrickzCell* c = &m_rows[yB][xB];
        c->m_flags |= BRICKZ_CELL_OCCUPIED;
    }
    return ret;
}

// @early-stop
RVA(0x00082030, 0x1a1)
i32 CMapMgr::UpdateDiagonals(CGruntzMgr* unused) {
    BrickzCell* cell = m_cellPool;
    if (m_dirty != 0) {
        for (u32 r = 0; r < m_height; r++) {
            for (u32 c = 0; c < m_width; c++) {
                if ((cell->m_flags & 0x100) != 0) {
                    BrickzCell* down = NULL;
                    BrickzCell* right = NULL;
                    BrickzCell* left = NULL;
                    BrickzCell* up = NULL;
                    BrickzCell* ur = NULL;
                    BrickzCell* ul = NULL;
                    BrickzCell* dr = NULL;
                    BrickzCell* dl = NULL;
                    if (r != 0) {
                        up = cell - m_width;
                    }
                    if (r < m_height - 1) {
                        down = cell + m_width;
                    }
                    if (c < m_width - 1) {
                        right = cell + 1;
                    }
                    if (c != 0) {
                        left = cell - 1;
                    }
                    if (up && right) {
                        ur = up + 1;
                    }
                    if (up && left) {
                        ul = up - 1;
                    }
                    if (down && right) {
                        dr = down + 1;
                    }
                    if (down && left) {
                        dl = down - 1;
                    }
                    cell->m_flags &= ~0x1000;
                    if ((up && down && !(up->m_flags & BRICKZ_BLOCKED_MASK)
                         && !(down->m_flags & BRICKZ_BLOCKED_MASK))
                        || (right && left && !(right->m_flags & BRICKZ_BLOCKED_MASK)
                            && !(left->m_flags & BRICKZ_BLOCKED_MASK))
                        || (ur && dl && !(ur->m_flags & BRICKZ_BLOCKED_MASK)
                            && !(dl->m_flags & BRICKZ_BLOCKED_MASK))
                        || (ul && dr && !(ul->m_flags & BRICKZ_BLOCKED_MASK)
                            && !(dr->m_flags & BRICKZ_BLOCKED_MASK))) {
                        cell->m_flags |= 0x1000;
                    }
                }
                cell++;
            }
        }
        m_dirty = 0;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00082250, 0x17c)
i32 CMapMgr::LineIsClear(i32 x0, i32 y0, i32 x1, i32 y1) {
    if (x0 == x1 && y0 == y1) {
        return 1;
    }

    i32 dy, dx;
    dx = x1 - x0;
    dy = y1 - y0;
    if (abs(dx) > abs(dy)) {
        i32 slope = (dy << 16) / dx;
        i32 yacc = y0 << 16;
        if (dx > 0) {
            for (i32 x = x0; x < x1; x++) {
                if (m_rows[yacc >> 16][x].m_flags != 0) {
                    return 0;
                }
                yacc += slope;
            }
        } else {
            for (i32 x = x0; x > x1; x--) {
                if (m_rows[yacc >> 16][x].m_flags != 0) {
                    return 0;
                }
                yacc += slope;
            }
        }
    } else {
        i32 slope = (dx << 16) / dy;
        i32 xacc = x0 << 16;
        if (dy > 0) {
            for (i32 y = y0; y < y1; y++) {
                if (m_rows[y][xacc >> 16].m_flags != 0) {
                    return 0;
                }
                xacc += slope;
            }
        } else {
            for (i32 y = y0; y > y1; y--) {
                if (m_rows[y][xacc >> 16].m_flags != 0) {
                    return 0;
                }
                xacc += slope;
            }
        }
    }
    return 1;
}

RVA(0x000853f0, 0x46)
i32 CMapMgr::IsCellClear(i32 x, i32 y) {
    return CellFlagsAt(x, y) == 0;
}
