#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

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
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
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
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000f8240, 0x5b9)
i32 CGrunt::StepMagicWandGruntBehavior() {
    m_defenderPx = m_lastTilePx;
    bool eqI = IsAnimationAct("I");
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
            occ = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            if (occ != NULL && GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0
                && occ->m_entranceCommitted != false) {
                if (m_combatActive != false) {
                    return 1;
                }
                if (m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0
                    && IsGruntAtSavedScreenPos(occ)) {
                    COMMIT_GRUNT_NEIGHBOR(occ);
                    return 1;
                }
                PLAY_VOICE_IN_VIEW(0x366);
                m_defenderState = AISTATE_CHASE;
                m_dwell = DWELL_REPATH_MS;
                return 1;
            }
            if (occ == NULL) {
                goto seek;
            }
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            PLAY_VOICE_IN_VIEW(0x366);
            return 1;

        case AISTATE_CHASE: {
            occ = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            CGrunt* g = m_triggerMgr->FindNearestEnemy(this);
            if (g != NULL && g != occ) {
                ResetToSeek(this);
                return 1;
            }
            if (occ == NULL || occ->m_entranceCommitted == false
                || GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) == 0) {
                goto seek;
            }
            RepathToward(this, occ);
            if (m_poweredUp != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
                return 1;
            }
            if (!IsGruntAtSavedScreenPos(occ)) {
                return 1;
            }
            COMMIT_GRUNT_NEIGHBOR(occ);
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_SEEK:
            occ = m_triggerMgr->FindNearestEnemy(this);
            if (rand() % 100 == 0 && m_health > 0x1a && occ != NULL && m_stamina >= STAMINA_FULL
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
                    SELECT_RANDOM_EXTENT_POINT_SEPARATE_BASE(
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
                    i32 coordCount = CoordCount();
                    if (coordCount != 0) {
                        i32 mx = Max(spanX, spanY);
                        if (coordCount > mx) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
                m_dwell = 0;
                return 1;
            }
            ResetEntranceAnimation(1, 1, 0);
            m_arrivalRerollTiming.Start(rand() % 0x7530 + 0x7530);
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
seek:
    m_defenderState = AISTATE_SEEK;
    return 1;
}
