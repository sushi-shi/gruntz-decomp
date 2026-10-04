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
#include <Gruntz/GruntCoordRecycleMacros.h>
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

RVA(0x000f0130, 0x7c0)
i32 CGrunt::StepGauntletGruntBehavior() {
    if (IsAnimationAct("I")) {
        return 1;
    }
    this->m_defenderPx = this->m_lastTilePx;
    FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget)

    b32 inCombat = this->m_inCombat;
    if (inCombat != false) {
        b32 attackQueued = this->m_attackQueued;
        if (attackQueued == false) {
            if (this->m_attackWindupActive != false) {
                return 1;
            }
            if (this->m_stamina >= STAMINA_FULL) {
                if (TryAttackRememberedTarget(1) != NULL) {
                    return 1;
                }
                if (atTarget && g == NULL) {
                    return 1;
                }
                if (this->m_inCombat == false) {
                    return 1;
                }
                if (this->m_attackQueued != false) {
                    return 1;
                }
                this->m_busy = false;
                this->m_attackWindupActive = false;
                this->m_attackQueued = false;
                this->m_inCombat = false;
                ResetIdleAnimation(1, 0, 0);
                return 1;
            }
            if (atTarget) {
                return 1;
            }
            if (this->m_inCombat == false) {
                return 1;
            }
            if (this->m_attackQueued != false) {
                return 1;
            }
            this->m_busy = false;
            this->m_attackWindupActive = false;
            this->m_attackQueued = false;
            this->m_inCombat = false;
            ResetIdleAnimation(1, 0, 0);
            return 1;
        }
        this->m_attackQueued = false;
        return 1;
    }

    switch (this->m_aiState) {
        case AISTATE_SEEK: {
            Coord c;
            if (g != NULL && this->m_inCombat == false && this->m_stamina >= STAMINA_FULL
                && IsGruntAtSavedScreenPos(g)
                && IsWithinReach(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
                ATTACK_GRUNT(g);
                break;
            }
            if (g != NULL && static_cast<u32>(this->m_dwell) > 1000) {
                if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) != 0) {
                    g->GetScreenPos(&c);
                    if (MoveToTile(
                            c.m_x >> TILE_SHIFT_PX,
                            c.m_y >> TILE_SHIFT_PX,
                            0,
                            this->m_arrivalFlags,
                            0,
                            0x20
                        )
                        != 0) {
                        SET_GRUNT_ARRIVAL_TARGET(g);
                        this->m_aiState = AISTATE_CHASE;
                        PLAY_VOICE_IF_VISIBLE(0x366);
                    }
                }
                this->m_dwell = 0;
                break;
            }
            if (this->m_idleVariantActive == false && this->m_hasExtent != false
                && static_cast<u32>(this->m_dwell) > 3000) {
                if (IsArrivalRerollPending() != 0) {
                    CGameObject* base = this->m_object;
                    SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(base, lo, ax, lo2, ay)
                    if (lo < g_gameReg->GetTileGrid()->GetWidth()
                        && lo2 < g_gameReg->GetTileGrid()->GetHeight()) {
                        MoveToTile(
                            static_cast<i32>(lo),
                            static_cast<i32>(lo2),
                            0,
                            this->m_arrivalFlags,
                            1,
                            0
                        );
                    }
                    if (!this->CoordsEmpty()) {
                        ax = Max(ax, ay);
                        if (this->CoordCount() > ax) {
                            SetEntrancePos(1, 1);
                        }
                    }
                } else {
                    ResetArrivalReroll();
                }
                this->m_dwell = 0;
            }
            break;
        }
        case AISTATE_CHASE: {
            CGrunt* slot = m_triggerMgr->UnitAt(this->m_arrivalCell.m_x, this->m_arrivalCell.m_y);
            CGrunt* found = m_triggerMgr->FindNearestEnemy(this);
            if (found == NULL || found == slot) {
                if (slot == NULL || slot->IsEntranceCommitted() == false
                    || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0) {
                    this->m_aiState = AISTATE_SEEK;
                } else {
                    MoveTo(
                        slot->m_lastTilePx.m_x,
                        slot->m_lastTilePx.m_y,
                        0,
                        this->m_arrivalFlags,
                        0,
                        0x20
                    );
                    if (this->m_inCombat == false && this->m_stamina >= STAMINA_FULL
                        && IsWithinReach(slot->m_object->m_screenX, slot->m_object->m_screenY) != 0
                        && IsGruntAtSavedScreenPos(slot)) {
                        ATTACK_GRUNT(slot);
                        this->m_aiState = AISTATE_ATTACK;
                    }
                }
            } else {
                ResetToSeek();
            }
            break;
        }
        case AISTATE_ATTACK: {
            if (m_inCombat == false) {
                m_aiState = AISTATE_CHASE;
                break;
            }
            CGrunt* slot = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            if (slot != NULL && GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) != 0
                && slot->IsEntranceCommitted() != false) {
                if (m_attackQueued != false || m_attackWindupActive != false
                    || m_stamina < STAMINA_FULL) {
                    break;
                }
                if (IsWithinReach(slot->m_object->m_screenX, slot->m_object->m_screenY) != 0
                    && IsGruntAtSavedScreenPos(slot)) {
                    ATTACK_GRUNT(slot);
                    break;
                }
            } else if (slot == NULL) {
                m_aiState = AISTATE_SEEK;
                break;
            }
            m_aiState = AISTATE_CHASE;
            PLAY_VOICE_IN_VIEW(0x366);
            break;
        }
    }

    if (!this->CoordsEmpty()) {

        Coord* cell = GetHeadCoord();

        BrickzCell& gc = g_gameReg->GetTileGrid()->CellAtUnchecked(cell->m_x, cell->m_y);
        if ((gc.m_flagBytes[0] & 0x20) != 0) {
            SetEntrancePos(1, 1);
            if (!this->CoordsEmpty()) {
                this->RecycleCoords();
            }
            g_gameReg->GetTriggerMgr()->UseEquippedToolAt(
                m_playerIndex,
                m_unitIndex,
                cell->m_x * 0x20 + 0x10,
                cell->m_y * 0x20 + 0x10
            );
        }
    }
    return 1;
}
