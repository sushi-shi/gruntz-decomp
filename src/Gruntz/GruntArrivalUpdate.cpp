#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntMovementInline.h>
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
RVA(0x000f0130, 0x7c0)
i32 CGrunt::StepGauntletGruntBehavior() {
    char* name = *g_typeColl.GetNameRecord(m_logicRecord->m_eventCode);
    bool eqI = (strcmp(name, "I") == 0);
    if (eqI) {
        return 1;
    }
    this->m_defenderPx = this->m_lastTilePx;
    i32 atTarget;
    CGrunt* g = FindNearestEnemyAtTarget(this, &atTarget);

    b32 poweredUp = this->m_poweredUp;
    if (poweredUp != false) {
        b32 neighborValid = this->m_neighborValid;
        if (neighborValid == false) {
            if (this->m_combatActive != false) {
                return 1;
            }
            if (this->m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    return 1;
                }
                if (atTarget && g == NULL) {
                    return 1;
                }
                if (this->m_poweredUp == false) {
                    return 1;
                }
                if (this->m_neighborValid != false) {
                    return 1;
                }
                this->m_entranceActive = false;
                this->m_combatActive = false;
                this->m_neighborValid = false;
                this->m_poweredUp = false;
                ResetEntranceAnimation(1, 0, 0);
                return 1;
            }
            if (atTarget) {
                return 1;
            }
            if (this->m_poweredUp == false) {
                return 1;
            }
            if (this->m_neighborValid != false) {
                return 1;
            }
            this->m_entranceActive = false;
            this->m_combatActive = false;
            this->m_neighborValid = false;
            this->m_poweredUp = false;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        this->m_neighborValid = false;
        return 1;
    }

    switch (this->m_defenderState) {
        case AISTATE_SEEK:
            if (g != NULL) {
                if (this->m_stamina >= STAMINA_FULL) {
                    Coord position = g->m_object->ScreenPos();
                    if (position == g->m_lastTilePx
                        && RectContains(position.m_x, position.m_y) != 0) {
                        CommitGruntNeighbor(this, g);
                        break;
                    }
                }
                if (g != NULL && static_cast<u32>(this->m_dwell) > 1000) {
                    if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) != 0) {
                        Coord targetTile;
                        g->GetScreenTile(&targetTile);
                        if (TileSwitch(
                                targetTile.m_x,
                                targetTile.m_y,
                                0,
                                this->m_arrivalFlags,
                                0,
                                BATTLEZ_ROUTE_OTHER_TOOLS
                            )
                            != 0) {
                            SetGruntArrivalTarget(this, g);
                            this->m_defenderState = AISTATE_CHASE;
                            CGruntzMgr* reg = g_gameReg;
                            i32 r = CGameLevel::PointInBounds(
                                &reg->m_world->m_level->m_mainPlane->m_planeViewRect,
                                this->m_object->m_screenPosition.m_x,
                                this->m_object->m_screenPosition.m_y
                            );
                            if (r != 0) {
                                reg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    this->m_dwell = 0;
                    break;
                }
            }
            if (this->m_resetApplied == false && this->m_hasExtent != false
                && static_cast<u32>(this->m_dwell) > 3000) {
                if (IsArrivalRerollPending() != 0) {
                    CGameObject* base = this->m_object;
                    Coord point;
                    Coord span;
                    SelectRandomExtentPoint(base, &point, &span);
                    if (static_cast<u32>(point.m_x) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(point.m_y) < g_gameReg->m_tileGrid->m_height) {
                        TileSwitch(point.m_x, point.m_y, 0, this->m_arrivalFlags, 1, 0);
                    }
                    if (this->CoordCount() != 0) {
                        if (this->CoordCount() > Max(span.m_x, span.m_y)) {
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
                m_triggerMgr->m_units
                    [this->m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + this->m_arrivalCell.m_y];
            CGrunt* found = m_triggerMgr->FindNearestEnemy(this);
            if (found == NULL || found == slot) {
                if (slot == NULL || slot->m_entranceCommitted == false
                    || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0) {
                    this->m_defenderState = AISTATE_SEEK;
                } else {
                    StepArrivalDrop(
                        slot->m_lastTilePx.m_x,
                        slot->m_lastTilePx.m_y,
                        0,
                        this->m_arrivalFlags,
                        0,
                        BATTLEZ_ROUTE_OTHER_TOOLS
                    );
                    if (this->m_poweredUp == false && this->m_stamina >= STAMINA_FULL
                        && RectContains(
                               slot->m_object->m_screenPosition.m_x,
                               slot->m_object->m_screenPosition.m_y
                           ) != 0
                        && IsGruntAtSavedScreenPos(slot)) {
                        CommitGruntNeighbor(this, slot);
                        this->m_defenderState = AISTATE_ATTACK;
                    }
                }
            } else {
                this->m_arrivalCell.Set(-1, -1);
                this->m_defenderState = AISTATE_SEEK;
            }
            break;
        }
        case AISTATE_ATTACK: {
            if (m_poweredUp != false) {
                CGrunt* slot =
                    m_triggerMgr
                        ->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
                if (slot != NULL && GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) != 0
                    && slot->m_entranceCommitted != false) {
                    if (m_neighborValid != false || m_combatActive != false
                        || m_stamina < STAMINA_FULL) {
                        break;
                    }
                    if (RectContains(
                            slot->m_object->m_screenPosition.m_x,
                            slot->m_object->m_screenPosition.m_y
                        ) != 0
                        && IsGruntAtSavedScreenPos(slot)) {
                        CommitGruntNeighbor(this, slot);
                        break;
                    }
                }
                if (slot == NULL) {
                    m_defenderState = AISTATE_SEEK;
                    break;
                }
                m_defenderState = AISTATE_CHASE;
                {
                    CGruntzMgr* reg = g_gameReg;
                    const RECT& view = reg->m_world->m_level->m_mainPlane->m_planeViewRect;
                    Coord voicePosition = m_object->ScreenPos();
                    if (::PtInRect(&view, voicePosition.m_x, voicePosition.m_y)) {
                        reg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
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
        if ((gc.m_flags & IDX(CELL_FLAG_DESTRUCTIBLE_ROCK)) != 0) {
            SetEntrancePos(1, 1);
            if (this->CoordCount() != 0) {
                RecycleGruntCoords(this);
            }
            Coord position = *cell;
            TileCenter(&position);
            g_gameReg->m_triggerMgr
                ->UseEquippedToolAt(m_playerIndex, m_unitIndex, position.m_x, position.m_y);
        }
    }
    return 1;
}
