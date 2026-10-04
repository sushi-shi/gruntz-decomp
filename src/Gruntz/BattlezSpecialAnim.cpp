#include <StdAfx.h>

#include <rva.h>

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
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordInline.h>
#include <Gruntz/GruntMovementMacros.h>
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
#include <stdlib.h>
#include <string.h>

RVA(0x00034460, 0x3fc)
i32 CBattlezAiController::CanIssueAiOrders(CGrunt* unit) {
    if (unit == NULL) {
        return 0;
    }
    CGameObject* object = unit->m_object;
    if (GRUNT_SCREEN_X_NOT_AT_SAVED_POS(object, unit)) {
        goto fail;
    }
    if (GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(object, unit)) {
        return 0;
    }
    if (unit->IsEntranceCommitted() == false) {
        return 0;
    }
    if (unit->IsDeathAnimationStarted() != false) {
        return 0;
    }
    if (unit->IsBusy() != false) {
        return 0;
    }
    if (unit->IsInCombat() != false) {
        return 0;
    }

    if (unit->IsAnimationAct("I")) {
        return 0;
    }
    if (unit->IsAnimationAct("G")) {
        return 0;
    }
    if (unit->IsAnimationAct("L")) {
        return 0;
    }

    CString* actName;
    CString* finalActName;
    i32 actCode;

    actName = &g_typeColl[unit->m_logicRecord->m_eventCode];
    if (*actName == "P") {
        return 0;
    }

    actName = &g_typeColl[unit->m_logicRecord->m_eventCode];
    if (*actName == "J") {
        return 0;
    }

    actName = &g_typeColl[unit->m_logicRecord->m_eventCode];
    if (*actName == "C") {
        goto fail;
    }

    actCode = unit->m_logicRecord->EventCode();
    finalActName = &g_typeColl[actCode];
    return *finalActName != "R";
fail:
    return 0;
}

RVA_COMPGEN(0x00034960, 0x24, ?handle_inl@zErrHandling@@QBEXPBDH@Z)
