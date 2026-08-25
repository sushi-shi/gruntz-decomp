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
// cl folds the second `occ == NULL` test that retail re-emits after the
// GruntInRadius/m_entranceCommitted guards, so we are two branches short.
RVA(0x000f2b20, 0x6e1)
i32 CGrunt::StepArrivalDefense() {
    m_defenderPx = m_lastTilePx;
    CGrunt* occ;
    switch (m_defenderState) {
        case AISTATE_ATTACK:
            if (m_poweredUp == 0) {
                m_defenderState = AISTATE_CHASE;
                return 1;
            }
            occ = m_tileMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (occ == NULL) {
                goto seek;
            }
            // the CHASE-and-shout arm is the FALL-THROUGH: forward gotos would
            // hoist it above the scroll arm, which retail emits first.
            if (GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0
                && occ->m_entranceCommitted != 0) {
                if (m_neighborValid != 0) {
                    return 1;
                }
                if (m_combatActive != 0) {
                    return 1;
                }
                if (m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0
                    && GRUNT_AT_SAVED_SCREEN_POS(occ)) {
                    if (m_vehiclePickupType == PICKUP_SCROLL) {
                        g_gameReg->m_cmdGrid->ApplyTriggerB(
                            m_playerIndex,
                            m_unitIndex,
                            occ->m_object->m_screenX,
                            occ->m_object->m_screenY
                        );
                        return 1;
                    }
                    COMMIT_GRUNT_NEIGHBOR(occ);
                    return 1;
                }
            } else if (occ == NULL) {
                goto seek;
            }
            m_defenderState = AISTATE_CHASE;
            {
                CWwdGameObjectA* h = m_object;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInRect(rect, vx, vy)) {
                    g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case AISTATE_CHASE: {
            occ = m_tileMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
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
            if (GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) == 0) {
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
            if (m_vehiclePickupType == PICKUP_SCROLL) {
                g_gameReg->m_cmdGrid->ApplyTriggerB(
                    m_playerIndex,
                    m_unitIndex,
                    occ->m_object->m_screenX,
                    occ->m_object->m_screenY
                );
                m_defenderState = AISTATE_ATTACK;
                return 1;
            }
            if (GRUNT_AT_SAVED_SCREEN_POS(occ)) {
                COMMIT_GRUNT_NEIGHBOR(occ);
            }
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_SEEK:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (occ == NULL) {
                goto L_f308a;
            }
            if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(occ)
                && RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
                if (m_vehiclePickupType == PICKUP_SCROLL) {
                    g_gameReg->m_cmdGrid->ApplyTriggerB(
                        m_playerIndex,
                        m_unitIndex,
                        occ->m_object->m_screenX,
                        occ->m_object->m_screenY
                    );
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
            if (occ != NULL && static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                if (GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) == 0) {
                    goto L_f318a;
                }
                {
                    Coord sp;
                    occ->GetScreenPos(&sp);
                    sp.m_y >>= TILE_SHIFT_PX;
                    sp.m_x >>= TILE_SHIFT_PX;
                    if (TileSwitch(sp.m_x, sp.m_y, 0, m_arrivalFlags, 1, 0) == 0) {
                        goto L_f318a;
                    }
                    SET_GRUNT_ARRIVAL_TARGET(occ);
                    m_defenderState = AISTATE_CHASE;
                    CWwdGameObjectA* h = m_object;
                    CGruntzMgr* reg = g_gameReg;
                    const RECT* rect = &reg->m_world->m_level->m_mainPlane->m_viewRect;
                    if (CGameLevel::PointInBounds(rect, h->m_screenX, h->m_screenY) == 0) {
                        goto L_f318a;
                    }
                    reg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
            L_f318a:
                m_dwell = 0;
                return 1;
            }
        L_f308a:
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_hasExtent == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }
            // retail lays the reroll arm LAST: the window-still-open arm is the
            // fall-through of the negated test.
            if (IsArrivalRerollPending() != 0) {
                CWwdGameObjectA* h = m_object;
                SELECT_RANDOM_EXTENT_POINT(h, outX, spanX, outY, spanY)
                if (outX < g_gameReg->m_tileGrid->m_width
                    && outY < g_gameReg->m_tileGrid->m_height) {
                    TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
                return 1;
            }
            ResetArrivalReroll();
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
seek:
    m_defenderState = AISTATE_SEEK;
    return 1;
}
