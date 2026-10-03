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
i32 CBattlezMapConfig::CanPlaySpecialAnim(CGrunt* unit) {
    if (unit == NULL) {
        return 0;
    }
    CGameObject* lvl = unit->m_object;
    if (GRUNT_SCREEN_X_NOT_AT_SAVED_POS(lvl, unit)) {
        goto fail;
    }
    if (GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(lvl, unit)) {
        return 0;
    }
    if (unit->m_entranceCommitted == false) {
        return 0;
    }
    if (unit->IsDeathAnimationStarted() != false) {
        return 0;
    }
    if (unit->m_entranceActive != false) {
        return 0;
    }
    if (unit->m_poweredUp != false) {
        return 0;
    }

    bool eq;
    eq = unit->IsAnimationAct("I");
    if (eq) {
        return 0;
    }
    eq = unit->IsAnimationAct("G");
    if (eq) {
        return 0;
    }
    eq = unit->IsAnimationAct("L");
    if (eq) {
        return 0;
    }

    CString* recs;
    CString* sel;
    i32 ci;

    recs = &g_typeColl[unit->m_logicRecord->m_eventCode];
    eq = (*recs == "P");
    if (eq) {
        return 0;
    }

    recs = &g_typeColl[unit->m_logicRecord->m_eventCode];
    eq = (*recs == "J");
    if (eq) {
        return 0;
    }

    recs = &g_typeColl[unit->m_logicRecord->m_eventCode];
    eq = (*recs == "C");
    if (eq) {
        goto fail;
    }

    ci = unit->m_logicRecord->EventCode();
    sel = &g_typeColl[ci];
    eq = (*sel == "R");
    return !eq;
fail:
    return 0;
}

RVA_COMPGEN(0x00034960, 0x24, ?handle_inl@zErrHandling@@QBEXPBDH@Z)
