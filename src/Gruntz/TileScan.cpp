#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>

// @early-stop
RVA(0x00035f10, 0x155)
i32 CBattlezMapConfig::Scan(CGrunt* arg) {
    if (static_cast<u32>(arg->m_dwell) <= static_cast<u32>(m_inactiveTargetRerouteDelay)) {
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
    i32 centerPxY = p->m_screenY;
    i32 centerPxX = p->m_screenX;
    i32 rowStart = (centerPxY >> TILE_SHIFT_PX) - 1;
    i32 rowEnd = (centerPxY >> TILE_SHIFT_PX) + 2;
    i32 colStart = (centerPxX >> TILE_SHIFT_PX) - 1;
    i32 colEnd = (centerPxX >> TILE_SHIFT_PX) + 2;
    for (i32 row = rowStart; row < rowEnd; row++) {
        for (i32 col = colStart; col < colEnd; col++) {
            if (col == (centerPxX >> TILE_SHIFT_PX) && row == (centerPxY >> TILE_SHIFT_PX)) {
                continue;
            }
            if (static_cast<u32>(col) >= static_cast<u32>(m_board->m_width)
                || static_cast<u32>(row) >= static_cast<u32>(m_board->m_height)) {
                continue;
            }
            i32 flags = m_board->CellFlagsAt(col, row);
            if (flags & BRICKZ_BLOCKED_MASK) {
                continue;
            }
            if ((flags & 2) == 0) {
                arg->TileSwitch(col, row, 0, 0xd87, 0, 0);
                arg->m_dwell = 0;
                return 1;
            }
        }
    }
    return 1;
}
