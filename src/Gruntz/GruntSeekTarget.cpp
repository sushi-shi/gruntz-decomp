#include <Enums.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h>
#include <stdlib.h>
#include <Gruntz/FreeNodePool.h>
#include <MfcWin.h>
#include <new>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <rva.h>
#include <Gruntz/GruntDirStatics.h>

#pragma intrinsic(strcmp)

#define IABS(v) ((v) = ((v) ^ ((v) >> 31)) - ((v) >> 31))

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <limits.h>

#define GRID_BOUNDS(grid)                                                                          \
    {                                                                                              \
        CRect ra(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        ra = rb;                                                                                   \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define RECYCLE_COORDS(head)                                                                       \
    {                                                                                              \
        CoordNode* n = (head);                                                                     \
        while (n != 0) {                                                                           \
            CoordNode* next = n->m_next;                                                           \
            void* pay = n->m_coord;                                                                \
            if (pay != 0) {                                                                        \
                CoordPoolNode* slot = g_coordPool.NodeOf(pay);                                     \
                slot->m_next = g_coordPool.m_freeHead;                                             \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
            n = next;                                                                              \
        }                                                                                          \
    }

#define GRID_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        CRect ra(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        ra = rb;                                                                                   \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define GRID_RECT_INLINE(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        ra.left = 0;                                                                               \
        ra.top = 0;                                                                                \
        ra.right = (grid)->m_width;                                                                \
        ra.bottom = (grid)->m_height;                                                              \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_width;                                                                \
        rb.bottom = (grid)->m_height;                                                              \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define DRAIN_COORDS()                                                                             \
    if (CoordCount() != 0) {                                                                       \
        POSITION dpos = m_coordList.GetHeadPosition();                                             \
        while (dpos != 0) {                                                                        \
            Coord* cur = static_cast<Coord*>(m_coordList.GetNext(dpos));                           \
            if (cur != 0) {                                                                        \
                g_coordPool.Push(cur);                                                             \
            }                                                                                      \
        }                                                                                          \
        m_coordList.RemoveAll();                                                                   \
    }

// @early-stop
#define PRIO(dst, r)                                                                               \
    switch (r) {                                                                                   \
        case PICKUP_BOMB:                                                                          \
            dst = 2;                                                                               \
            break;                                                                                 \
        case PICKUP_WELDER:                                                                        \
            dst = 3;                                                                               \
            break;                                                                                 \
        case PICKUP_SWORD:                                                                         \
            dst = 4;                                                                               \
            break;                                                                                 \
        case PICKUP_GUNHAT:                                                                        \
            dst = 5;                                                                               \
            break;                                                                                 \
        case PICKUP_CLUB:                                                                          \
            dst = 6;                                                                               \
            break;                                                                                 \
        case PICKUP_ROCK:                                                                          \
            dst = 7;                                                                               \
            break;                                                                                 \
        case PICKUP_SHOVEL:                                                                        \
            dst = 8;                                                                               \
            break;                                                                                 \
        case PICKUP_BOOMERANG:                                                                     \
            dst = 9;                                                                               \
            break;                                                                                 \
        case PICKUP_SPRING:                                                                        \
            dst = 10;                                                                              \
            break;                                                                                 \
        case PICKUP_GAUNTLETZ:                                                                     \
            dst = 11;                                                                              \
            break;                                                                                 \
        case PICKUP_WINGZ:                                                                         \
            dst = 12;                                                                              \
            break;                                                                                 \
        case PICKUP_SPY:                                                                           \
            dst = 13;                                                                              \
            break;                                                                                 \
        case PICKUP_BRICK:                                                                         \
            dst = 14;                                                                              \
            break;                                                                                 \
        case PICKUP_GRAVITYBOOTZ:                                                                  \
            dst = 15;                                                                              \
            break;                                                                                 \
        case PICKUP_SHIELD:                                                                        \
            dst = 16;                                                                              \
            break;                                                                                 \
        case PICKUP_GOOBER:                                                                        \
            dst = 17;                                                                              \
            break;                                                                                 \
        case PICKUP_TOOB:                                                                          \
            dst = 18;                                                                              \
            break;                                                                                 \
        case PICKUP_GLOVEZ:                                                                        \
            dst = 19;                                                                              \
            break;                                                                                 \
        case PICKUP_TIMEBOMB:                                                                      \
            dst = 20;                                                                              \
            break;                                                                                 \
        case PICKUP_NERFGUN:                                                                       \
            dst = 21;                                                                              \
            break;                                                                                 \
        case PICKUP_WAND:                                                                          \
            dst = 22;                                                                              \
            break;                                                                                 \
        default:                                                                                   \
            dst = 23;                                                                              \
            break;                                                                                 \
    }

// @early-stop
RVA(0x000f71c0, 0x721)
i32 CGrunt::SeekTarget() {
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;
    if (this->CoordCount() != 0
        && g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + this->m_arrivalCell.m_x] == NULL) {
        CoordNode* p = this->CoordHead();
        while (p != NULL) {
            CoordNode* next = p->m_next;
            Coord** link = &p->m_coord;
            p = next;
            if (*link != NULL) {
                g_coordPool.Push(*link);
            }
        }
        m_coordList.RemoveAll();
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
        CGrunt* slot = g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + reason];
        if (slot == NULL || slot->m_entranceCommitted == 0) {
            if (this->CoordCount() != 0) {
                CoordNode* p = this->CoordHead();
                while (p != NULL) {
                    CoordNode* next = p->m_next;
                    Coord** link = &p->m_coord;
                    p = next;
                    if (*link != NULL) {
                        g_coordPool.Push(*link);
                    }
                }
                m_coordList.RemoveAll();
            }
            this->m_arrivalCell.m_x = -1;
            return 1;
        }

        Coord selfTile;
        GetScreenPos(&selfTile);
        selfTile.m_x >>= 5;
        selfTile.m_y >>= 5;
        Coord slotTile;
        slot->GetScreenPos(&slotTile);
        slotTile.m_x >>= 5;
        slotTile.m_y >>= 5;
        Coord selfTileB;
        GetScreenPos(&selfTileB);
        selfTileB.m_x >>= 5;
        selfTileB.m_y >>= 5;
        Coord slotTileB;
        slot->GetScreenPos(&slotTileB);
        slotTileB.m_x >>= 5;
        slotTileB.m_y >>= 5;
        i32 dx = selfTile.m_x - slotTile.m_x;
        i32 dy = selfTileB.m_y - slotTileB.m_y;
        if (((dx ^ (dx >> 31)) - (dx >> 31)) < 2 && ((dy ^ (dy >> 31)) - (dy >> 31)) < 2) {
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
                CoordNode* p = this->CoordHead();
                while (p != NULL) {
                    CoordNode* next = p->m_next;
                    Coord** link = &p->m_coord;
                    p = next;
                    if (*link != NULL) {
                        g_coordPool.Push(*link);
                    }
                }
                m_coordList.RemoveAll();
                return 1;
            }
        }
    }

    reason = IDX(this->m_entranceReason);
    if (reason > 0x16) {
        reason = IDX(this->m_toolId);
    }
    if (reason == 0) {
        if (this->CoordCount() == 0) {
            if (this->m_defenderState != AISTATE_SEEK) {
                return 1;
            }
            i32 best = INT_MAX;
            i32 bestIdx = -1;
            CGrunt** slots = g_gameReg->m_cmdGrid->m_grid;
            i32 i = 0;
            do {
                CGrunt* sv = slots[i];
                if (sv != NULL && sv->m_entranceCommitted != 0) {
                    PickupType k = sv->m_entranceReason;
                    PickupType kk = k;
                    if (k > PICKUP_EQUIPPABLE_LAST) {
                        kk = sv->m_toolId;
                    }
                    if (kk != PICKUP_NONE && kk != PICKUP_WARPSTONE && kk != PICKUP_BOMB
                        && !(
                            k > PICKUP_EQUIPPABLE_LAST ? (sv->m_toolId == PICKUP_WARPSTONE) : false
                        )
                        && sv->m_gruntKind != GRUNT_GHOST) {
                        i32 ex = sv->m_object->m_screenX >> TILE_SHIFT_PX;
                        i32 ddx = ex - (this->m_object->m_screenX >> TILE_SHIFT_PX);
                        i32 ey = (sv->m_object->m_screenY >> TILE_SHIFT_PX)
                                 - (this->m_object->m_screenY >> TILE_SHIFT_PX);
                        i32 dist = ddx * ddx + ey * ey;
                        if (dist < best
                            && dist <= this->m_defenderRadius * this->m_defenderRadius) {
                            best = dist;
                            bestIdx = i;
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
                    if (bx < board->right && board->left <= bx && by < board->bottom
                        && board->top <= by) {
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
        if (static_cast<u32>(this->m_dwell) < 0x3e9) {
            return 1;
        }
        CGameObject* base =
            g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + this->m_arrivalCell.m_x]->m_object;
        TileSwitch(
            base->m_screenX >> TILE_SHIFT_PX,
            base->m_screenY >> TILE_SHIFT_PX,
            0,
            this->m_arrivalFlags,
            1,
            0
        );
    } else {
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
            if (this->m_neighborValid != 0) {
                this->m_neighborValid = 0;
                return 1;
            }
            if (this->m_combatActive != 0) {
                return 1;
            }
            if (this->m_stamina < STAMINA_FULL) {
                if (atTarget) {
                    return 1;
                }
                if (this->m_poweredUp == 0) {
                    return 1;
                }
                this->m_entranceActive = 0;
                this->m_combatActive = 0;
                this->m_neighborValid = 0;
                this->m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
                return 1;
            }
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
        this->m_defenderPx.m_x = this->m_lastTilePx.m_x;
        this->m_defenderPx.m_y = this->m_lastTilePx.m_y;
        if (g == NULL || g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
            this->m_blockedVoicePending = 0;
            return 1;
        }
        if (this->m_poweredUp == 0 && this->m_stamina > 99) {
            i32 x = g->m_object->m_screenX;
            if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y

                && RectContains(x, g->m_object->m_screenY) != 0) {
                CommitNeighbor(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    g->m_lastTilePx.m_x,
                    g->m_lastTilePx.m_y
                );
            }
        }
        if (static_cast<u32>(this->m_dwell) < 0x1f5) {
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
    }
    this->m_dwell = 0;
    return 1;
}
