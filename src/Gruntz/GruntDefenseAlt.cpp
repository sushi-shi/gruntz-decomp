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

RVA(0x000f1c70, 0x620)
i32 CGrunt::StepArrivalDefenseAlt() {
    m_arrivalFlags |= 0x40000;
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    i32 inRange = 0;
    if (occ != NULL && GRUNT_AT_SAVED_SCREEN_POS(occ)
        && RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
        inRange = 1;
    }

    i32 powered = m_poweredUp;
    if (powered != 0) {
        i32 neighborValid = m_neighborValid;
        if (neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            goto tail;
        }
        // Retail spells the stand-down twice, once per stamina arm (two
        // ResetEntranceAnimation calls and two `mov eax,1` epilogues at
        // 0xf1d36 and 0xf1d7e).
        if (m_stamina >= STAMINA_FULL) {
            if (FindGridNeighbor(1) != NULL) {
                goto tail;
            }
            if (inRange != 0 && occ == NULL) {
                goto tail;
            }
            if (m_poweredUp == 0) {
                goto tail;
            }
            if (m_neighborValid != 0) {
                goto tail;
            }
            RESET_GRUNT_POWERED_STATE
            return 1;
        } else {
            if (inRange != 0) {
                goto tail;
            }
            if (m_poweredUp == 0) {
                goto tail;
            }
            if (m_neighborValid != 0) {
                goto tail;
            }
            RESET_GRUNT_POWERED_STATE
            return 1;
        }
    }

    switch (m_defenderState) {
        case AISTATE_SEEK: {
            CGrunt* o = m_tileMgr->FindNearestEnemy(this);
            if (o != NULL) {
                if (m_poweredUp != 0) {
                    goto tail;
                }
                if (m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(o)
                    && RectContains(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                    Coord tile = o->m_lastTilePx;
                    CommitNeighbor(o->m_tileOwnerHi, o->m_tileOwnerLo, tile.m_x, tile.m_y);
                    return 1;
                }
            }
            if (m_poweredUp != 0) {
                goto tail;
            }
            if (m_lastTilePx.m_x != m_entrancePx.m_x || m_lastTilePx.m_y != m_entrancePx.m_y) {
                goto tail;
            }
            {
                i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
                i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
                i32 gx = m_defenderPx.m_x >> TILE_SHIFT_PX;
                i32 gy = m_defenderPx.m_y >> TILE_SHIFT_PX;
                // Four FLAT `if (a && b)` statements, not two nested tests: retail
                // re-compares tx against gx before each pair (0xf1ebe / 0xf1ef6 /
                // 0xf1f2e / 0xf1f6a) and chains the redundant compares itself.
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
            CGrunt* o = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != NULL && g != o) {
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (o == NULL) {
                goto resetState;
            }
            if (o->m_entranceCommitted == 0) {
                goto resetState;
            }
            if (GruntInRadius(o->m_tileOwnerHi, o->m_tileOwnerLo) == 0) {
                goto resetState;
            }
            if (GruntInRadius(m_arrivalCell.m_x, m_arrivalCell.m_y) == 0) {
                goto resetState;
            }
            StepArrivalDrop(o->m_lastTilePx.m_x, o->m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
            if (m_poweredUp != 0) {
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
            Coord tile = o->m_lastTilePx;
            CommitNeighbor(o->m_tileOwnerHi, o->m_tileOwnerLo, tile.m_x, tile.m_y);
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
            CGrunt* o = m_tileMgr->FindNearestEnemy(this);
            if (o == NULL) {
                goto tail;
            }
            if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(o)
                && RectContains(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                Coord tile = o->m_lastTilePx;
                CommitNeighbor(o->m_tileOwnerHi, o->m_tileOwnerLo, tile.m_x, tile.m_y);
                m_defenderState = AISTATE_ATTACK;
            }
            if (GruntInRadius(o->m_tileOwnerHi, o->m_tileOwnerLo) == 0) {
                goto tail;
            }
            m_arrivalCell.m_x = o->m_tileOwnerHi;
            m_arrivalCell.m_y = o->m_tileOwnerLo;
            m_defenderState = AISTATE_CHASE;
            {
                CWwdGameObjectA* h = m_object;
                i32 x = h->m_screenX;
                i32 y = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInRect(rect, x, y)) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
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
