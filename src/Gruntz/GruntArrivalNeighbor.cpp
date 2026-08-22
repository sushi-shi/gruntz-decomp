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
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
// regalloc: retail spills x/y into its `sub esp,0x8` frame and rematerialises them for
// the push (an un-CSE'd second load of +0x17c) - a spill pair, not a source local.
// docs/patterns/dead-eight-byte-coord-temp-is-unreproduced.md
RVA(0x000f26f0, 0x106)
i32 CGrunt::ResolveArrivalNeighbor() {
    switch (m_defenderState) {
        case AISTATE_SEEK:
            return 1;
        case AISTATE_ATTACK:
            break;
        default:
            return 1;
    }

    if (m_poweredUp == 0) {
        m_defenderState = AISTATE_SEEK;
    }
    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina < STAMINA_FULL) {
            return 1;
        }
        FindGridNeighbor(1);
        return 1;
    }

    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    if (occ == NULL) {
        return 1;
    }
    if (m_poweredUp != 0) {
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
    COMMIT_GRUNT_NEIGHBOR_COPY(occ, tile);
    return 1;
}
