#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>

RVA(0x00035f10, 0x155)
i32 CBattlezMapConfig::RerouteSwitchSeeker(CGrunt* grunt) {
    if (static_cast<u32>(grunt->m_dwell) <= static_cast<u32>(m_inactiveTargetRerouteDelay)) {
        return 1;
    }
    i32 targetTeamIndex = grunt->m_targetTeam;
    i32 targetUnavailable = 0;
    if (targetTeamIndex != -1) {
        GruntzPlayer* targetPlayer = &m_ctx->m_players[targetTeamIndex];
        if (targetPlayer->m_clearedRound != 0) {
            targetUnavailable = 1;
        } else if (targetPlayer->m_active == 0) {
            targetUnavailable = 1;
        }
    }
    if (targetUnavailable == 0) {
        return 1;
    }

    CGameObject* object = grunt->m_object;
    i32 centerPxY = object->m_screenY;
    i32 centerPxX = object->m_screenX;
    RECT box;
    box.top = (centerPxY >> TILE_SHIFT_PX) - 1;
    box.bottom = (centerPxY >> TILE_SHIFT_PX) + 2;
    box.left = (centerPxX >> TILE_SHIFT_PX) - 1;
    box.right = (centerPxX >> TILE_SHIFT_PX) + 2;
    for (i32 row = box.top; row < box.bottom; row++) {
        for (i32 col = box.left; col < box.right; col++) {
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
                grunt->TileSwitch(col, row, 0, 0xd87, 0, 0);
                grunt->m_dwell = 0;
                return 1;
            }
        }
    }
    return 1;
}
