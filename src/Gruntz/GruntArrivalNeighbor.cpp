#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000f2820, 0x106)
i32 CGrunt::StepPostGuardBehavior() {
    switch (m_defenderState) {
        case AISTATE_SEEK:
            return 1;
        case AISTATE_ATTACK:
            break;
        default:
            return 1;
    }

    if (m_poweredUp == false) {
        m_defenderState = AISTATE_SEEK;
    }
    if (m_poweredUp != false) {
        if (m_neighborValid != false) {
            return 1;
        }
        if (m_combatActive != false) {
            return 1;
        }
        if (m_stamina < STAMINA_FULL) {
            return 1;
        }
        FindGridNeighbor(1);
        return 1;
    }

    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    if (occ == NULL) {
        return 1;
    }
    if (m_poweredUp != false) {
        return 1;
    }
    if (m_stamina < STAMINA_FULL) {
        return 1;
    }
    if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
        return 1;
    }
    if (occ->GRUNT_SCREEN_X_NOT_AT_SAVED_POS(m_object, occ)) {
        return 1;
    }
    if (occ->GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(m_object, occ)) {
        return 1;
    }
    COMMIT_GRUNT_NEIGHBOR(occ);
    return 1;
}
