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
// objdiff pairs the symbol but scores 0 - the instruction alignment
// carries more inserts+deletes than matches, so the body still diverges structurally.
RVA(0x000ee800, 0x971)
i32 CGrunt::ArrivalReticleScan() {
    i32 defTX = m_defenderPx.m_x >> TILE_SHIFT_PX;
    i32 defTY = m_defenderPx.m_y >> TILE_SHIFT_PX;

    i32 scanRadius = m_defenderRadius + m_reachRect.right - 1;
    RECT scanBounds;
    scanBounds.left = defTX - scanRadius;
    scanBounds.top = defTY - scanRadius;
    scanBounds.right = defTX + scanRadius + 1;
    scanBounds.bottom = defTY + scanRadius + 1;

    Coord pt;
    GetScreenPos(&pt);
    i32 dTX = abs((pt.m_x >> TILE_SHIFT_PX) - (m_defenderPx.m_x >> TILE_SHIFT_PX));
    GetScreenPos(&pt);
    i32 dTY = abs((pt.m_y >> TILE_SHIFT_PX) - (m_defenderPx.m_y >> TILE_SHIFT_PX));
    i32 dist = dTX > dTY ? dTX : dTY;
    if (dist > m_defenderRadius) {
        m_defenderPx.m_x = m_lastTilePx.m_x;
        m_defenderPx.m_y = m_lastTilePx.m_y;
        return 1;
    }

    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    i32 occOnTile = 0;
    if (occ) {
        CGameObject* oo = occ->m_object;
        if (oo->m_screenX == occ->m_lastTilePx.m_x && oo->m_screenY == occ->m_lastTilePx.m_y) {
            if (RectContains(oo->m_screenX, oo->m_screenY)) {
                occOnTile = 1;
            }
        }
    }

    if (m_poweredUp) {
        if (m_neighborValid) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive) {
            return 1;
        }
        if (m_stamina >= STAMINA_FULL) {
            if (FindGridNeighbor(1)) {
                return 1;
            }
            if (occOnTile && occ == NULL) {
                return 1;
            }
        } else {
            if (occOnTile) {
                return 1;
            }
        }
        if (m_neighborValid) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (occ == NULL) {
        m_blockedVoicePending = 0;
    } else {
        if (m_neighborValid) {
            return 1;
        }
        if (m_combatActive == 0 && m_stamina >= STAMINA_FULL && occOnTile) {
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePx.m_x,
                occ->m_lastTilePx.m_y
            );
            if (CoordCount()) {
                for (CoordNode* n = CoordHead(); n; n = n->m_next) {
                    if (n->m_coord) {
                        g_coordPool.Push(n->m_coord);
                    }
                }
                m_coordList.RemoveAll();
            }
            return 1;
        }
        if (occOnTile) {
            if (CoordCount()) {
                for (CoordNode* n = CoordHead(); n; n = n->m_next) {
                    if (n->m_coord) {
                        g_coordPool.Push(n->m_coord);
                    }
                }
                m_coordList.RemoveAll();
            }
            return 1;
        }
    }

    CMapMgr* grid = g_gameReg->m_tileGrid;
    if (occ != NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
        i32 occTX = occ->m_object->m_screenX >> TILE_SHIFT_PX;
        i32 occTY = occ->m_object->m_screenY >> TILE_SHIFT_PX;
        i32 dx = abs(occTX - defTX);
        i32 dy = abs(occTY - defTY);
        i32 radius = dx > dy ? dx : dy;

        if (radius < m_defenderRadius + m_reachRect.right) {
            if (m_blockedVoicePending != 0) {
                const RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(view, m_object->m_screenX, m_object->m_screenY)
                    != 0) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = 0;
            }

            POINT target;
            target.x = occTX;
            target.y = occTY;
            if (PtInRect(&scanBounds, target) != 0 && m_defenderRadius > 1) {
                RECT oldBounds = grid->m_bounds;
                CDWordArray saved;
                for (i32 y = oldBounds.top; y <= oldBounds.bottom; y++) {
                    for (i32 x = oldBounds.left; x <= oldBounds.right; x++) {
                        if (static_cast<u32>(x) < grid->m_width
                            && static_cast<u32>(y) < grid->m_height) {
                            saved.SetAtGrow(
                                saved.GetSize(),
                                static_cast<DWORD>(grid->m_rowInts[y][x * 7])
                            );
                        }
                    }
                }

                i32 left = defTX - m_defenderRadius;
                i32 right = defTX + m_defenderRadius;
                i32 top = defTY - m_defenderRadius;
                i32 bottom = defTY + m_defenderRadius;
                for (i32 borderX = left; borderX <= right; borderX++) {
                    if (static_cast<u32>(borderX) < grid->m_width
                        && static_cast<u32>(top) < grid->m_height
                        && (borderX != occTX || top != occTY)) {
                        grid->m_rowInts[top][borderX * 7] = 1;
                    }
                    if (static_cast<u32>(borderX) < grid->m_width
                        && static_cast<u32>(bottom) < grid->m_height
                        && (borderX != occTX || bottom != occTY)) {
                        grid->m_rowInts[bottom][borderX * 7] = 1;
                    }
                }
                for (i32 borderY = top; borderY <= bottom; borderY++) {
                    if (static_cast<u32>(left) < grid->m_width
                        && static_cast<u32>(borderY) < grid->m_height
                        && (left != occTX || borderY != occTY)) {
                        grid->m_rowInts[borderY][left * 7] = 1;
                    }
                    if (static_cast<u32>(right) < grid->m_width
                        && static_cast<u32>(borderY) < grid->m_height
                        && (right != occTX || borderY != occTY)) {
                        grid->m_rowInts[borderY][right * 7] = 1;
                    }
                }

                TileSwitch(occTX, occTY, 0, m_arrivalFlags, 1, 0);

                i32 savedIndex = 0;
                for (i32 restoreY = oldBounds.top; restoreY <= oldBounds.bottom; restoreY++) {
                    for (i32 restoreX = oldBounds.left; restoreX <= oldBounds.right; restoreX++) {
                        if (static_cast<u32>(restoreX) < grid->m_width
                            && static_cast<u32>(restoreY) < grid->m_height) {
                            grid->m_rowInts[restoreY][restoreX * 7] = saved.GetAt(savedIndex++);
                        }
                    }
                }

                Coord* previous = 0;
                POSITION pos = m_coordList.GetHeadPosition();
                POSITION trimPos = 0;
                Coord* trimCoord = 0;
                i32 foundOutside = 0;
                while (pos != NULL) {
                    trimPos = pos;
                    trimCoord = static_cast<Coord*>(m_coordList.GetNext(pos));
                    i32 pathDx = abs(trimCoord->m_x - defTX);
                    i32 pathDy = abs(trimCoord->m_y - defTY);
                    i32 pathDist = pathDx > pathDy ? pathDx : pathDy;
                    if (pathDist > m_defenderRadius - 1) {
                        foundOutside = 1;
                        break;
                    }
                    previous = trimCoord;
                }

                if (foundOutside != 0) {
                    if (previous == NULL) {
                        SetEntrancePos(1, 1);
                        DRAIN_COORDS();
                    } else {
                        i32 pathDx = abs(previous->m_x - occTX);
                        i32 pathDy = abs(previous->m_y - occTY);
                        i32 pathDist = pathDx > pathDy ? pathDx : pathDy;
                        if (pathDist <= m_reachRect.right) {
                            g_coordPool.Push(trimCoord);
                            m_coordList.RemoveAt(trimPos);
                            while (pos != NULL) {
                                POSITION nextPos = pos;
                                Coord* coord = static_cast<Coord*>(m_coordList.GetNext(pos));
                                if (coord != NULL) {
                                    g_coordPool.Push(coord);
                                }
                                m_coordList.RemoveAt(nextPos);
                            }
                        } else {
                            SetEntrancePos(1, 1);
                            DRAIN_COORDS();
                        }
                    }
                }
            }
        } else if ((m_object->m_screenX >> TILE_SHIFT_PX) != defTX
                   || (m_object->m_screenY >> TILE_SHIFT_PX) != defTY) {
            TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
        }
        m_dwell = 0;
    } else if (occ == NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS
               && ((m_object->m_screenX >> TILE_SHIFT_PX) != defTX
                   || (m_object->m_screenY >> TILE_SHIFT_PX) != defTY)) {
        TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
    }

    GRID_RECT_INLINE(grid);

    return 1;
}
