#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>

static inline i32 GridLookup(CMapMgr* g, i32 x, i32 y) {
    if (static_cast<u32>(x) < static_cast<u32>(g->m_width)
        && static_cast<u32>(y) < static_cast<u32>(g->m_height)) {
        return g->m_rows[y][x].m_flags;
    }
    return 1;
}

// @early-stop
RVA(0x00035f10, 0x155)
i32 CBattlezMapConfig::Scan(CGrunt* arg) {
    if (arg->m_dwell <= m_inactiveTargetRerouteDelay) {
        return 1;
    }
    i32 v = arg->m_targetTeam;
    i32 ok = 0;
    if (v != -1) {
        GruntzPlayer* fs = &m_ctx->m_options[v];
        if (fs->m_clearedRound != 0) {
            ok = 1;
        } else if (fs->m_liveGate == 0) {
            ok = 1;
        }
    }
    if (ok == 0) {
        return 1;
    }

    CGameObject* p = arg->m_object;
    i32 v60 = p->m_screenY;
    i32 v5c = p->m_screenX;
    i32 tileY = v60 >> 5;
    i32 tileX = v5c >> 5;
    for (i32 a = tileY - 1; a < tileY + 2; a++) {
        for (i32 b = tileX - 1; b < tileX + 2; b++) {
            if (b == (v5c >> 5) && a == (v60 >> 5)) {
                continue;
            }
            CMapMgr* grid = m_board;
            if (static_cast<u32>(b) >= static_cast<u32>(grid->m_width)
                || static_cast<u32>(a) >= static_cast<u32>(grid->m_height)) {
                continue;
            }
            i32 flags = GridLookup(grid, b, a);
            if (flags & 0x939) {
                continue;
            }
            if ((flags & 2) == 0) {
                arg->TileSwitch(b, a, 0, 0xd87, 0, 0);
                arg->m_dwell = 0;
                return 1;
            }
        }
    }
    return 1;
}
