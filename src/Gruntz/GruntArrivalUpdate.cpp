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
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
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
// Residue is the second copy of the powered-up reset tail: retail emits the
// stamina>=FULL arm (0xf0238, members RE-read because FindGridNeighbor sits in
// front of it) and the stamina<FULL arm (0xf0288, the cached ecx/eax) as two
// full `m_entranceActive/m_combatActive/m_neighborValid/m_poweredUp = 0` blocks
// with their own epilogues; cl proves the second arm's two guards redundant
// against the enclosing `poweredUp != 0` gate, drops them and cross-jumps what
// is left onto the first block. Local outer guards + MEMBER inner re-tests
// keep the second arm's guards (local-guard-member-retest-blocks-fold).
RVA(0x000f0130, 0x7c0)
i32 CGrunt::UpdateArrival() {
    char* name = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
    // POLARITY: retail's `sete cl` (0xf0184) leaves the body on the strcmp!=0 path -
    // the arrival AI is SKIPPED for act "I". We had `!=`, which ran it only for "I".
    // Same spelling as CBattlezMapConfig::ChooseIdleBehavior's four name gates.
    bool eqI = (strcmp(name, "I") == 0);
    if (eqI) {
        return 1;
    }
    this->m_defenderPx = this->m_lastTilePx;
    FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget, x)

    i32 poweredUp = this->m_poweredUp;
    if (poweredUp != 0) {
        // The gate pair is m_neighborValid (+0x21c) then m_combatActive (+0x218) -
        // retail tests 0x21c and clears 0x21c (0xf01f8 / 0xf02c8). The two names were
        // swapped here; the rest of the tree agrees with Grunt.h (store_offsets shows
        // no other 0x218/0x21c divergence).
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
                this->m_entranceActive = 0;
                this->m_combatActive = 0;
                this->m_neighborValid = 0;
                this->m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
                return 1;
            }
            if (atTarget) {
                return 1;
            }
            if (this->m_poweredUp == 0) {
                return 1;
            }
            if (this->m_neighborValid != 0) {
                return 1;
            }
            this->m_entranceActive = 0;
            this->m_combatActive = 0;
            this->m_neighborValid = 0;
            this->m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        this->m_neighborValid = 0;
        return 1;
    }

    switch (this->m_defenderState) {
        case AISTATE_SEEK:
            if (g != NULL) {
                if (this->m_stamina >= STAMINA_FULL) {
                    i32 x = g->m_object->m_screenX;
                    if (GRUNT_X_AT_SAVED_POS(x, g) && g->GRUNT_SCREEN_Y_AT_SAVED_POS(m_object, g)
                        && RectContains(x, g->m_object->m_screenY) != 0) {
                        COMMIT_GRUNT_NEIGHBOR(g);
                        break;
                    }
                }
                if (g != NULL && static_cast<u32>(this->m_dwell) > 1000) {
                    if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) != 0) {
                        Coord c[2];
                        g->GetScreenPos(c);
                        if (TileSwitch(
                                c[0].m_x >> TILE_SHIFT_PX,
                                c[0].m_y >> TILE_SHIFT_PX,
                                0,
                                this->m_arrivalFlags,
                                0,
                                0x20
                            )
                            != 0) {
                            SET_GRUNT_ARRIVAL_TARGET(g);
                            this->m_defenderState = AISTATE_CHASE;
                            CGruntzMgr* reg = g_gameReg;
                            i32 r = CGameLevel::PointInBounds(
                                &reg->m_world->m_level->m_mainPlane->m_viewRect,
                                this->m_object->m_screenX,
                                this->m_object->m_screenY
                            );
                            if (r != 0) {
                                reg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    this->m_dwell = 0;
                    break;
                }
            }
            if (this->m_resetApplied == 0 && this->m_hasExtent != 0
                && static_cast<u32>(this->m_dwell) > 3000) {
                if (IsArrivalRerollPending() != 0) {
                    CGameObject* base = this->m_object;
                    SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(base, lo, ax, lo2, ay)
                    if (lo < g_gameReg->m_tileGrid->m_width
                        && lo2 < g_gameReg->m_tileGrid->m_height) {
                        TileSwitch(
                            static_cast<i32>(lo),
                            static_cast<i32>(lo2),
                            0,
                            this->m_arrivalFlags,
                            1,
                            0
                        );
                    }
                    if (this->CoordCount() != 0) {
                        if (ax <= ay) {
                            ax = ay;
                        }
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
        case AISTATE_CHASE: {
            CGrunt* slot =
                m_tileMgr->m_units
                    [this->m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + this->m_arrivalCell.m_y];
            CGrunt* found = m_tileMgr->FindNearestEnemy(this);
            if (found == NULL || found == slot) {
                if (slot == NULL || slot->m_entranceCommitted == 0
                    || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0) {
                    this->m_defenderState = AISTATE_SEEK;
                } else {
                    StepArrivalDrop(
                        slot->m_lastTilePx.m_x,
                        slot->m_lastTilePx.m_y,
                        0,
                        this->m_arrivalFlags,
                        0,
                        0x20
                    );
                    if (this->m_poweredUp == 0 && this->m_stamina >= STAMINA_FULL
                        && RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) != 0
                        && GRUNT_AT_SAVED_SCREEN_POS(slot)) {
                        COMMIT_GRUNT_NEIGHBOR(slot);
                        this->m_defenderState = AISTATE_ATTACK;
                    }
                }
            } else {
                Coord none;
                this->m_arrivalCell = *none.Set(-1, -1);
                this->m_defenderState = AISTATE_SEEK;
            }
            break;
        }
        case AISTATE_ATTACK: {
            if (m_poweredUp != 0) {
                CGrunt* slot =
                    m_tileMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
                // The null test is the FIRST term of the &&-chain and is repeated after
                // the block: retail jump-threads the leading `slot == NULL` straight to
                // the SEEK store (0xf056c) and threads the in-block failures PAST the
                // repeat (0xf03ce vs 0xf03d6). Hoisting it to its own early `break`
                // loses the second copy.
                if (slot != NULL && GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) != 0
                    && slot->m_entranceCommitted != 0) {
                    // 0x21c (m_neighborValid) is tested first here too - see above.
                    if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < STAMINA_FULL) {
                        break;
                    }
                    if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) != 0
                        && GRUNT_AT_SAVED_SCREEN_POS(slot)) {
                        COMMIT_GRUNT_NEIGHBOR(slot);
                        break;
                    }
                }
                if (slot == NULL) {
                    m_defenderState = AISTATE_SEEK;
                    break;
                }
                m_defenderState = AISTATE_CHASE;
                {
                    // Retail expands the bounds test here (0xf03f8) and CALLS it
                    // from the AISTATE_SEEK arm above (0xf06a0), so this arm takes
                    // the inline PointInRect sibling.
                    CGruntzMgr* reg = g_gameReg;
                    const RECT& view = reg->m_world->m_level->m_mainPlane->m_viewRect;
                    i32 px = m_object->m_screenX;
                    i32 py = m_object->m_screenY;
                    if (CGameLevel::PointInRect(&view, px, py)) {
                        reg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                    }
                }
                break;
            }
            m_defenderState = AISTATE_CHASE;
            break;
        }
    }

    if (this->CoordCount() != 0) {

        Coord* cell = this->CoordHead()->m_coord;

        BrickzCell& gc = g_gameReg->m_tileGrid->m_rows[cell->m_y][cell->m_x];
        if ((gc.m_flagBytes[0] & 0x20) != 0) {
            SetEntrancePos(1, 1);
            if (this->CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS_EXPANDED(this)
            }
            g_gameReg->m_cmdGrid->ApplyTriggerA(
                m_playerIndex,
                m_unitIndex,
                cell->m_x * 0x20 + 0x10,
                cell->m_y * 0x20 + 0x10
            );
        }
    }
    return 1;
}
