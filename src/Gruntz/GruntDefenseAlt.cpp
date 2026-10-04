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

RVA(0x000f1c70, 0x620)
i32 CGrunt::StepObjectGuardBehavior() {
    m_arrivalFlags |= 0x40000;
    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    i32 inRange = 0;
    if (occ != NULL && IsGruntAtSavedScreenPos(occ)
        && IsWithinReach(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
        inRange = 1;
    }

    b32 inCombat = m_inCombat;
    if (inCombat != false) {
        b32 attackQueued = m_attackQueued;
        if (attackQueued == false) {
            if (m_attackWindupActive != false) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (TryAttackRememberedTarget(1) != NULL) {
                    return 1;
                }
                if (inRange != 0 && occ == NULL) {
                    return 1;
                }
                if (m_inCombat == false) {
                    return 1;
                }
                if (m_attackQueued != false) {
                    return 1;
                }
                RESET_GRUNT_COMBAT_STATE(this)
                return 1;
            }
            if (inRange != 0) {
                return 1;
            }
            if (m_inCombat == false) {
                return 1;
            }
            if (m_attackQueued != false) {
                return 1;
            }
            RESET_GRUNT_COMBAT_STATE(this)
            return 1;
        }
        m_attackQueued = false;
        return 1;
    }

    switch (m_aiState) {
        case AISTATE_SEEK: {
            CGrunt* o = m_triggerMgr->FindNearestEnemy(this);
            if (o != NULL) {
                if (m_inCombat != false) {
                    return 1;
                }
                if (m_stamina >= STAMINA_FULL && IsGruntAtSavedScreenPos(o)
                    && IsWithinReach(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                    ATTACK_GRUNT(o);
                    return 1;
                }
            }
            if (m_inCombat != false) {
                return 1;
            }
            {
                Coord entrance = EntrancePx();
                Coord tile = LastTilePx();
                if (tile != entrance) {
                    return 1;
                }
            }
            {
                i32 gx = m_defenderPx.m_x >> TILE_SHIFT_PX;
                i32 gy = m_defenderPx.m_y >> TILE_SHIFT_PX;
                i32 tx = LastTilePx().m_x >> TILE_SHIFT_PX;
                i32 ty = LastTilePx().m_y >> TILE_SHIFT_PX;
                if (tx < gx && ty < gy) {
                    MoveTo(m_lastTilePx.m_x + 0x40, m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
                    return 1;
                }
                if (tx < gx && ty > gy) {
                    MoveTo(m_lastTilePx.m_x, m_lastTilePx.m_y - 0x40, 0, m_arrivalFlags, 1, 0);
                    return 1;
                }
                if (tx > gx && ty < gy) {
                    MoveTo(m_lastTilePx.m_x, m_lastTilePx.m_y + 0x40, 0, m_arrivalFlags, 1, 0);
                    return 1;
                }
                if (tx > gx && ty > gy) {
                    MoveTo(m_lastTilePx.m_x - 0x40, m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
                    return 1;
                }
                goto resetState;
            }
        }

        case AISTATE_CHASE: {
            CGrunt* o = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            CGrunt* g = m_triggerMgr->FindNearestEnemy(this);
            if (g != NULL && g != o) {
                ResetToSeek();
                return 1;
            }
            if (o == NULL || o->IsEntranceCommitted() == false
                || GruntInRadius(o->GetPlayerIndex(), o->GetUnitIndex()) == 0
                || GruntInRadius(m_arrivalCell.m_x, m_arrivalCell.m_y) == 0) {
                goto resetState;
            }
            MoveTo(o->m_lastTilePx.m_x, o->m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
            if (m_inCombat != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (IsWithinReach(o->m_object->m_screenX, o->m_object->m_screenY) == 0) {
                return 1;
            }
            if (!IsGruntAtSavedScreenPos(o)) {
                return 1;
            }
            ATTACK_GRUNT(o);
            m_aiState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_ATTACK:
            m_aiState = AISTATE_SEEK;
            return 1;

        case AISTATE_RETURN: {
            MoveTo(m_defenderPx.m_x - 0x20, m_defenderPx.m_y - 0x20, 0, m_arrivalFlags, 1, 0);
            if (m_object->m_screenX == m_defenderPx.m_x - 0x20
                && m_object->m_screenY == m_defenderPx.m_y - 0x20) {
                m_aiState = AISTATE_SEEK;
                return 1;
            }
            CGrunt* o = m_triggerMgr->FindNearestEnemy(this);
            if (o == NULL) {
                return 1;
            }
            if (m_inCombat == false && m_stamina >= STAMINA_FULL && IsGruntAtSavedScreenPos(o)
                && IsWithinReach(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                ATTACK_GRUNT(o);
                m_aiState = AISTATE_ATTACK;
            }
            if (GruntInRadius(o->GetPlayerIndex(), o->GetUnitIndex()) == 0) {
                return 1;
            }
            m_arrivalCell.m_x = o->GetPlayerIndex();
            m_arrivalCell.m_y = o->GetUnitIndex();
            m_aiState = AISTATE_CHASE;
            PLAY_VOICE_IN_VIEW(0x366);
            return 1;
        }

        default:
            return 1;
    }

resetState:
    m_aiState = AISTATE_RETURN;
    return 1;
}
