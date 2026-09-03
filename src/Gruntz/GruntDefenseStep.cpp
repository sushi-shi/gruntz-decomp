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
#include <Gruntz/RandomExtentPoint.h>
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
RVA(0x000f2b20, 0x6e1)
i32 CGrunt::StepScrollGruntBehavior() {
    m_defenderPx = m_lastTilePx;
    CGrunt* occ;
    switch (m_defenderState) {
        case AISTATE_ATTACK:
            if (m_poweredUp == false) {
                m_defenderState = AISTATE_CHASE;
                return 1;
            }
            occ =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (occ == NULL) {
                goto seek;
            }
            if (GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0
                && occ->m_entranceCommitted != false) {
                if (m_neighborValid != false) {
                    return 1;
                }
                if (m_combatActive != false) {
                    return 1;
                }
                if (m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(
                        occ->m_object->m_screenPosition.m_x,
                        occ->m_object->m_screenPosition.m_y
                    ) != 0
                    && GRUNT_AT_SAVED_SCREEN_POS(occ)) {
                    if (m_vehiclePickupType == PICKUP_SCROLL) {
                        g_gameReg->m_triggerMgr->UseToyAt(
                            m_playerIndex,
                            m_unitIndex,
                            occ->m_object->m_screenPosition.m_x,
                            occ->m_object->m_screenPosition.m_y
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
                Coord voicePosition = m_object->ScreenPos();
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (::PtInRect(rect, voicePosition.m_x, voicePosition.m_y)) {
                    g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case AISTATE_CHASE: {
            occ =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            CGrunt* g = m_triggerMgr->FindNearestEnemy(this);
            if (g != NULL && g != occ) {
                m_arrivalCell.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (occ == NULL) {
                goto seek;
            }
            if (occ->m_entranceCommitted == false) {
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
            if (m_poweredUp != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(
                    occ->m_object->m_screenPosition.m_x,
                    occ->m_object->m_screenPosition.m_y
                )
                == 0) {
                return 1;
            }
            if (m_vehiclePickupType == PICKUP_SCROLL) {
                g_gameReg->m_triggerMgr->UseToyAt(
                    m_playerIndex,
                    m_unitIndex,
                    occ->m_object->m_screenPosition.m_x,
                    occ->m_object->m_screenPosition.m_y
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
            occ = m_triggerMgr->FindNearestEnemy(this);
            if (occ == NULL) {
                goto L_f308a;
            }
            if (m_poweredUp == false && m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(occ)
                && RectContains(
                       occ->m_object->m_screenPosition.m_x,
                       occ->m_object->m_screenPosition.m_y
                   ) != 0) {
                if (m_vehiclePickupType == PICKUP_SCROLL) {
                    g_gameReg->m_triggerMgr->UseToyAt(
                        m_playerIndex,
                        m_unitIndex,
                        occ->m_object->m_screenPosition.m_x,
                        occ->m_object->m_screenPosition.m_y
                    );
                    return 1;
                }
                if (occ->m_object->ScreenPos() != occ->m_lastTilePx) {
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
                    occ->GetScreenTile(&sp);
                    if (TileSwitch(sp.m_x, sp.m_y, 0, m_arrivalFlags, 1, 0) == 0) {
                        goto L_f318a;
                    }
                    SET_GRUNT_ARRIVAL_TARGET(occ);
                    m_defenderState = AISTATE_CHASE;
                    CWwdSpriteObject* h = m_object;
                    CGruntzMgr* reg = g_gameReg;
                    const RECT* rect = &reg->m_world->m_level->m_mainPlane->m_planeViewRect;
                    if (CGameLevel::PointInBounds(
                            rect,
                            h->m_screenPosition.m_x,
                            h->m_screenPosition.m_y
                        )
                        == 0) {
                        goto L_f318a;
                    }
                    reg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
            L_f318a:
                m_dwell = 0;
                return 1;
            }
        L_f308a:
            if (m_resetApplied != false) {
                return 1;
            }
            if (m_hasExtent == false) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }
            if (IsArrivalRerollPending() != 0) {
                CWwdSpriteObject* h = m_object;
                Coord point;
                Coord span;
                SelectRandomExtentPoint(h, &point, &span);
                if (static_cast<u32>(point.m_x) < g_gameReg->m_tileGrid->m_width
                    && static_cast<u32>(point.m_y) < g_gameReg->m_tileGrid->m_height) {
                    TileSwitch(point.m_x, point.m_y, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    if (m328 > Max(span.m_x, span.m_y)) {
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
