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
#include <Gruntz/GruntPickupInline.h>
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
#include <Lith/BDefs.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000f71c0, 0x721)
i32 CGrunt::StepToolThiefBehavior() {
    this->m_defenderPx = this->m_lastTilePx;
    if (!this->CoordsEmpty()
        && g_gameReg->GetTriggerMgr()->UnitAt(0, this->m_arrivalCell.m_x) == NULL) {
        this->RecycleCoords();
        this->m_arrivalCell.m_x = 0;
    }

    i32 reason = IDX(this->GetEquippedToolType());
    if (reason == 0 && (reason = this->m_arrivalCell.m_x, reason >= 0) && reason < 0xf) {
        CGrunt* slot = g_gameReg->GetTriggerMgr()->UnitAt(0, reason);
        if (slot == NULL || slot->IsEntranceCommitted() == false) {
            if (!this->CoordsEmpty()) {
                this->RecycleCoords();
            }
            this->m_arrivalCell.m_x = -1;
            return 1;
        }

        Coord slotTile;
        Coord selfTileX;
        Coord selfTileY;
        slot->GetScreenTile(&slotTile);
        i32 slotX = slotTile.m_x;
        selfTileY.m_y = slotTile.m_y;
        this->GetScreenPos(&selfTileX);
        i32 selfX = selfTileX.m_x >> TILE_SHIFT_PX;
        slot->GetScreenTile(&slotTile);
        selfTileX = slotTile;
        this->GetScreenPos(&selfTileY);
        selfTileY.m_y >>= TILE_SHIFT_PX;
        i32 dx = slotX - selfX;
        i32 dy = selfTileX.m_y - selfTileY.m_y;
        if (abs(dx) <= 1 && abs(dy) <= 1) {
            PickupType r2 = slot->GetEquippedToolType();
            if (r2 != PICKUP_WARPSTONE && r2 != PICKUP_BOMB) {
                this->ApplyPickup(r2, 1, 0, 0);
                slot->ApplyPickup(PICKUP_NONE, 1, 0, 0);
                this->m_aiState = AISTATE_COOLDOWN;
                if (this->CoordsEmpty()) {
                    return 1;
                }
                this->RecycleCoords();
                return 1;
            }
        }
    }

    reason = IDX(this->GetEquippedToolType());
    if (reason != 0) {
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
                    RESET_CURRENT_GRUNT_COMBAT_STATE
                    return 1;
                } else {
                    if (atTarget) {
                        return 1;
                    }
                    if (this->m_inCombat == false) {
                        return 1;
                    }
                    if (this->m_attackQueued != false) {
                        return 1;
                    }
                    RESET_CURRENT_GRUNT_COMBAT_STATE
                    return 1;
                }
            } else {
                this->m_attackQueued = false;
            }
            return 1;
        }
        this->m_defenderPx = this->m_lastTilePx;
        if (g == NULL || GruntInRadius(g->GetPlayerIndex(), g->GetUnitIndex()) == 0) {
            this->m_blockedVoicePending = false;
            return 1;
        }
        if (this->m_inCombat == false && this->m_stamina >= STAMINA_FULL) {
            i32 x = g->m_object->m_screenX;
            if (GRUNT_X_AT_SAVED_POS(x, g) && g->GRUNT_SCREEN_Y_AT_SAVED_POS(m_object, g)

                && IsWithinReach(x, g->m_object->m_screenY) != 0) {
                ATTACK_GRUNT(g);
            }
        }
        if (static_cast<u32>(this->m_dwell) <= DWELL_REPATH_MS) {
            return 1;
        }
        if (MoveToTile(
                g->m_object->m_screenX >> TILE_SHIFT_PX,
                g->m_object->m_screenY >> TILE_SHIFT_PX,
                0,
                this->m_arrivalFlags,
                1,
                0
            )
            == 0) {
            return 1;
        }
        if (this->m_blockedVoicePending != false) {
            PLAY_VOICE_IF_VISIBLE(0x366);
            this->m_blockedVoicePending = false;
            this->m_dwell = 0;
            return 1;
        }
    } else {
        if (this->CoordsEmpty()) {
            if (this->m_aiState != AISTATE_SEEK) {
                return 1;
            }
            i32 best = INT_MAX;
            i32 bestIdx = -1;
            CGrunt** slots = g_gameReg->GetTriggerMgr()->PlayerUnits(0);
            i32 i = 0;
            do {
                CGrunt* sv = slots[i];
                if (sv != NULL && sv->IsEntranceCommitted() != false) {
                    PickupType k = sv->GetActivePickupType();
                    if (EQUIPPED_TOOL_OF_TERNARY_LE(sv, k) != PICKUP_NONE
                        && EQUIPPED_TOOL_OF_TERNARY_LE(sv, k) != PICKUP_WARPSTONE
                        && EQUIPPED_TOOL_OF_TERNARY_LE(sv, k) != PICKUP_BOMB) {
                        i32 seekable = 1;
                        if (sv->GetPowerupType() == GRUNT_GHOST) {
                            seekable = 0;
                        }
                        if (EQUIPPED_TOOL_OF_TERNARY_LE(sv, k) == PICKUP_WARPSTONE) {
                            seekable = 0;
                        }
                        if (seekable) {
                            i32 ex = sv->GetScreenTileX();
                            i32 ddx = ex - this->GetScreenTileX();
                            i32 ey = sv->GetScreenTileY() - this->GetScreenTileY();
                            i32 dist = abs(SQR(ddx)) + abs(SQR(ey));
                            if (dist < best && dist <= SQR(this->m_defenderRadius)) {
                                best = dist;
                                bestIdx = i;
                            }
                        }
                    }
                }
                i++;
            } while (i < 0xf);
            if (bestIdx != -1) {
                this->m_arrivalCell.m_x = bestIdx;
                CGameObject* base = g_gameReg->GetTriggerMgr()->UnitAt(0, bestIdx)->m_object;
                if (MoveToTile(
                        base->m_screenX >> TILE_SHIFT_PX,
                        base->m_screenY >> TILE_SHIFT_PX,
                        0,
                        this->m_arrivalFlags,
                        1,
                        0
                    )
                    != 0) {
                    PLAY_VOICE_IN_VIEW(0x366);
                }
            }
            this->m_dwell = 0;
            return 1;
        }
        if (this->m_aiState != AISTATE_SEEK) {
            return 1;
        }
        if (static_cast<u32>(this->m_dwell) <= 0x3e8) {
            return 1;
        }
        CGameObject* base =
            g_gameReg->GetTriggerMgr()->UnitAt(0, this->m_arrivalCell.m_x)->m_object;
        MoveToTile(
            base->m_screenX >> TILE_SHIFT_PX,
            base->m_screenY >> TILE_SHIFT_PX,
            0,
            this->m_arrivalFlags,
            1,
            0
        );
    }
    this->m_dwell = 0;
    return 1;
}
