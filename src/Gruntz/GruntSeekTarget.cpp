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
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
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

// @early-stop
// One dead spill of the candidate's tile x at 0xf778f remains unreached.
RVA(0x000f71c0, 0x721)
i32 CGrunt::SeekTarget() {
    COPY_CURRENT_GRUNT_LAST_TILE_TO_DEFENDER
    if (this->CoordCount() != 0
        && g_gameReg->m_cmdGrid->m_units[0 * TM_UNITS_PER_PLAYER + this->m_arrivalCell.m_x]
               == NULL) {
        RECYCLE_GRUNT_COORDS(this)
        this->m_arrivalCell.m_x = 0;
    }

    // NOT a PickupType local: it is seeded from m_entranceReason/m_toolId but
    // then REUSED to hold a cell x-coordinate below, so it carries two
    // domains. Left as i32 until the reuse is untangled.
    i32 reason = IDX(this->m_entranceReason);
    if (reason > 0x16) {
        reason = IDX(this->m_toolId);
    }
    if (reason == 0 && (reason = this->m_arrivalCell.m_x, reason >= 0) && reason < 0xf) {
        CGrunt* slot = g_gameReg->m_cmdGrid->m_units[0 * TM_UNITS_PER_PLAYER + reason];
        if (slot == NULL || slot->m_entranceCommitted == 0) {
            if (this->CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS(this)
            }
            this->m_arrivalCell.m_x = -1;
            return 1;
        }

        Coord selfTile;
        GetScreenTile(&selfTile);
        Coord slotTile;
        slot->GetScreenTile(&slotTile);
        Coord selfTileB;
        GetScreenTile(&selfTileB);
        Coord slotTileB;
        slot->GetScreenTile(&slotTileB);
        i32 dx = selfTile.m_x - slotTile.m_x;
        i32 dy = selfTileB.m_y - slotTileB.m_y;
        if (abs(dx) <= 1 && abs(dy) <= 1) {
            PickupType r2 = slot->m_entranceReason;
            if (r2 > PICKUP_EQUIPPABLE_LAST) {
                r2 = slot->m_toolId;
            }
            if (r2 != PICKUP_WARPSTONE && r2 != PICKUP_BOMB) {
                slot->LoadGruntTypeTable(r2, 1, 0, 0);
                slot->LoadGruntTypeTable(PICKUP_NONE, 1, 0, 0);
                this->m_defenderState = AISTATE_COOLDOWN;
                if (this->CoordCount() == 0) {
                    return 1;
                }
                RECYCLE_GRUNT_COORDS(this)
                return 1;
            }
        }
    }

    reason = IDX(this->m_entranceReason);
    if (reason > 0x16) {
        reason = IDX(this->m_toolId);
    }
    if (reason != 0) {
        FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget, x)
        i32 powered = this->m_poweredUp;
        if (powered != 0) {
            i32 neighborValid = this->m_neighborValid;
            if (neighborValid == 0) {
                if (this->m_combatActive != 0) {
                    return 1;
                }
                if (this->m_stamina >= STAMINA_FULL) {
                    if (FindGridNeighbor(1) != NULL) {
                        return 1;
                    }
                    if (atTarget && g == NULL) {
                        return 1;
                    }
                    if (this->m_poweredUp == 0) {
                        return 1;
                    }
                    if (this->m_neighborValid != 0) {
                        return 1;
                    }
                    RESET_CURRENT_GRUNT_POWERED_STATE
                    return 1;
                } else {
                    if (atTarget) {
                        return 1;
                    }
                    if (this->m_poweredUp == 0) {
                        return 1;
                    }
                    if (this->m_neighborValid != 0) {
                        return 1;
                    }
                    RESET_CURRENT_GRUNT_POWERED_STATE
                    return 1;
                }
            } else {
                this->m_neighborValid = 0;
            }
            return 1;
        }
        COPY_CURRENT_GRUNT_LAST_TILE_TO_DEFENDER
        if (g == NULL || GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
            this->m_blockedVoicePending = 0;
            return 1;
        }
        if (this->m_poweredUp == 0 && this->m_stamina >= STAMINA_FULL) {
            i32 x = g->m_object->m_screenX;
            if (GRUNT_X_AT_SAVED_POS(x, g) && g->GRUNT_SCREEN_Y_AT_SAVED_POS(m_object, g)

                && RectContains(x, g->m_object->m_screenY) != 0) {
                COMMIT_GRUNT_NEIGHBOR(g);
            }
        }
        if (static_cast<u32>(this->m_dwell) <= DWELL_REPATH_MS) {
            return 1;
        }
        if (TileSwitch(
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
        if (this->m_blockedVoicePending != 0) {
            i32 r = CGameLevel::PointInBounds(
                &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
                this->m_object->m_screenX,
                this->m_object->m_screenY
            );
            if (r != 0) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
            }
            this->m_blockedVoicePending = 0;
            this->m_dwell = 0;
            return 1;
        }
    } else {
        if (this->CoordCount() == 0) {
            if (this->m_defenderState != AISTATE_SEEK) {
                return 1;
            }
            i32 best = INT_MAX;
            i32 bestIdx = -1;
            CGrunt** slots = g_gameReg->m_cmdGrid->m_units;
            i32 i = 0;
            do {
                CGrunt* sv = slots[i];
                if (sv != NULL && sv->m_entranceCommitted != 0) {
                    PickupType k = sv->m_entranceReason;
                    if (ARRIVAL_PICKUP_OF_TERNARY_LE(sv, k) != PICKUP_NONE
                        && ARRIVAL_PICKUP_OF_TERNARY_LE(sv, k) != PICKUP_WARPSTONE
                        && ARRIVAL_PICKUP_OF_TERNARY_LE(sv, k) != PICKUP_BOMB) {
                        i32 seekable = 1;
                        if (sv->m_gruntKind == GRUNT_GHOST) {
                            seekable = 0;
                        }
                        if (ARRIVAL_PICKUP_OF_TERNARY_LE(sv, k) == PICKUP_WARPSTONE) {
                            seekable = 0;
                        }
                        if (seekable) {
                            i32 ex = sv->m_object->m_screenX >> TILE_SHIFT_PX;
                            i32 ddx = ex - (this->m_object->m_screenX >> TILE_SHIFT_PX);
                            i32 ey = (sv->m_object->m_screenY >> TILE_SHIFT_PX)
                                     - (this->m_object->m_screenY >> TILE_SHIFT_PX);
                            i32 dist = abs(ddx * ddx) + abs(ey * ey);
                            if (dist < best
                                && dist <= this->m_defenderRadius * this->m_defenderRadius) {
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
                CGameObject* base = slots[bestIdx]->m_object;
                if (TileSwitch(
                        base->m_screenX >> TILE_SHIFT_PX,
                        base->m_screenY >> TILE_SHIFT_PX,
                        0,
                        this->m_arrivalFlags,
                        1,
                        0
                    )
                    != 0) {
                    i32 by = this->m_object->m_screenY;
                    i32 bx = this->m_object->m_screenX;
                    CCueRect* board = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                    if (CGameLevel::PointInRect(board, bx, by)) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                    }
                }
            }
            this->m_dwell = 0;
            return 1;
        }
        if (this->m_defenderState != AISTATE_SEEK) {
            return 1;
        }
        if (static_cast<u32>(this->m_dwell) <= 0x3e8) {
            return 1;
        }
        CGameObject* base =
            g_gameReg->m_cmdGrid->m_units[0 * TM_UNITS_PER_PLAYER + this->m_arrivalCell.m_x]
                ->m_object;
        TileSwitch(
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
