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
#include <Gruntz/GameRand.h>
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
i32 CBattlezAiController::RetargetIdleUnit(CGrunt* unit) {
    GruntzPlayer* recA = NULL;
    CBattlezAiController* cfgB = NULL;
    i32 cell = unit->ArrivalCell().m_x;
    if (cell >= 0 && cell < 4) {
        recA = &m_game->GetPlayer(cell);
        cfgB = recA->GetBattlezAiController();
    }
    if (unit->CoordsEmpty()) {
        if (cell == -1) {
            if (static_cast<u32>(unit->GetDwell()) <= static_cast<u32>(m_moveBudget)) {
                return 1;
            }
            i32 r = GetRandom(3);
            if (r == m_playerIndex) {
                r++;
            }
            i32 band = r % 4;
            CBattlezAiController* b = m_game->GetPlayer(band).GetBattlezAiController();
            if (b != NULL) {
                i32 cnt = b->GetAttackWaypointCount();
                Coord goal = b->GetBaseTile();
                if (cnt != 0) {
                    Coord* pair = b->GetAttackWaypoint(rand() % cnt);
                    goal = *pair;
                }
                if (unit->MoveToTile(goal.m_x, goal.m_y, 0, 0x9cf, 0, 0x4020) != 0) {
                    unit->m_arrivalCell.Set(band, 0);
                    AcceptAlways(unit);
                }
            }
            unit->ResetDwell();
            return 1;
        }
        CBattlezAiController* recB = m_game->GetPlayer(cell).GetBattlezAiController();
        if (recB == NULL) {
            return 1;
        }
        if (static_cast<u32>(unit->GetDwell()) <= 0x7d0) {
            return 1;
        }

        i32 y = recB->GetBaseTile().m_y;
        i32 x = recB->GetBaseTile().m_x;
        unit->MoveToTile(x, y, 0, 0x987, 0, 0x4068);
        unit->ResetDwell();
        return 1;
    }
    if (recA == NULL || cfgB == NULL) {
        UNSET_COORD(unit->m_arrivalCell);
        return 1;
    }
    if (recA->IsHumanControlled() == false && cfgB->m_active == false) {
        unit->RecycleCoords();
        UNSET_COORD(unit->m_arrivalCell);
        return 1;
    }
    if (unit->ArrivalCell().m_y == 1) {
        return 1;
    }
    CGameObject* lvl = unit->GetSpriteObject();
    i32 px = lvl->m_screenX >> TILE_SHIFT_PX;
    i32 py = lvl->m_screenY >> TILE_SHIFT_PX;
    i32 nearBand = 0;

    i32 cnt2 = cfgB->GetAttackWaypointCount();
    if (cnt2 > 0) {
        for (i32 j = 0; j < cnt2; j++) {
            Coord* pair = cfgB->GetAttackWaypoint(j);
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
