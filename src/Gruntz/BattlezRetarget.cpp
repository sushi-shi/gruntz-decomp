#include <StdAfx.h>

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Globals.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
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
#include <Wwd/WwdFile.h>
#include <ZTools/BitVec.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::RetargetIdleUnit(CGrunt* unit) {
    GruntzPlayer* recA = NULL;
    CBattlezMapConfig* cfgB = NULL;
    i32 cell = unit->ArrivalCell().m_x;
    if (cell >= 0 && cell < 4) {
        recA = &m_ctx->m_players[cell];
        cfgB = recA->GetBattlezConfig();
    }
    if (unit->CoordsEmpty()) {
        if (cell == -1) {
            if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_moveBudget)) {
                return 1;
            }
            i32 r = rand() % 4;
            if (r == m_playerIndex) {
                r++;
            }
            i32 band = r % 4;
            CBattlezMapConfig* b = m_ctx->m_players[band].GetBattlezConfig();
            if (b != NULL) {
                i32 cnt = b->m_attackWaypoints.GetSize();
                Coord goal = b->m_marker;
                if (cnt != 0) {
                    Coord* pair = static_cast<Coord*>(b->m_attackWaypoints.GetAt(rand() % cnt));
                    goal = *pair;
                }
                if (unit->TileSwitch(goal.m_x, goal.m_y, 0, 0x9cf, 0, 0x4020) != 0) {
                    unit->m_arrivalCell.Set(band, 0);
                    AcceptAlways(unit);
                }
            }
            unit->m_dwell = 0;
            return 1;
        }
        CBattlezMapConfig* recB = m_ctx->m_players[cell].GetBattlezConfig();
        if (recB == NULL) {
            return 1;
        }
        if (static_cast<u32>(unit->m_dwell) <= 0x7d0) {
            return 1;
        }

        i32 y = recB->m_marker.m_y;
        i32 x = recB->m_marker.m_x;
        unit->TileSwitch(x, y, 0, 0x987, 0, 0x4068);
        unit->m_dwell = 0;
        return 1;
    }
    if (recA == NULL || cfgB == NULL) {
        UNSET_COORD(unit->m_arrivalCell);
        return 1;
    }
    if (recA->m_humanControlled == false && cfgB->m_active == false) {
        unit->RecycleCoords();
        UNSET_COORD(unit->m_arrivalCell);
        return 1;
    }
    if (unit->ArrivalCell().m_y == 1) {
        return 1;
    }
    CGameObject* lvl = unit->m_object;
    i32 px = lvl->m_screenX >> TILE_SHIFT_PX;
    i32 py = lvl->m_screenY >> TILE_SHIFT_PX;
    i32 nearBand = 0;

    i32 cnt2 = cfgB->m_attackWaypoints.GetSize();
    if (cnt2 > 0) {
        for (i32 j = 0; j < cnt2; j++) {
            Coord* pair = static_cast<Coord*>(cfgB->m_attackWaypoints.GetAt(j));
            i32 dy = abs(pair->m_y - py);
            i32 dx = abs(pair->m_x - px);
            if (dx + dy <= 6) {
                nearBand = 1;
            }
        }
    }
    if (nearBand == 0) {
        return 1;
    }
    unit->m_arrivalCell.Set(unit->m_arrivalCell.m_x, 1);
    if (unit->CoordsEmpty()) {
        return 1;
    }
    unit->RecycleCoords();
    return 1;
}
