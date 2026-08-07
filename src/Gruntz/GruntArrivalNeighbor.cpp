#include <Enums.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h>
#include <stdlib.h>
#include <Gruntz/FreeNodePool.h>
#include <MfcWin.h>
#include <new>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <rva.h>
#include <Gruntz/GruntDirStatics.h>

#pragma intrinsic(strcmp)

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/ScanGridMacros.h>
#include <limits.h>

// @early-stop

// @early-stop
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
    if (m_object->m_screenX != occ->m_lastTilePx.m_x) {
        return 1;
    }
    if (m_object->m_screenY != occ->m_lastTilePx.m_y) {
        return 1;
    }
    Coord tile = occ->m_lastTilePx;
    CommitNeighbor(
        occ->m_tileOwnerHi,
        occ->m_tileOwnerLo,
        occ->m_lastTilePx.m_x,
        occ->m_lastTilePx.m_y
    );
    return 1;
}
