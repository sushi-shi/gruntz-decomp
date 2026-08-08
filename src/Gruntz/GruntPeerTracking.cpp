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
RVA(0x000f7d90, 0x171)
i32 CGrunt::StepPeerTracking() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    if (m_vehiclePickupType == PICKUP_NONE) {
        m_arrivalState = AI_POSTGUARD;
        m_defenderState = AISTATE_SEEK;
        m_dwell = 0;
        return 1;
    }
    CGrunt* p = m_tileMgr->FindNearestEnemy(this);
    if (p == NULL) {
        return 1;
    }
    if (p->m_entranceCommitted == 0) {
        return 1;
    }
    CGameObject* a = p->m_object;
    if (a->m_screenX == p->m_lastTilePx.m_x && a->m_screenY == p->m_lastTilePx.m_y
        && RectContainsGated(a->m_screenX, a->m_screenY)) {
        CGameObject* b = p->m_object;
        g_gameReg->m_cmdGrid
            ->ApplyTriggerB(m_tileOwnerHi, m_tileOwnerLo, b->m_screenX, b->m_screenY);
        return 1;
    }
    if (static_cast<u32>(m_dwell) <= DWELL_SEEK_PATH_MS) {
        return 1;
    }
    if (GruntInRadius(p->m_tileOwnerHi, p->m_tileOwnerLo)) {
        CGameObject* b = p->m_object;
        TileSwitch(
            b->m_screenX >> TILE_SHIFT_PX,
            b->m_screenY >> TILE_SHIFT_PX,
            0,
            m_arrivalFlags,
            1,
            0
        );
        m_dwell = 0;
        if (m_blockedVoicePending == 0) {
            return 1;
        }
        CWwdGameObjectA* c = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 y = c->m_screenY;
        i32 x = c->m_screenX;
        CDDrawWorkerHost* r = g->m_world->m_level->m_mainPlane;
        if (CGameLevel::PointInRect(&r->m_viewRect, x, y)) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
        }
    }
    m_blockedVoicePending = 0;
    return 1;
}
