#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Wap32/TileGeometry.h>

static inline Coord ScreenPosition(CGameObject* object) {
    return object->ScreenPos();
}

static inline Coord ScreenTile(CGrunt* grunt) {
    Coord out;
    grunt->GetScreenTile(&out);
    return out;
}

static inline RECT TileNeighborhood(CGrunt* grunt) {
    Coord high = ScreenTile(grunt);
    Coord low = ScreenTile(grunt);
    return MakeRect(low.m_x - 1, low.m_y - 1, high.m_x + 2, high.m_y + 2);
}

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
    RECT box = TileNeighborhood(grunt);
    for (i32 row = box.top; row < box.bottom; row++) {
        for (i32 col = box.left; col < box.right; col++) {
            Coord tile = center;
            ScreenTile(&tile);
            if (col == tile.m_x && row == tile.m_y) {
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
                grunt->TileSwitch(
                    col,
                    row,
                    0,
                    IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                        | CELL_FLAG_WATER | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD),
                    0,
                    0
                );
                grunt->m_dwell = 0;
                return 1;
            }
        }
    }
    return 1;
}
