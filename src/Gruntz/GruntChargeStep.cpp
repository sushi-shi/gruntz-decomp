#include <StdAfx.h>

#include <rva.h>

#include <Enums.h>
#include <Globals.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/RandomExtentPoint.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000ef6b0, 0x61d)
i32 CGrunt::StepDumbChaserBehavior() {
    m_defenderPx = m_lastTilePx;
    CGrunt* g = m_triggerMgr->FindNearestEnemy(this);
    b32 hitGate = false;
    if (g != NULL) {
        CGameObject* gp = g->m_object;
        if (GRUNT_OBJECT_AT_SAVED_SCREEN_POS(gp, g)
            && RectContains(gp->m_screenPosition.m_x, gp->m_screenPosition.m_y)) {
            hitGate = true;
        }
    }

    b32 powered = m_poweredUp;
    if (powered != false) {
        b32 neighborValid = m_neighborValid;
        if (neighborValid == false) {
            if (m_combatActive != false) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    return 1;
                }
                if (hitGate != false && g == NULL) {
                    return 1;
                }
                if (m_poweredUp == false) {
                    return 1;
                }
                if (m_neighborValid != false) {
                    return 1;
                }
                RESET_GRUNT_POWERED_STATE(this)
                return 1;
            }
            if (hitGate != false) {
                return 1;
            }
            if (m_poweredUp == false) {
                return 1;
            }
            if (m_neighborValid != false) {
                return 1;
            }
            RESET_GRUNT_POWERED_STATE(this)
            return 1;
        }
        m_neighborValid = false;
        return 1;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK: {

            if (g != NULL && m_poweredUp == false && m_stamina >= STAMINA_FULL
                && IsGruntAtSavedScreenPos(g)
                && RectContains(
                       g->m_object->m_screenPosition.m_x,
                       g->m_object->m_screenPosition.m_y
                   ) != 0) {
                COMMIT_GRUNT_NEIGHBOR(g);
                return 1;
            }
            if (g != NULL && static_cast<u32>(m_dwell) > 500) {
                if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
                    return 1;
                }
                if (TileSwitch(
                        g->m_object->m_screenPosition.m_x >> TILE_SHIFT_PX,
                        g->m_object->m_screenPosition.m_y >> TILE_SHIFT_PX,
                        0,
                        m_arrivalFlags,
                        1,
                        0
                    )
                    != 0) {
                    SET_GRUNT_ARRIVAL_TARGET(g);
                    m_defenderState = AISTATE_CHASE;
                    PLAY_VOICE_IF_VISIBLE(0x366);
                }
                m_dwell = 0;
                return 1;
            }
            if (m_resetApplied == false && m_hasExtent != false
                && static_cast<u32>(m_dwell) > 3000) {
                CWwdSpriteObject* mp = m_object;
                SELECT_RANDOM_EXTENT_POINT(mp, baseX, spanX, baseY, spanY)
                if (static_cast<u32>(baseX) < static_cast<u32>(g_gameReg->m_tileGrid->m_width)
                    && static_cast<u32>(baseY)
                           < static_cast<u32>(g_gameReg->m_tileGrid->m_height)) {
                    TileSwitch(baseX, baseY, 0, m_arrivalFlags, 1, 0);
                }
                if (m_coordList.GetCount() != 0) {
                    spanX = Max(spanX, spanY);
                    if (m_coordList.GetCount() > spanX) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
            }
            break;
        }
        case AISTATE_CHASE: {

            CGrunt* t = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            CGrunt* cur = m_triggerMgr->FindNearestEnemy(this);
            if (cur != NULL && cur != t) {
                ResetToSeek(this);
                return 1;
            }
            if (t == NULL || t->m_entranceCommitted == false
                || GruntInRadius(t->m_playerIndex, t->m_unitIndex) == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            RepathToward(this, t);
            if (m_poweredUp == false && m_stamina >= STAMINA_FULL
                && RectContains(
                       t->m_object->m_screenPosition.m_x,
                       t->m_object->m_screenPosition.m_y
                   ) != 0
                && IsGruntAtSavedScreenPos(t)) {
                COMMIT_GRUNT_NEIGHBOR(t);
                m_defenderState = AISTATE_ATTACK;
                return 1;
            }
            break;
        }
        case AISTATE_ATTACK: {

            if (m_poweredUp != false) {
                CGrunt* t = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
                if (t == NULL || GruntInRadius(t->m_playerIndex, t->m_unitIndex) == 0
                    || t->m_entranceCommitted == false) {
                    m_defenderState = AISTATE_CHASE;
                    m_dwell = DWELL_REPATH_MS;
                    return 1;
                }
                if (m_neighborValid != false || m_combatActive != false
                    || m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(
                        t->m_object->m_screenPosition.m_x,
                        t->m_object->m_screenPosition.m_y
                    ) == 0
                    || !IsGruntAtSavedScreenPos(t)) {
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
