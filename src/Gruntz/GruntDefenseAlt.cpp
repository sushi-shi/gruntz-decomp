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

RVA(0x000f1da0, 0x620)
i32 CGrunt::StepObjectGuardBehavior() {
    m_arrivalFlags |= 0x40000;
    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    i32 inRange = 0;
    if (occ != NULL && GRUNT_AT_SAVED_SCREEN_POS(occ)
        && RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
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
            RESET_GRUNT_POWERED_STATE(this)
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
            RESET_GRUNT_POWERED_STATE(this)
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
                if (m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(o)
                    && RectContains(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                    COMMIT_GRUNT_NEIGHBOR(o);
                    return 1;
                }
            }
            if (m_poweredUp != false) {
                goto tail;
            }
            {
                Coord entrance = EntrancePx();
                Coord tile = LastTilePx();
                if (tile.m_x != entrance.m_x || tile.m_y != entrance.m_y) {
                    goto tail;
                }
            }
            {
                i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
                i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
                i32 gx = m_defenderPx.m_x >> TILE_SHIFT_PX;
                i32 gy = m_defenderPx.m_y >> TILE_SHIFT_PX;
                if (tx < gx && ty < gy) {
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
                if (tx < gx && ty > gy) {
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
                if (tx > gx && ty < gy) {
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
                if (tx > gx && ty > gy) {
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
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
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
            if (RectContains(o->m_object->m_screenX, o->m_object->m_screenY) == 0) {
                goto tail;
            }
            if (o->GRUNT_SCREEN_X_NOT_AT_SAVED_POS(m_object, o)) {
                goto tail;
            }
            if (o->GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(m_object, o)) {
                goto tail;
            }
            COMMIT_GRUNT_NEIGHBOR(o);
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_ATTACK:
            m_defenderState = AISTATE_SEEK;
            return 1;

        case AISTATE_RETURN: {
            StepArrivalDrop(
                m_defenderPx.m_x - 0x20,
                m_defenderPx.m_y - 0x20,
                0,
                m_arrivalFlags,
                1,
                0
            );
            if (m_object->m_screenX == m_defenderPx.m_x - 0x20
                && m_object->m_screenY == m_defenderPx.m_y - 0x20) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            CGrunt* o = m_triggerMgr->FindNearestEnemy(this);
            if (o == NULL) {
                goto tail;
            }
            if (m_poweredUp == false && m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(o)
                && RectContains(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                COMMIT_GRUNT_NEIGHBOR(o);
                m_defenderState = AISTATE_ATTACK;
            }
            if (GruntInRadius(o->m_playerIndex, o->m_unitIndex) == 0) {
                goto tail;
            }
            m_arrivalCell.m_x = o->m_playerIndex;
            m_arrivalCell.m_y = o->m_unitIndex;
            m_defenderState = AISTATE_CHASE;
            {
                CWwdSpriteObject* h = m_object;
                i32 x = h->m_screenX;
                i32 y = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (CGameLevel::PointInRect(rect, x, y)) {
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
