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
RVA(0x000f0130, 0x7c0)
i32 CGrunt::UpdateArrival() {
    char* name = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
    bool neI = (strcmp(name, "I") != 0);
    if (neI) {
        return 1;
    }
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != NULL) {
        i32 x = g->m_object->m_screenX;
        if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(x, g->m_object->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    if (this->m_poweredUp != 0) {
        if (this->m_combatActive != 0) {
            this->m_combatActive = 0;
            return 1;
        }
        if (this->m_neighborValid != 0) {
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
            if (this->m_combatActive != 0) {
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
        if (this->m_combatActive != 0) {
            return 1;
        }
        this->m_entranceActive = 0;
        this->m_combatActive = 0;
        this->m_neighborValid = 0;
        this->m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    switch (this->m_defenderState) {
        case AISTATE_SEEK:
            if (g != NULL) {
                if (this->m_stamina > 99) {
                    i32 x = g->m_object->m_screenX;
                    if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
                        && RectContains(x, g->m_object->m_screenY) != 0) {
                        Coord cp = g->m_lastTilePx;
                        CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, cp.m_x, cp.m_y);
                        break;
                    }
                }
                if (g != NULL && static_cast<u32>(this->m_dwell) > 1000) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        Coord c[2];
                        g->GetScreenPos(c);
                        if (TileSwitch(
                                c[0].m_y >> TILE_SHIFT_PX,
                                c[0].m_x >> TILE_SHIFT_PX,
                                0,
                                this->m_arrivalFlags,
                                0,
                                0x20
                            )
                            != 0) {
                            SetEntrancePos(1, 1);
                            this->m_arrivalCell.m_x = g->m_tileOwnerHi;
                            this->m_arrivalCell.m_y = g->m_tileOwnerLo;
                            this->m_defenderState = AISTATE_CHASE;
                            i32 r = CGameLevel::PointInBounds(
                                &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
                                this->m_object->m_screenX,
                                this->m_object->m_screenY
                            );
                            if (r != 0) {
                                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    this->m_dwell = 0;
                    break;
                }
            }
            if (this->m_resetApplied == 0 && this->m_hasExtent != 0
                && static_cast<u32>(this->m_dwell) > 3000) {
                if (static_cast<i64>(g_frameTime) - this->m_arrivalReroll64
                    < this->m_arrivalRerollWindow64) {
                    CGameObject* base = this->m_object;
                    u32 lo = base->m_extent.left;
                    i32 ax = abs(base->m_extent.right - static_cast<i32>(lo));
                    u32 lo2 = base->m_extent.top;
                    i32 ay = abs(base->m_extent.bottom - static_cast<i32>(lo2));
                    if (ax != 0) {
                        lo = lo + rand() % ax;
                    }
                    if (ay != 0) {
                        lo2 = lo2 + rand() % ay;
                    }
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
                    ResetEntranceAnimation(1, 1, 0);
                    this->m_arrivalRerollLo = 0;
                    this->m_arrivalRerollWindowLo = 0;
                    this->m_arrivalRerollHi = 0;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollWindowLo = rand() % 30000 + 30000;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                    this->m_arrivalRerollHi = 0;
                }
                this->m_dwell = 0;
            }
            break;
        case AISTATE_CHASE: {
            CGrunt* slot =
                m_tileMgr->m_grid[this->m_arrivalCell.m_x * TM_GRID_COLS + this->m_arrivalCell.m_y];
            CGrunt* found = m_tileMgr->FindNearestEnemy(this);
            if (found == NULL || found == slot) {
                if (slot == NULL || slot->m_entranceCommitted == 0
                    || GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
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
                    if (this->m_poweredUp == 0 && this->m_stamina > 99
                        && RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) != 0
                        && slot->m_object->m_screenX == slot->m_lastTilePx.m_x
                        && slot->m_object->m_screenY == slot->m_lastTilePx.m_y) {
                        Coord cp = slot->m_lastTilePx;
                        CommitNeighbor(slot->m_tileOwnerHi, slot->m_tileOwnerLo, cp.m_x, cp.m_y);
                        this->m_defenderState = AISTATE_ATTACK;
                    }
                }
            } else {
                this->m_arrivalCell.m_x = -1;
                this->m_defenderState = AISTATE_SEEK;
                this->m_arrivalCell.m_y = -1;
            }
            break;
        }
        case AISTATE_ATTACK: {
            if (m_poweredUp != 0) {
                CGrunt* slot =
                    m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                if (slot == NULL) {
                    m_defenderState = AISTATE_SEEK;
                    break;
                }
                if (GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0
                    || slot->m_entranceCommitted == 0) {
                    goto repath;
                }
                if (m_combatActive != 0 || m_neighborValid != 0 || m_stamina < STAMINA_FULL) {
                    break;
                }
                if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0
                    || slot->m_object->m_screenX != slot->m_lastTilePx.m_x
                    || slot->m_object->m_screenY != slot->m_lastTilePx.m_y) {
                    goto repath;
                }
                {
                    Coord cp = slot->m_lastTilePx;
                    CommitNeighbor(slot->m_tileOwnerHi, slot->m_tileOwnerLo, cp.m_x, cp.m_y);
                }
                break;
            repath:
                m_defenderState = AISTATE_CHASE;
                {
                    // Retail expands the bounds test here (0xf03f8) and CALLS it
                    // from the AISTATE_SEEK arm above (0xf06a0), so this arm takes
                    // the inline PointInRect sibling.
                    const RECT& view = g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                    i32 px = m_object->m_screenX;
                    i32 py = m_object->m_screenY;
                    if (CGameLevel::PointInRect(&view, px, py)) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
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
                CoordNode* p = this->CoordHead();
                while (p != NULL) {
                    CoordNode* next = p->m_next;
                    Coord** link = &p->m_coord;
                    p = next;
                    if (*link != NULL) {
                        CoordPoolNode* n2 = g_coordPool.NodeOf(*link);
                        n2->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = n2;
                    }
                }
                m_coordList.RemoveAll();
            }
            g_gameReg->m_cmdGrid->ApplyTriggerA(
                m_tileOwnerHi,
                m_tileOwnerLo,
                cell->m_x * 0x20 + 0x10,
                cell->m_y * 0x20 + 0x10
            );
        }
    }
    return 1;
}
