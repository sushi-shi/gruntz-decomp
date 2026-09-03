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
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
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

RVA(0x000f1c70, 0x620)
i32 CGrunt::StepObjectGuardBehavior() {
    m_arrivalFlags |= IDX(CELL_FLAG_IN_GAME_ICON);
    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    i32 inRange = 0;
    if (occ != NULL && IsGruntAtSavedScreenPos(occ)
        && RectContains(occ->m_object->m_screenPosition.m_x, occ->m_object->m_screenPosition.m_y)
               != 0) {
        inRange = 1;
    }

    b32 powered = m_poweredUp;
    if (powered != false) {
        b32 neighborValid = m_neighborValid;
        if (neighborValid != false) {
            m_neighborValid = false;
            return 1;
        }
        if (m_combatActive != false) {
            goto tail;
        }
        if (m_stamina >= STAMINA_FULL) {
            if (FindGridNeighbor(1) != NULL) {
                goto tail;
            }
            if (inRange != 0 && occ == NULL) {
                goto tail;
            }
            if (m_poweredUp == false) {
                goto tail;
            }
            if (m_neighborValid != false) {
                goto tail;
            }
            ResetGruntPoweredState(this);
            return 1;
        } else {
            if (inRange != 0) {
                goto tail;
            }
            if (m_poweredUp == false) {
                goto tail;
            }
            if (m_neighborValid != false) {
                goto tail;
            }
            ResetGruntPoweredState(this);
            return 1;
        }
    }

    switch (m_defenderState) {
        case AISTATE_SEEK: {
            CGrunt* o = m_triggerMgr->FindNearestEnemy(this);
            if (o != NULL) {
                if (m_poweredUp != false) {
                    goto tail;
                }
                if (m_stamina >= STAMINA_FULL && IsGruntAtSavedScreenPos(o)
                    && RectContains(
                           o->m_object->m_screenPosition.m_x,
                           o->m_object->m_screenPosition.m_y
                       ) != 0) {
                    CommitGruntNeighbor(this, o);
                    return 1;
                }
            }
            if (m_poweredUp != false) {
                goto tail;
            }
            {
                Coord entrance = EntrancePx();
                Coord tile = LastTilePx();
                if (tile != entrance) {
                    goto tail;
                }
            }
            {
                Coord tile = m_lastTilePx;
                Coord guardTile = m_defenderPx;
                ScreenTile(&tile);
                ScreenTile(&guardTile);
                if (tile.m_x < guardTile.m_x && tile.m_y < guardTile.m_y) {
                    StepArrivalDrop(
                        m_lastTilePx.m_x + 0x40,
                        m_lastTilePx.m_y,
                        0,
                        m_arrivalFlags,
                        1,
                        0
                    );
                    return 1;
                }
                if (tile.m_x < guardTile.m_x && tile.m_y > guardTile.m_y) {
                    StepArrivalDrop(
                        m_lastTilePx.m_x,
                        m_lastTilePx.m_y - 0x40,
                        0,
                        m_arrivalFlags,
                        1,
                        0
                    );
                    return 1;
                }
                if (tile.m_x > guardTile.m_x && tile.m_y < guardTile.m_y) {
                    StepArrivalDrop(
                        m_lastTilePx.m_x,
                        m_lastTilePx.m_y + 0x40,
                        0,
                        m_arrivalFlags,
                        1,
                        0
                    );
                    return 1;
                }
                if (tile.m_x > guardTile.m_x && tile.m_y > guardTile.m_y) {
                    StepArrivalDrop(
                        m_lastTilePx.m_x - 0x40,
                        m_lastTilePx.m_y,
                        0,
                        m_arrivalFlags,
                        1,
                        0
                    );
                    return 1;
                }
                goto resetState;
            }
        }

        case AISTATE_CHASE: {
            CGrunt* o =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            CGrunt* g = m_triggerMgr->FindNearestEnemy(this);
            if (g != NULL && g != o) {
                m_arrivalCell.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (o == NULL) {
                goto resetState;
            }
            if (o->m_entranceCommitted == false) {
                goto resetState;
            }
            if (GruntInRadius(o->m_playerIndex, o->m_unitIndex) == 0) {
                goto resetState;
            }
            if (GruntInRadius(m_arrivalCell.m_x, m_arrivalCell.m_y) == 0) {
                goto resetState;
            }
            StepArrivalDrop(o->m_lastTilePx.m_x, o->m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
            if (m_poweredUp != false) {
                goto tail;
            }
            if (m_stamina < STAMINA_FULL) {
                goto tail;
            }
            if (RectContains(o->m_object->m_screenPosition.m_x, o->m_object->m_screenPosition.m_y)
                == 0) {
                goto tail;
            }
            if (o->m_object->ScreenPos() != o->m_lastTilePx) {
                goto tail;
            }
            CommitGruntNeighbor(this, o);
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_ATTACK:
            m_defenderState = AISTATE_SEEK;
            return 1;

        case AISTATE_RETURN: {
            Coord returnPosition = m_defenderPx - Coord(TILE_SIZE_PX, TILE_SIZE_PX);
            StepArrivalDrop(returnPosition.m_x, returnPosition.m_y, 0, m_arrivalFlags, 1, 0);
            if (m_object->ScreenPos() == returnPosition) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            CGrunt* o = m_triggerMgr->FindNearestEnemy(this);
            if (o == NULL) {
                goto tail;
            }
            if (m_poweredUp == false && m_stamina >= STAMINA_FULL && IsGruntAtSavedScreenPos(o)
                && RectContains(
                       o->m_object->m_screenPosition.m_x,
                       o->m_object->m_screenPosition.m_y
                   ) != 0) {
                CommitGruntNeighbor(this, o);
                m_defenderState = AISTATE_ATTACK;
            }
            if (GruntInRadius(o->m_playerIndex, o->m_unitIndex) == 0) {
                goto tail;
            }
            m_arrivalCell.Set(o->m_playerIndex, o->m_unitIndex);
            m_defenderState = AISTATE_CHASE;
            {
                Coord voicePosition = m_object->ScreenPos();
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (::PtInRect(rect, voicePosition.m_x, voicePosition.m_y)) {
                    g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
            }
            goto tail;
        }

        default:
            goto tail;
    }

resetState:
    m_defenderState = AISTATE_RETURN;
    return 1;

tail:
    return 1;
}
