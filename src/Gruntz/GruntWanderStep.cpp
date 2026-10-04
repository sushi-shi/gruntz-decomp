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
#include <Gruntz/GruntPoweredStateMacros.h>
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
#include <RectMacros.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000ed9f0, 0x900)
i32 CGrunt::StepHitAndRunnerBehavior() {
    m_defenderPx = m_lastTilePx;

    i32 flag = 0;
    FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(g, flag)

    b32 inCombat = m_inCombat;
    if (inCombat != false) {
        b32 attackQueued = m_attackQueued;
        if (attackQueued == false) {
            if (m_attackWindupActive == false) {
                if (m_stamina >= STAMINA_FULL) {
                    if (TryAttackRememberedTarget(1) != NULL) {
                        m_aiState = AISTATE_RETREAT;
                        return 1;
                    }
                    if (flag != 0 && g == NULL) {
                        goto retreat;
                    }
                    if (m_inCombat == false || m_attackQueued != false) {
                        goto retreat;
                    }
                    RESET_GRUNT_COMBAT_STATE(this)
                } else {
                    if (flag != 0) {
                        goto retreat;
                    }
                    if (m_inCombat == false || m_attackQueued != false) {
                        goto retreat;
                    }
                    RESET_GRUNT_COMBAT_STATE(this)
                }
            }
        } else {
            m_attackQueued = false;
        }
    retreat:
        m_aiState = AISTATE_RETREAT;
    }

    switch (m_aiState) {
        case AISTATE_SEEK: {
            Coord c;
            if (g != NULL && m_inCombat == false && m_stamina >= STAMINA_FULL
                && IsGruntAtSavedScreenPos(g)
                && IsWithinReach(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
                COMMIT_HIT_AND_RUN_ATTACK(g);
                return 1;
            }
            if (g != NULL && static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) != 0) {
                    g->GetScreenTile(&c);
                    if (MoveToTile(c.m_x, c.m_y, 0, m_arrivalFlags, 1, 0) != 0) {
                        SET_GRUNT_ARRIVAL_TARGET(g);
                        m_aiState = AISTATE_CHASE;
                        PLAY_VOICE_IF_VISIBLE(0x366);
                    }
                }
                m_dwell = 0;
                return 1;
            }
            break;
        }

        case AISTATE_CHASE: {
            CGrunt* slot = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            CGrunt* active = m_triggerMgr->FindNearestEnemy(this);
            if (active != NULL && active != slot) {
                ResetToSeek();
                return 1;
            }
            if (slot == NULL || slot->IsEntranceCommitted() == false
                || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0) {
                m_aiState = AISTATE_SEEK;
                return 1;
            }
            RepathToward(slot);
            if (m_inCombat != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (IsWithinReach(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                return 1;
            }
            if (!IsGruntAtSavedScreenPos(slot)) {
                return 1;
            }
            COMMIT_HIT_AND_RUN_ATTACK(slot);
            return 1;
        }

        case AISTATE_ATTACK: {
            if (m_inCombat == false) {
                m_aiState = AISTATE_SEEK;
                return 1;
            }
            CGrunt* slot = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            if (slot == NULL || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0
                || slot->IsEntranceCommitted() == false) {
                goto ph1;
            }
            if (m_attackQueued != false) {
                return 1;
            }
            if (m_attackWindupActive != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (IsWithinReach(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                goto ph1;
            }
            if (!IsGruntAtSavedScreenPos(slot)) {
                goto ph1;
            }
            COMMIT_HIT_AND_RUN_ATTACK(slot);
            m_dwell = DWELL_REPATH_MS;
            return 1;
        ph1:
            m_aiState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            return 1;
        }

        case AISTATE_RETREAT: {
            if (m_attackWindupActive != false) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                m_aiState = AISTATE_SEEK;
                return 1;
            }
            if (!CoordsEmpty()) {
                return 1;
            }
            CWwdSpriteObject* base = m_object;
            i32 clip = 1;
            i32 baseTileY = base->m_screenY >> TILE_SHIFT_PX;
            i32 baseTileX = base->m_screenX >> TILE_SHIFT_PX;
            i32 py = GetRandom(3) + baseTileY - 2;
            i32 px = GetRandom(3) + baseTileX - 2;
            if (static_cast<u32>(m_arrivalCell.m_x) < 4
                && static_cast<u32>(m_arrivalCell.m_y) < 0xf) {
                CGrunt* entry =
                    g_gameReg->GetTriggerMgr()->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
                if (entry != NULL) {
                    CRect rc(
                        ScreenTile(entry).m_x - 2,
                        ScreenTile(entry).m_y - 2,
                        ScreenTile(entry).m_x + 3,
                        ScreenTile(entry).m_y + 3
                    );
                    POINT pt;
                    SET_POINT_COMPONENTS(pt, px, py);
                    if (rc.PtInRect(pt)) {
                        clip = 0;
                    }
                }
            }
            if (clip == 0) {
                return 1;
            }
            CMapMgr* grid = g_gameReg->GetTileGrid();
            if (static_cast<u32>(px) >= static_cast<u32>(grid->GetWidth())) {
                return 1;
            }
            if (static_cast<u32>(py) >= static_cast<u32>(grid->GetHeight())) {
                return 1;
            }
            MoveToTile(px, py, 0, m_arrivalFlags, 1, 0);
            return 1;
        }

        default:
            return 1;
    }

    if (m_idleVariantActive == false && m_hasExtent != false
        && static_cast<u32>(m_dwell) > DWELL_STUCK_RESET_MS) {
        if (IsArrivalRerollPending() != 0) {
            CWwdSpriteObject* base = m_object;
            SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(base, lx, ax, ly, ay)
            if (lx < g_gameReg->GetTileGrid()->GetWidth()
                && ly < g_gameReg->GetTileGrid()->GetHeight()) {
                MoveToTile(static_cast<i32>(lx), static_cast<i32>(ly), 0, m_arrivalFlags, 1, 0);
            }
            if (!CoordsEmpty()) {
                ax = Max(ax, ay);
                if (CoordCount() > ax) {
                    SetEntrancePos(1, 1);
                    m_dwell = 0;
                    return 1;
                }
            }
        } else {
            ResetArrivalReroll();
        }
        m_dwell = 0;
    }
    return 1;
}
