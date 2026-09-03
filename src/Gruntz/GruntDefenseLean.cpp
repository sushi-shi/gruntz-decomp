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
#include <Lith/ObjectUtilities.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000f8240, 0x5b9)
i32 CGrunt::StepMagicWandGruntBehavior() {
    m_defenderPx = m_lastTilePx;
    bool eqI = ANIMATION_ACT_EQUALS("I");
    if (eqI) {
        return 1;
    }
    CGrunt* occ;
    switch (m_defenderState) {
        case AISTATE_ATTACK:
            if (m_poweredUp == false) {
                m_defenderState = AISTATE_CHASE;
                return 1;
            }
            occ =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (occ != NULL && GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0
                && occ->m_entranceCommitted != false) {
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
                    && IsGruntAtSavedScreenPos(occ)) {
                    CommitGruntNeighbor(this, occ);
                    return 1;
                }
                {
                    Coord voicePosition = m_object->ScreenPos();
                    const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
                    if (::PtInRect(rect, voicePosition.m_x, voicePosition.m_y)) {
                        g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
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
            if (occ->m_object->ScreenPos() != occ->m_lastTilePx) {
                return 1;
            }
            CommitGruntNeighbor(this, occ);
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_SEEK:
            occ = m_triggerMgr->FindNearestEnemy(this);
            if (IsRandomChance(1) && m_health > 0x1a && occ != NULL && m_stamina >= STAMINA_FULL
                && GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0) {
                m_triggerMgr->UseEquippedToolAt(
                    m_playerIndex,
                    m_unitIndex,
                    m_lastTilePx.m_x,
                    m_lastTilePx.m_y
                );
                return 1;
            }
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
                {
                    CWwdSpriteObject* h = m_object;
                    Coord point;
                    Coord span;
                    SelectRandomExtentPoint(h, &point, &span);
                    CMapMgr* bd = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(point.m_x) < static_cast<u32>(bd->m_width)
                        && static_cast<u32>(point.m_y) < static_cast<u32>(bd->m_height)) {
                        TileSwitch(point.m_x, point.m_y, 0, m_arrivalFlags, 1, 0);
                    }
                    i32 m328 = CoordCount();
                    if (m328 != 0) {
                        if (m328 > Max(span.m_x, span.m_y)) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
                m_dwell = 0;
                return 1;
            }
            ResetEntranceAnimation(1, 1, 0);
            m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollLo = static_cast<i32>(g_frameTime);
            m_arrivalRerollHi = 0;
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
seek:
    m_defenderState = AISTATE_SEEK;
    return 1;
}
