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
#include <Gruntz/GruntArrivalRerollMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
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

RVA(0x000f8240, 0x5b9)
i32 CGrunt::StepArrivalDefenseLean() {
    m_defenderPx = m_lastTilePx;
    bool eqI = ANIMATION_ACT_EQUALS("I");
    if (eqI) {
        return 1;
    }
    CGrunt* occ;
    switch (m_defenderState) {
        case AISTATE_ATTACK:
            if (m_poweredUp == 0) {
                m_defenderState = AISTATE_CHASE;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (occ != NULL && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0
                && occ->m_entranceCommitted != 0) {
                if (m_combatActive != 0) {
                    return 1;
                }
                if (m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0
                    && GRUNT_AT_SAVED_SCREEN_POS(occ)) {
                    COMMIT_GRUNT_NEIGHBOR_COPY(occ, cp);
                    return 1;
                }
                {
                    CWwdGameObjectA* h = m_object;
                    i32 vx = h->m_screenX;
                    i32 vy = h->m_screenY;
                    const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                    if (CGameLevel::PointInRect(rect, vx, vy)) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                    }
                }
                m_defenderState = AISTATE_CHASE;
                m_dwell = DWELL_REPATH_MS;
                return 1;
            }
            if (occ == NULL) {
                goto seek;
            }
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            {
                CWwdGameObjectA* h = m_object;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInRect(rect, vx, vy)) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case AISTATE_CHASE: {
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != NULL && g != occ) {
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (occ == NULL) {
                goto seek;
            }
            if (occ->m_entranceCommitted == 0) {
                goto seek;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto seek;
            }
            if (static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
                StepArrivalDrop(
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                );
                m_dwell = 0;
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
            COMMIT_GRUNT_NEIGHBOR_COPY(occ, cp);
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_SEEK:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (rand() % 0x64 == 0 && m_health > 0x1a && occ != NULL && m_stamina >= STAMINA_FULL
                && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
                m_tileMgr->ApplyTriggerA(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    m_lastTilePx.m_x,
                    m_lastTilePx.m_y
                );
                return 1;
            }
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_hasExtent == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }
            if (IsArrivalRerollPending() != 0) {
                {
                    CWwdGameObjectA* h = m_object;
                    SELECT_RANDOM_EXTENT_POINT_SIGNED_OUTPUT(
                        h,
                        baseX,
                        spanX,
                        baseY,
                        spanY,
                        outX,
                        outY
                    )
                    CMapMgr* bd = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(outX) < static_cast<u32>(bd->m_width)
                        && static_cast<u32>(outY) < static_cast<u32>(bd->m_height)) {
                        TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                    }
                    i32 m328 = CoordCount();
                    if (m328 != 0) {
                        i32 mx = spanX > spanY ? spanX : spanY;
                        if (m328 > mx) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
                m_dwell = 0;
                return 1;
            }
            RESET_GRUNT_ARRIVAL_REROLL_COMPACT
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
seek:
    m_defenderState = AISTATE_SEEK;
    return 1;
}
