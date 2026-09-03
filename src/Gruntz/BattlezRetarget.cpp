#include <rva.h>

#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::RetargetIdleUnit(CGrunt* unit) {
    GruntzPlayer* recA = NULL;
    CBattlezMapConfig* cfgB = NULL;
    i32 cell = unit->ArrivalCell().m_x;
    if (cell >= 0 && cell < 4) {
        recA = &m_ctx->m_players[cell];
        cfgB = &recA->m_battlezConfig;
    }
    if (unit->CoordCount() == 0) {
        if (cell == -1) {
            if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_moveBudget)) {
                return 1;
            }
            i32 r = rand() % 4;
            if (r == m_playerIndex) {
                r++;
            }
            i32 band = r % 4;
            CBattlezMapConfig* b = &m_ctx->m_players[band].m_battlezConfig;
            if (b != NULL) {
                i32 cnt = b->m_attackWaypoints.GetSize();
                Coord target = b->m_marker;
                if (cnt != 0) {
                    Coord** arr = MfcPtrArrayData<Coord>(b->m_attackWaypoints);
                    Coord* pair = arr[rand() % cnt];
                    target = *pair;
                }
                if (unit->TileSwitch(
                        target.m_x,
                        target.m_y,
                        0,
                        IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER
                            | CELL_FLAG_BRIDGE | CELL_FLAG_REVEALED_POWERUP | CELL_FLAG_ARROW
                            | CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD),
                        0,
                        BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER
                    )
                    != 0) {
                    unit->m_arrivalCell.Set(band, 0);
                    AcceptAlways(unit);
                }
            }
            unit->m_dwell = 0;
            return 1;
        }
        CBattlezMapConfig* recB = &m_ctx->m_players[cell].m_battlezConfig;
        if (recB == NULL) {
            return 1;
        }
        if (static_cast<u32>(unit->m_dwell) <= 0x7d0) {
            return 1;
        }

        Coord target = recB->m_marker;
        unit->TileSwitch(
            target.m_x,
            target.m_y,
            0,
            IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                | CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD),
            0,
            IDX(CELL_FLAG_BRIDGE | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_REVEALED_POWERUP
                | CELL_FLAG_GAUNTLET_BRICK)
        );
        unit->m_dwell = 0;
        return 1;
    }
    if (recA == NULL || cfgB == NULL) {
        unit->m_arrivalCell.Set(-1, -1);
        return 1;
    }
    if (recA->m_humanControlled == false && cfgB->m_active == false) {
        RecycleGruntCoords(unit);
        unit->m_arrivalCell.Set(-1, -1);
        return 1;
    }
    if (unit->ArrivalCell().m_y == 1) {
        return 1;
    }
    Coord position;
    unit->GetScreenTile(&position);
    i32 nearBand = 0;

    i32 cnt2 = cfgB->m_attackWaypoints.GetSize();
    if (cnt2 > 0) {
        Coord** vec = MfcPtrArrayData<Coord>(cfgB->m_attackWaypoints);
        for (i32 j = cnt2; j > 0; j--) {
            Coord* pair = *vec;
            Coord distance = (*pair - position).GetAbs();
            if (distance.m_x + distance.m_y <= 6) {
                nearBand = 1;
            }
            vec++;
        }
    }
    if (nearBand == 0) {
        return 1;
    }
    unit->m_arrivalCell.m_y = 1;
    if (unit->CoordCount() == 0) {
        return 1;
    }
    RecycleGruntCoords(unit);
    return 1;
}
