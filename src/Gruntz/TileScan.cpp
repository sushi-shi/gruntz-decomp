#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntNeighborhoodInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
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
        if (targetPlayer->m_clearedRound != false) {
            targetUnavailable = 1;
        } else if (targetPlayer->m_active == false) {
            targetUnavailable = 1;
        }
    }
    if (targetUnavailable == 0) {
        return 1;
    }

    Coord center = ScreenPosition(grunt->m_object);
    RECT box = AdjacentTileNeighborhood(grunt);
    for (i32 row = box.top; row < box.bottom; row++) {
        for (i32 col = box.left; col < box.right; col++) {
            i32 tileX = center.m_x >> TILE_SHIFT_PX;
            i32 tileY = center.m_y >> TILE_SHIFT_PX;
            if (col == tileX && row == tileY) {
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
            if ((flags & IDX(CELL_FLAG_SPECIAL)) == 0) {
                grunt->TileSwitch(col, row, 0, 0xd87, 0, 0);
                grunt->m_dwell = 0;
                return 1;
            }
        }
    }
    return 1;
}
