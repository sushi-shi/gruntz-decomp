

#include <Gruntz/Grunt.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>

#include <Gruntz/TileGrid.h>

#include <Gruntz/TriggerMgrViews.h>

RVA(0x000759e0, 0x18)
Coord* CTriggerMgr::GetOriginXY(Coord* out) {
    out->m_x = m_cellFlag[0x16];
    out->m_y = m_cellFlag[0x17];
    return out;
}

RVA(0x00075a10, 0x12)
Coord* Coord::Set(i32 a, i32 b) {
    m_x = a;
    m_y = b;
    return this;
}

RVA(0x00075a40, 0x34)
i32 CGridLookup::Lookup(i32 x, i32 y) {
    if (static_cast<u32>(x) < static_cast<u32>(m_width)
        && static_cast<u32>(y) < static_cast<u32>(m_height)) {
        return m_rows[y][x].m_flags;
    }
    return 1;
}

RVA(0x00075a90, 0x27)
i32 TmFlagsAllow(i32 a, i32 b, i32 c) {
    i32 m = b & a;
    if (m & 0x20000000) {
        return 0;
    }
    if (m != 0 && (c & a) == 0) {
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x00075af0, 0x111)
CGrunt* CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact) {
    CMapMgr* plane = g_gameReg->m_tileGrid;
    i32 ix = x >> 5;
    i32 iy = y >> 5;
    i32 attr;
    if (ix >= plane->m_width || iy >= plane->m_height) {
        attr = -1;
    } else {
        attr = plane->m_rowInts[iy][ix * 7 + 1];
    }
    if (attr == -1) {
        return 0;
    }
    i32 row = (attr >> 8) & 0xff;
    i32 col = attr & 0xff;
    CGrunt* cell = m_grid[col + row * TM_GRID_COLS];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }

    if (exact == 0) {
        CGameObject* o = cell->m_object;
        i32 ylo = y - 7;
        i32 yhi = y + 7;
        i32 xlo = x - 7;
        i32 xhi = x + 7;
        i32 ox = o->m_screenX - 7;
        i32 oy = o->m_screenY - 7;
        if (xlo > ox + 14 || xhi < ox || ylo > oy + 14 || yhi < oy) {
            return 0;
        }
        *outRow = row;
        *outCol = col;
        return cell;
    }
    CGameObject* o = cell->m_object;
    if (o->m_screenX != x || o->m_screenY != y) {
        return 0;
    }
    *outRow = row;
    *outCol = col;
    return cell;
}

// @early-stop
RVA(0x00075c60, 0x1ba)
CGrunt* CTriggerMgr::FindGruntAt(i32 px, i32 py, RECT* span, i32* outCol, i32* outRow, RECT* src) {
    i32 tcol = px >> 5;
    i32 trow = py >> 5;
    RECT rc;
    if (src) {
        CopyRect(&rc, src);
    } else {
        SetRect(
            &rc,
            px - span->left * 32 - 7,
            py - span->top * 32 - 7,
            span->right * 32 + px + 7,
            span->bottom * 32 + py + 7
        );
    }
    i32 xEnd = span->right + tcol + 1;
    i32 x = tcol - span->left - 1;

    if (static_cast<u32>(x) <= static_cast<u32>(xEnd)) {
        do {
            i32 yEnd = span->bottom + trow + 1;
            for (i32 y = trow - span->top - 1; static_cast<u32>(y) <= static_cast<u32>(yEnd); y++) {
                if (static_cast<u32>(x) >= static_cast<u32>(g_gameReg->m_tileGrid->m_width)) {
                    continue;
                }
                CMapMgr* grid = g_gameReg->m_tileGrid;
                if (static_cast<u32>(y) >= static_cast<u32>(grid->m_height)) {
                    continue;
                }
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
                i32 col = val & 0xff;
                i32 row = (val >> 8) & 0xff;
                CGrunt* g = m_grid[col + row * TM_GRID_COLS];
                if (!g) {
                    continue;
                }
                if (!g->m_entranceCommitted) {
                    continue;
                }
                i32 sx = g->m_object->m_screenX - 7;
                i32 sy = g->m_object->m_screenY - 7;
                if (rc.left <= sx + 0xe && rc.right >= sx && rc.top <= sy + 0xe
                    && rc.bottom >= sy) {
                    *outCol = row;
                    *outRow = col;
                    return g;
                }
            }
            x++;
        } while (static_cast<u32>(x) <= static_cast<u32>(xEnd));
    }
    return 0;
}
