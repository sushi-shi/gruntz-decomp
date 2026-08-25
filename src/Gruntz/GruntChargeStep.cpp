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
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
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

// @early-stop
// Three cl-vs-cl differences, none of them steerable from source so far (80 of retail's
// 83 blocks):
//   * the `stamina < FULL` arm's `if (m_neighborValid != 0) return 1;` is deleted here -
//     cl knows the member is 0 from the entry test at 0xef724 - while retail keeps the
//     compare against the eax that load left behind (0xef7bc);
//   * the powered-up block's two ResetEntranceAnimation tails are byte-identical in
//     retail (0xef77c and 0xef7c4) and cl cross-jumps ours into one;
//   * conversely retail SHARES the SEEK and ATTACK CommitNeighbor tails (0xef8?? jmps
//     into B58) where cl emits both.
RVA(0x000ef6b0, 0x61d)
i32 CGrunt::StepDumbChaserBehavior() {
    m_defenderPx = m_lastTilePx;
    CGrunt* g = m_triggerMgr->FindNearestEnemy(this);
    i32 hitGate = 0;
    if (g != NULL) {
        CGameObject* gp = g->m_object;
        if (GRUNT_OBJECT_AT_SAVED_SCREEN_POS(gp, g) && RectContains(gp->m_screenX, gp->m_screenY)) {
            hitGate = 1;
        }
    }

    i32 powered = m_poweredUp;
    if (powered != 0) {
        if (m_neighborValid == 0) {
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    return 1;
                }
                if (hitGate != 0 && g == NULL) {
                    return 1;
                }
                if (m_poweredUp == 0) {
                    return 1;
                }
                if (m_neighborValid != 0) {
                    return 1;
                }
                RESET_GRUNT_POWERED_STATE(this)
                return 1;
            }
            if (hitGate != 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            RESET_GRUNT_POWERED_STATE(this)
            return 1;
        }
        m_neighborValid = 0;
        return 1;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK: {

            if (g != NULL) {
                if (hitGate != 0 && m_stamina >= STAMINA_FULL) {
                    CGameObject* gp = g->m_object;
                    if (GRUNT_OBJECT_AT_SAVED_SCREEN_POS(gp, g)
                        && RectContains(gp->m_screenX, gp->m_screenY)) {
                        COMMIT_GRUNT_NEIGHBOR(g);
                        return 1;
                    }
                }
                if (static_cast<u32>(m_dwell) > 500) {
                    if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
                        return 1;
                    }
                    if (TileSwitch(
                            g->m_object->m_screenX >> TILE_SHIFT_PX,
                            g->m_object->m_screenY >> TILE_SHIFT_PX,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        )
                        != 0) {
                        SET_GRUNT_ARRIVAL_TARGET(g);
                        m_defenderState = AISTATE_CHASE;
                        CWwdSpriteObject* mp = m_object;
                        CGruntzMgr* mgr = g_gameReg;

                        i32 los = CGameLevel::PointInBounds(
                            &mgr->m_world->m_level->m_mainPlane->m_planeViewRect,
                            mp->m_screenX,
                            mp->m_screenY
                        );
                        if (los != 0) {
                            mgr->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            if (m_resetApplied == 0 && m_hasExtent != 0 && static_cast<u32>(m_dwell) > 3000) {
                CWwdSpriteObject* mp = m_object;
                SELECT_RANDOM_EXTENT_POINT(mp, baseX, spanX, baseY, spanY)
                CGruntzMgr* mgr = g_gameReg;
                if (static_cast<u32>(baseX) < static_cast<u32>(mgr->m_tileGrid->m_width)
                    && static_cast<u32>(baseY) < static_cast<u32>(mgr->m_tileGrid->m_height)) {
                    TileSwitch(baseX, baseY, 0, m_arrivalFlags, 1, 0);
                }
                if (m_coordList.GetCount() != 0) {
                    if (spanX <= spanY) {
                        spanX = spanY;
                    }
                    if (m_coordList.GetCount() > spanX) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
            }
            break;
        }
        case AISTATE_CHASE: {

            CGrunt* t =
                m_triggerMgr->m_units[m_arrivalCell.m_y + m_arrivalCell.m_x * TM_UNITS_PER_PLAYER];
            CGrunt* cur = m_triggerMgr->FindNearestEnemy(this);
            if (cur != NULL && cur != t) {
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (t == NULL || t->m_entranceCommitted == 0
                || GruntInRadius(t->m_playerIndex, t->m_unitIndex) == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 500) {
                StepArrivalDrop(t->m_lastTilePx.m_x, t->m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL
                && RectContains(t->m_object->m_screenX, t->m_object->m_screenY) != 0
                && GRUNT_AT_SAVED_SCREEN_POS(t)) {
                COMMIT_GRUNT_NEIGHBOR(t);
                m_defenderState = AISTATE_ATTACK;
                return 1;
            }
            break;
        }
        case AISTATE_ATTACK: {

            if (m_poweredUp != 0) {
                CGrunt* t =
                    m_triggerMgr
                        ->m_units[m_arrivalCell.m_y + m_arrivalCell.m_x * TM_UNITS_PER_PLAYER];
                if (t == NULL || GruntInRadius(t->m_playerIndex, t->m_unitIndex) == 0
                    || t->m_entranceCommitted == 0) {
                    m_defenderState = AISTATE_CHASE;
                    m_dwell = DWELL_REPATH_MS;
                    return 1;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(t->m_object->m_screenX, t->m_object->m_screenY) == 0
                    || GRUNT_NOT_AT_SAVED_SCREEN_POS(t)) {
                    m_defenderState = AISTATE_CHASE;
                    m_dwell = DWELL_REPATH_MS;
                    return 1;
                }
                COMMIT_GRUNT_NEIGHBOR(t);
                return 1;
            }
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            return 1;
        }
    }
    return 1;
}
