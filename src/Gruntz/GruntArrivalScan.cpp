#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h>
#include <stdlib.h>
#include <Gruntz/FreeNodePool.h>
#include <Wap32/Rect.h>
#include <new>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <rva.h>

#pragma intrinsic(strcmp)

#define IABS(v) ((v) = ((v) ^ ((v) >> 31)) - ((v) >> 31))

#include <Gruntz/FreeNodePool.h>

#define GRID_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
        RECT* pb = static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));   \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
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
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
        RECT* pb = static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));   \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
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
RVA(0x000ec670, 0x298)
i32 CGrunt::ResolveArrivalReposition() {
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    if (occ != 0 && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
        if (static_cast<u32>(m_dwell) > 0xfa) {
            CGameObject* oh = occ->m_object;
            if (TileSwitch(oh->m_screenX >> 5, oh->m_screenY >> 5, 0, m_arrivalFlags, 1, 0) != 0) {
                CGameObject* oh2 = occ->m_object;
                if (m_tileMgr->ApplyTriggerA(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        oh2->m_screenX,
                        oh2->m_screenY
                    )
                    == -1) {
                    m_dwell = 0;
                    if (m_blockedVoicePending != 0) {
                        CWwdGameObjectA* h = m_object;
                        i32 vx = h->m_screenX;
                        i32 vy = h->m_screenY;
                        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                        if (vx < rect->right && vx >= rect->left && vy < rect->bottom
                            && vy >= rect->top) {
                            g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                        }
                        m_blockedVoicePending = 0;
                        m_dwell = 0;
                        return 1;
                    }
                }
            }
            goto L8a2;
        }
        return 1;
    }

    {
        u32 dwell = static_cast<u32>(m_dwell);
        if (dwell > 0x3e8 && m_resetApplied == 0 && m_hasExtent != 0 && dwell > 0xbb8) {

            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_arrivalReroll64
                < m_arrivalRerollWindow64) {

                CWwdGameObjectA* h = m_object;
                i32 spanX = abs(h->m_extent.right - h->m_extent.left);
                i32 spanY = abs(h->m_extent.bottom - h->m_extent.top);
                i32 outX = h->m_extent.left;
                i32 outY = h->m_extent.top;
                if (spanX != 0) {
                    outX += rand() % spanX;
                }
                if (spanY != 0) {
                    outY += rand() % spanY;
                }
                TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
            } else {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollLo = 0;
                m_arrivalRerollWindowLo = 0;
                m_arrivalRerollHi = 0;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
            }
            m_blockedVoicePending = 1;
            goto L8a2;
        }
    }
    return 1;

L8a2:
    m_dwell = 0;
    return 1;
}

// @early-stop
RVA(0x000ecc90, 0x86a)
i32 CGrunt::StepBrickLayerBehavior() {
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    Coord c1[2];
    GetScreenPos(c1);
    i32 cx = c1[0].m_x >> 5;
    Coord c2[2];
    GetScreenPos(c2);
    i32 cy = c2[0].m_y >> 5;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = g->m_object->m_screenX;
        if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(x, g->m_object->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        }
        if (m_neighborValid != 0) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (g == 0) {
        m_blockedVoicePending = 0;
        goto L_ed006;
    }
    if (m_neighborValid != 0) {
        return 1;
    }
    if (m_combatActive == 0 && m_stamina >= 100) {
        if (atTarget) {
            CommitNeighbor(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                g->m_lastTilePx.m_x,
                g->m_lastTilePx.m_y
            );
            DRAIN_COORDS();
            return 1;
        }
    } else {
        if (atTarget) {
            DRAIN_COORDS();
            return 1;
        }
    }

L_ed006:
    if (g == 0 || static_cast<u32>(m_dwell) <= 0x1f4
        || GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
        m_blockedVoicePending = 0;
        goto L_ed153;
    }
    if (m_poweredUp != 0) {
        goto L_ed153;
    }
    if (m_stamina >= 100 && g->m_object->m_screenX == g->m_lastTilePx.m_x
        && g->m_object->m_screenY == g->m_lastTilePx.m_y
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        CommitNeighbor(
            g->m_tileOwnerHi,
            g->m_tileOwnerLo,
            g->m_lastTilePx.m_x,
            g->m_lastTilePx.m_y
        );
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != 0) {
        goto L_ed153;
    }
    if (TileSwitch(
            g->m_object->m_screenX >> 5,
            g->m_object->m_screenY >> 5,
            0,
            m_arrivalFlags,
            1,
            0
        )
        == 0) {
        goto L_ed153;
    }
    if (m_blockedVoicePending != 0) {
        CCueRect* board = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        if (x < board->right && board->left <= x && y < board->bottom && board->top <= y) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
        }
        m_blockedVoicePending = 0;
    }
    m_dwell = 0;

L_ed153:
    if (CoordCount() != 0) {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        BrickzCell* cell = &grid->m_rows[row][col];
        if ((cell->m_flags & 0x8000) != 0 || cell->m_typeCode == 0x97 || cell->m_typeCode == 0x98) {
            m_tileMgr
                ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, (col << 5) + 0x10, (row << 5) + 0x10);
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
        return 1;
    }
    if (static_cast<u32>(m_dwell) <= 0x3e8) {
        return 1;
    }

    i32 r = m_defenderRadius;
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_width;
    gb.bottom = grid->m_height;
    RECT isect;
    if (!IntersectRect(&isect, &box, &gb)) {
        isect = box;
    }

    i32 best = 0x7fffffff;
    i32 bestCol = -1;
    i32 bestRow = -1;
    {
        RECT lb;
        lb.left = isect.left;
        lb.top = isect.top;
        lb.right = isect.right + 1;
        lb.bottom = isect.bottom + 1;
        RECT gb2;
        gb2.left = 0;
        gb2.top = 0;
        gb2.right = grid->m_width;
        gb2.bottom = grid->m_height;
        if (!IntersectRect(&grid->m_bounds, &lb, &gb2)) {
            grid->m_bounds = lb;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
    }
    for (i32 row = isect.top; row < isect.bottom; row++) {
        BrickzCell* cell = &grid->m_rows[row][isect.left];
        for (i32 col = isect.left; col < isect.right; col++) {
            if ((cell->m_flags & 0x8000) != 0 || cell->m_typeCode == 0x97
                || cell->m_typeCode == 0x98) {
                i32 dr = row - cy;
                IABS(dr);
                i32 dc = col - cx;
                IABS(dc);
                i32 dist = dr + dc;
                if (dist < best) {
                    best = dist;
                    bestCol = col;
                    bestRow = row;
                }
            }
            cell++;
        }
    }
    if (best != 0x7fffffff) {
        i32 dc = bestCol - cx;
        IABS(dc);
        i32 dr = bestRow - cy;
        IABS(dr);
        if (dc <= 1 && dr <= 1) {
            m_tileMgr->ApplyTriggerA(
                m_tileOwnerHi,
                m_tileOwnerLo,
                (bestCol << 5) + 0x10,
                (bestRow << 5) + 0x10
            );
            SetEntrancePos(1, 1);
        } else {
            TileSwitch(bestCol, bestRow, 0, m_arrivalFlags, 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    m_dwell = 0;
    return 1;
}

// @early-stop
RVA(0x000ed9f0, 0x8dd)
i32 CGrunt::WanderStep() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

    i32 flag = 0;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    if (g != 0) {
        i32 gx = g->m_object->m_screenX;
        if (gx == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(gx, g->m_object->m_screenY) != 0) {
            flag = 1;
        }
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
        } else if (m_combatActive == 0) {
            bool reset;
            if (m_stamina >= 0x64) {
                if (FindGridNeighbor(1) != 0) {
                    m_defenderState = 5;
                    return 1;
                }
                reset = !(flag != 0 && g == 0);
            } else {
                reset = (flag == 0);
            }
            if (reset) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
        }
        m_defenderState = 5;
    }

    switch (m_defenderState) {
        case 0:
            if (g != 0) {
                if (m_poweredUp == 0 && m_stamina >= 0x64
                    && g->m_object->m_screenX == g->m_lastTilePx.m_x
                    && g->m_object->m_screenY == g->m_lastTilePx.m_y
                    && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
                    CommitNeighbor(
                        g->m_tileOwnerHi,
                        g->m_tileOwnerLo,
                        g->m_lastTilePx.m_x,
                        g->m_lastTilePx.m_y
                    );
                    m_neighborScanEnabled = 0;
                    if (CoordCount() != 0) {
                        void* node = m_coordList.GetHeadPosition();
                        if (node != 0) {
                            do {
                                CoordNode* cur = static_cast<CoordNode*>(node);
                                node = *static_cast<void**>(node);
                                Coord* data = cur->m_coord;
                                if (data != 0) {
                                    g_coordPool.Push(data);
                                }
                            } while (node != 0);
                        }
                        m_coordList.RemoveAll();
                    }
                    m_defenderState = 5;
                    return 1;
                }
                if (static_cast<u32>(m_dwell) > 0x3e8) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        Coord c[2];
                        g->GetScreenPos(c);
                        if (TileSwitch(c[0].m_x >> 5, c[0].m_y >> 5, 0, m_arrivalFlags, 1, 0)
                            != 0) {
                            SetEntrancePos(1, 1);
                            m_arrivalCell.m_x = g->m_tileOwnerHi;
                            m_arrivalCell.m_y = g->m_tileOwnerLo;
                            m_defenderState = 1;
                            if (CGameLevel::PointInBounds(
                                    &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
                                    m_object->m_screenX,
                                    m_object->m_screenY
                                )
                                != 0) {
                                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            goto timeout;

        case 1: {
            CGrunt* slot = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* active = m_tileMgr->FindNearestEnemy(this);
            if (active != 0 && active != slot) {
                m_arrivalCell.m_x = -1;
                m_defenderState = 0;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (slot == 0 || slot->m_entranceCommitted == 0
                || GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 0x1f4) {
                StepArrivalDrop(
                    slot->m_lastTilePx.m_x,
                    slot->m_lastTilePx.m_y,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                );
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                return 1;
            }
            if (slot->m_object->m_screenX != slot->m_lastTilePx.m_x) {
                return 1;
            }
            if (slot->m_object->m_screenY != slot->m_lastTilePx.m_y) {
                return 1;
            }
            CommitNeighbor(
                slot->m_tileOwnerHi,
                slot->m_tileOwnerLo,
                slot->m_lastTilePx.m_x,
                slot->m_lastTilePx.m_y
            );
            m_neighborScanEnabled = 0;
            if (CoordCount() != 0) {

                POSITION pos = m_coordList.GetHeadPosition();
                while (pos != 0) {
                    void* data = m_coordList.GetNext(pos);
                    if (data != 0) {
                        g_coordPool.Push(data);
                    }
                }
                m_coordList.RemoveAll();
            }
            m_defenderState = 5;
            return 1;
        }

        case 2: {
            if (m_poweredUp == 0) {
                m_defenderState = 0;
                return 1;
            }
            CGrunt* slot = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (slot == 0 || GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0
                || slot->m_entranceCommitted == 0) {
                goto ph1;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                goto ph1;
            }
            if (slot->m_object->m_screenX != slot->m_lastTilePx.m_x) {
                goto ph1;
            }
            if (slot->m_object->m_screenY != slot->m_lastTilePx.m_y) {
                goto ph1;
            }
            CommitNeighbor(
                slot->m_tileOwnerHi,
                slot->m_tileOwnerLo,
                slot->m_lastTilePx.m_x,
                slot->m_lastTilePx.m_y
            );
            m_neighborScanEnabled = 0;
            if (CoordCount() != 0) {
                POSITION pos = m_coordList.GetHeadPosition();
                while (pos != 0) {
                    void* data = m_coordList.GetNext(pos);
                    if (data != 0) {

                        CoordPoolNode* fslot = g_coordPool.NodeOf(data);
                        fslot->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = fslot;
                    }
                }
                m_coordList.RemoveAll();
            }
            m_defenderState = 5;
            m_dwell = 0x1f4;
            return 1;
        ph1:
            m_defenderState = 1;
            m_dwell = 0x1f4;
            return 1;
        }

        case 5: {
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina >= 0x64) {
                m_defenderState = 0;
                return 1;
            }
            if (CoordCount() != 0) {
                return 1;
            }
            CWwdGameObjectA* base = m_object;
            i32 clip = 1;
            i32 py = GameRand() % 4 + (base->m_screenY >> 5) - 2;
            i32 px = GameRand() % 4 + (base->m_screenX >> 5) - 2;
            if (static_cast<u32>(m_arrivalCell.m_x) < 4
                && static_cast<u32>(m_arrivalCell.m_y) < 0xf) {
                CGrunt* entry = g_gameReg->m_cmdGrid
                                    ->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                if (entry != 0) {
                    CGameObject* e10 = entry->m_object;
                    RECT rc;
                    rc.left = (e10->m_screenX >> 5) - 2;
                    rc.top = (e10->m_screenY >> 5) - 2;
                    rc.right = (e10->m_screenX >> 5) + 3;
                    rc.bottom = (e10->m_screenY >> 5) + 3;
                    POINT pt;
                    pt.x = px;
                    pt.y = py;
                    if (PtInRect(&rc, pt)) {
                        clip = 0;
                    }
                }
            }
            if (clip == 0) {
                return 1;
            }
            CMapMgr* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(px) >= static_cast<u32>(grid->m_width)) {
                return 1;
            }
            if (static_cast<u32>(py) >= static_cast<u32>(grid->m_height)) {
                return 1;
            }
            TileSwitch(px, py, 0, m_arrivalFlags, 1, 0);
            return 1;
        }

        default:
            return 1;
    }

timeout:
    if (m_resetApplied == 0 && m_hasExtent != 0 && static_cast<u32>(m_dwell) > 0xbb8) {
        i32 hi =
            -static_cast<i32>((static_cast<u32>(g_frameTime) < static_cast<u32>(m_arrivalRerollLo)))
            - m_arrivalRerollHi;
        i32 lo = static_cast<i32>((g_frameTime - static_cast<u32>(m_arrivalRerollLo)));
        if (m_arrivalRerollWindowHi < hi
            || (m_arrivalRerollWindowHi == hi
                && static_cast<u32>(lo) >= static_cast<u32>(m_arrivalRerollWindowLo))) {
            ResetEntranceAnimation(1, 1, 0);
            m_arrivalRerollLo = 0;
            m_arrivalRerollWindowLo = 0;
            m_arrivalRerollHi = 0;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollWindowLo = GameRand() % 30000 + 30000;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollLo = static_cast<i32>(g_frameTime);
            m_arrivalRerollHi = 0;
        } else {
            CWwdGameObjectA* base = m_object;
            u32 lx = static_cast<u32>(base->m_extent.left);
            i32 dxr = base->m_extent.right - static_cast<i32>(lx);
            i32 ax = (dxr ^ (dxr >> 31)) - (dxr >> 31);
            u32 ly = static_cast<u32>(base->m_extent.top);
            i32 dyr = base->m_extent.bottom - static_cast<i32>(ly);
            i32 ay = (dyr ^ (dyr >> 31)) - (dyr >> 31);
            if (ax != 0) {
                lx += GameRand() % ax;
            }
            if (ay != 0) {
                ly += GameRand() % ay;
            }
            if (lx < g_gameReg->m_tileGrid->m_width && ly < g_gameReg->m_tileGrid->m_height) {
                TileSwitch(static_cast<i32>(lx), static_cast<i32>(ly), 0, m_arrivalFlags, 1, 0);
            }
            if (CoordCount() != 0) {
                if (ax <= ay) {
                    ax = ay;
                }
                if (ax < CoordCount()) {
                    SetEntrancePos(1, 1);
                    m_dwell = 0;
                    return 1;
                }
            }
        }
        m_dwell = 0;
    }
    return 1;
}

RVA(0x000ee800, 0x971)
i32 CGrunt::ArrivalReticleScan() {
    i32 defTX = m_defenderPx.m_x >> 5;
    i32 defTY = m_defenderPx.m_y >> 5;

    i32 scanRadius = m_defenderRadius + m_reachRect.right - 1;
    RECT scanBounds;
    scanBounds.left = defTX - scanRadius;
    scanBounds.top = defTY - scanRadius;
    scanBounds.right = defTX + scanRadius + 1;
    scanBounds.bottom = defTY + scanRadius + 1;

    Coord pt;
    GetScreenPos(&pt);
    i32 dTX = abs((pt.m_x >> 5) - defTX);
    GetScreenPos(&pt);
    i32 dTY = abs((pt.m_y >> 5) - defTY);
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
        if (m_stamina >= 0x64) {
            if (FindGridNeighbor(1)) {
                return 1;
            }
            if (occOnTile && occ == 0) {
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

    if (occ == 0) {
        m_blockedVoicePending = 0;
    } else {
        if (m_neighborValid) {
            return 1;
        }
        if (m_combatActive == 0 && m_stamina >= 0x64 && occOnTile) {
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
    if (occ != 0 && static_cast<u32>(m_dwell) > 0x1f4) {
        i32 occTX = occ->m_object->m_screenX >> 5;
        i32 occTY = occ->m_object->m_screenY >> 5;
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
                i32 endY = oldBounds.bottom + 1;
                i32 endX = oldBounds.right + 1;
                for (i32 y = oldBounds.top; y < endY; y++) {
                    for (i32 x = oldBounds.left; x < endX; x++) {
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
                for (i32 borderX = left; borderX < right + 1; borderX++) {
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
                for (i32 borderY = top; borderY < bottom + 1; borderY++) {
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
                for (i32 restoreY = oldBounds.top; restoreY < endY; restoreY++) {
                    for (i32 restoreX = oldBounds.left; restoreX < endX; restoreX++) {
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
                while (pos != 0) {
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
                    if (previous == 0) {
                        SetEntrancePos(1, 1);
                        DRAIN_COORDS();
                    } else {
                        i32 pathDx = abs(previous->m_x - occTX);
                        i32 pathDy = abs(previous->m_y - occTY);
                        i32 pathDist = pathDx > pathDy ? pathDx : pathDy;
                        if (pathDist <= m_reachRect.right) {
                            g_coordPool.Push(trimCoord);
                            m_coordList.RemoveAt(trimPos);
                            while (pos != 0) {
                                POSITION nextPos = pos;
                                Coord* coord = static_cast<Coord*>(m_coordList.GetNext(pos));
                                if (coord != 0) {
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
        } else if ((m_object->m_screenX >> 5) != defTX || (m_object->m_screenY >> 5) != defTY) {
            TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
        }
        m_dwell = 0;
    } else if (occ == 0 && static_cast<u32>(m_dwell) > 0x1f4
               && ((m_object->m_screenX >> 5) != defTX || (m_object->m_screenY >> 5) != defTY)) {
        TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
    }

    GRID_RECT_INLINE(grid);

    return 1;
}

// @early-stop
RVA(0x000f0130, 0x7c0)
i32 CGrunt::UpdateArrival() {
    char* name = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
    if (strcmp(name, "I") != 0) {
        return 1;
    }
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    bool atTarget = false;
    if (g != 0) {
        i32 x = g->m_object->m_screenX;
        if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && g->RectContains(x, g->m_object->m_screenY) != 0) {
            atTarget = true;
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
        if (this->m_stamina < 100) {
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
        if (FindGridNeighbor(1) != 0) {
            return 1;
        }
        if (atTarget && g == 0) {
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

    switch (this->m_defenderState) {
        case 0:
            if (g != 0) {
                if (this->m_stamina > 99) {
                    i32 x = g->m_object->m_screenX;
                    if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
                        && g->RectContains(x, g->m_object->m_screenY) != 0) {
                        CommitNeighbor(
                            g->m_tileOwnerHi,
                            g->m_tileOwnerLo,
                            g->m_lastTilePx.m_x,
                            g->m_lastTilePx.m_y
                        );
                        break;
                    }
                }
                if (g != 0 && static_cast<u32>(this->m_dwell) > 1000) {
                    if (g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        Coord c[2];
                        GetScreenPos(c);
                        if (TileSwitch(
                                c[0].m_y >> 5,
                                c[0].m_x >> 5,
                                0,
                                this->m_arrivalFlags,
                                0,
                                0x20
                            )
                            != 0) {
                            SetEntrancePos(1, 1);
                            this->m_arrivalCell.m_x = g->m_tileOwnerHi;
                            this->m_arrivalCell.m_y = g->m_tileOwnerLo;
                            this->m_defenderState = 1;
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
                i32 cmp =
                    -static_cast<i32>(
                        (static_cast<u32>(g_frameTime) < static_cast<u32>(this->m_arrivalRerollLo))
                    )
                    - this->m_arrivalRerollHi;
                if (this->m_arrivalRerollWindowHi < cmp
                    || (this->m_arrivalRerollWindowHi <= cmp
                        && static_cast<u32>(this->m_arrivalRerollWindowLo)
                               <= g_frameTime - static_cast<u32>(this->m_arrivalRerollLo))) {
                    ResetEntranceAnimation(1, 1, 0);
                    this->m_arrivalRerollLo = 0;
                    this->m_arrivalRerollWindowLo = 0;
                    this->m_arrivalRerollHi = 0;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollWindowLo = rand() % 30000 + 30000;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                    this->m_arrivalRerollHi = 0;
                } else {
                    CGameObject* base = this->m_object;
                    u32 lo = base->m_extent.left;
                    i32 dx = base->m_extent.right - static_cast<i32>(lo);
                    i32 ax = (dx ^ (dx >> 31)) - (dx >> 31);
                    u32 lo2 = base->m_extent.top;
                    i32 dy = base->m_extent.bottom - static_cast<i32>(lo2);
                    i32 ay = (dy ^ (dy >> 31)) - (dy >> 31);
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
                        if (ax < this->CoordCount()) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
                this->m_dwell = 0;
            }
            break;
        case 1: {
            CGrunt* slot =
                m_tileMgr->m_grid[this->m_arrivalCell.m_x * TM_GRID_COLS + this->m_arrivalCell.m_y];
            i32 cur = m_tileMgr->FindNearestEnemy(this) ? 1 : 0;
            CGrunt* found = m_tileMgr->FindNearestEnemy(this);
            static_cast<void>(cur);
            if (found == 0 || found == slot) {
                if (slot == 0 || slot->m_entranceCommitted == 0
                    || slot->GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
                    this->m_defenderState = 0;
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
                        && slot->RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY)
                               != 0
                        && slot->m_object->m_screenX == slot->m_lastTilePx.m_x
                        && slot->m_object->m_screenY == slot->m_lastTilePx.m_y) {
                        CommitNeighbor(
                            slot->m_tileOwnerHi,
                            slot->m_tileOwnerLo,
                            slot->m_lastTilePx.m_x,
                            slot->m_lastTilePx.m_y
                        );
                        this->m_defenderState = 2;
                    }
                }
            } else {
                this->m_arrivalCell.m_x = -1;
                this->m_defenderState = 0;
                this->m_arrivalCell.m_y = -1;
            }
            break;
        }
        case 2:
            this->m_defenderState = 1;
            break;
    }

    if (this->CoordCount() != 0) {

        Coord* cell = this->CoordHead()->m_coord;

        BrickzCell& gc = g_gameReg->m_tileGrid->m_rows[cell->m_y][cell->m_x];
        if ((gc.m_flagBytes[0] & 0x20) != 0) {
            SetEntrancePos(1, 1);
            if (this->CoordCount() != 0) {
                CoordNode* p = this->CoordHead();
                while (p != 0) {
                    CoordNode* next = p->m_next;
                    Coord** link = &p->m_coord;
                    p = next;
                    if (*link != 0) {
                        CoordPoolNode* n2 = g_coordPool.NodeOf(*link);
                        n2->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = n2;
                    }
                }
                m_coordList.RemoveAll();
            }
            SetEntrancePos(cell->m_x * 0x20 + 0x10, cell->m_y * 0x20 + 0x10);
        }
    }
    return 1;
}

// @early-stop
RVA(0x000f0e20, 0x928)
i32 CGrunt::StepGooSuckerBehavior() {
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    Coord c1[2];
    GetScreenPos(c1);
    i32 cx = c1[0].m_x >> 5;
    Coord c2[2];
    GetScreenPos(c2);
    i32 cy = c2[0].m_y >> 5;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = g->m_object->m_screenX;
        if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(x, g->m_object->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        }
        if (m_neighborValid != 0) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (g == 0) {
        m_blockedVoicePending = 0;
        goto L_ed006b;
    }
    if (m_neighborValid != 0) {
        return 1;
    }
    if (m_combatActive == 0 && m_stamina >= 100) {
        if (atTarget) {
            CommitNeighbor(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                g->m_lastTilePx.m_x,
                g->m_lastTilePx.m_y
            );
            DRAIN_COORDS();
            return 1;
        }
    } else {
        if (atTarget) {
            DRAIN_COORDS();
            return 1;
        }
    }

L_ed006b:
    if (g == 0 || GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
        m_blockedVoicePending = 0;
        goto L_scanb;
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (m_stamina >= 100 && g->m_object->m_screenX == g->m_lastTilePx.m_x
        && g->m_object->m_screenY == g->m_lastTilePx.m_y
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        CommitNeighbor(
            g->m_tileOwnerHi,
            g->m_tileOwnerLo,
            g->m_lastTilePx.m_x,
            g->m_lastTilePx.m_y
        );
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (static_cast<u32>(m_dwell) <= 0x1f4) {
        goto L_scanb;
    }
    {
        Coord cc[2];
        g->GetScreenPos(cc);
        if (TileSwitch(cc[0].m_x >> 5, cc[0].m_y >> 5, 0, m_arrivalFlags, 1, 0) != 0) {
            if (m_blockedVoicePending != 0) {
                i32 x = m_object->m_screenX;
                i32 y = m_object->m_screenY;
                if (CGameLevel::PointInBounds(
                        &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
                        x,
                        y
                    )
                    != 0) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = 0;
            }
            m_dwell = 0;
        }
    }

L_scanb:
    if (CoordCount() != 0) {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        if (CellTargetable(col, row) != 0) {
            m_tileMgr
                ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, (col << 5) + 0x10, (row << 5) + 0x10);
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
        return 1;
    }
    if (static_cast<u32>(m_dwell) <= 0x3e8) {
        return 1;
    }

    i32 r = m_defenderRadius;
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_width;
    gb.bottom = grid->m_height;
    RECT isect;
    if (!IntersectRect(&isect, &box, &gb)) {
        isect = box;
    }
    {
        RECT lb;
        lb.left = isect.left;
        lb.top = isect.top;
        lb.right = isect.right + 1;
        lb.bottom = isect.bottom + 1;
        RECT gb2;
        gb2.left = 0;
        gb2.top = 0;
        gb2.right = grid->m_width;
        gb2.bottom = grid->m_height;
        if (!IntersectRect(&grid->m_bounds, &lb, &gb2)) {
            grid->m_bounds = lb;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
    }

    i32 best = 0x7fffffff;
    i32 bestX = 0;
    i32 bestY = 0;

    POSITION pos = m_tileMgr->m_baseList.GetHeadPosition();
    while (pos != 0) {
        CGruntPuddle* gg = static_cast<CGruntPuddle*>(m_tileMgr->m_baseList.GetNext(pos));
        if (gg->m_pending == 0) {
            i32 gx = gg->m_tileX;
            i32 gy = gg->m_tileY;
            if (RectContains((gx << 5) + 0x10, (gy << 5) + 0x10) != 0) {
                m_tileMgr->ApplyTriggerA(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    (gx << 5) + 0x10,
                    (gy << 5) + 0x10
                );
                GRID_RECT_BOUNDS(grid);
                return 1;
            }
            i32 dx = gx - (m_object->m_screenX >> 5);
            IABS(dx);
            i32 dy = gy - (m_object->m_screenY >> 5);
            i32 dist = ((dy ^ (dy >> 31)) - (dy >> 31)) + dx;
            if (dist < best) {
                POINT pt;
                pt.x = gx;
                pt.y = gy;
                if (PtInRect(&isect, pt)) {
                    best = dist;
                    bestX = gx;
                    bestY = gy;
                }
            }
        }
    }
    if (best != 0x7fffffff) {
        i32 dx = bestX - cx;
        IABS(dx);
        i32 dy = bestY - cy;
        IABS(dy);
        if (dx <= 1 && dy <= 1) {
            m_tileMgr->ApplyTriggerA(
                m_tileOwnerHi,
                m_tileOwnerLo,
                (bestX << 5) + 0x10,
                (bestY << 5) + 0x10
            );
            SetEntrancePos(1, 1);
        } else {
            TileSwitch(bestX, bestY, 0, m_arrivalFlags, 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    m_dwell = 0;
    return 1;
}

// @early-stop
RVA(0x000f1c70, 0x60d)
i32 CGrunt::StepArrivalDefenseAlt() {
    m_arrivalFlags |= 0x40000;
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    i32 inRange = 0;
    if (occ != 0 && occ->m_object->m_screenX == occ->m_lastTilePx.m_x
        && occ->m_object->m_screenY == occ->m_lastTilePx.m_y
        && RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
        inRange = 1;
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            goto tail;
        }
        if (m_stamina >= 0x64) {
            if (FindGridNeighbor(1) != 0) {
                goto tail;
            }
            if (inRange != 0 && occ == 0) {
                goto tail;
            }
            if (m_poweredUp == 0) {
                goto tail;
            }
            if (m_neighborValid != 0) {
                goto tail;
            }
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
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    switch (m_defenderState) {
        case 0: {
            CGrunt* o = m_tileMgr->FindNearestEnemy(this);
            if (o != 0) {
                if (m_poweredUp != 0) {
                    goto tail;
                }
                if (m_stamina >= 0x64 && o->m_object->m_screenX == o->m_lastTilePx.m_x
                    && o->m_object->m_screenY == o->m_lastTilePx.m_y
                    && RectContains(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                    CommitNeighbor(
                        o->m_tileOwnerHi,
                        o->m_tileOwnerLo,
                        o->m_lastTilePx.m_x,
                        o->m_lastTilePx.m_y
                    );
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
                i32 tx = m_lastTilePx.m_x >> 5;
                i32 ty = m_lastTilePx.m_y >> 5;
                i32 gx = m_defenderPx.m_x >> 5;
                i32 gy = m_defenderPx.m_y >> 5;
                if (tx < gx) {
                    if (ty < gy) {
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
                    if (ty > gy) {
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
                    goto resetState;
                }
                if (tx > gx) {
                    if (ty < gy) {
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
                    if (ty > gy) {
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
                }
                goto resetState;
            }
        }

        case 1: {
            CGrunt* o = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != 0 && g != o) {
                m_arrivalCell.m_x = -1;
                m_defenderState = 0;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (o == 0) {
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
            if (m_stamina < 0x64) {
                goto tail;
            }
            if (RectContains(o->m_object->m_screenX, o->m_object->m_screenY) == 0) {
                goto tail;
            }
            if (o->m_object->m_screenX != o->m_lastTilePx.m_x) {
                goto tail;
            }
            if (o->m_object->m_screenY != o->m_lastTilePx.m_y) {
                goto tail;
            }
            CommitNeighbor(
                o->m_tileOwnerHi,
                o->m_tileOwnerLo,
                o->m_lastTilePx.m_x,
                o->m_lastTilePx.m_y
            );
            m_defenderState = 2;
            return 1;
        }

        case 2:
            m_defenderState = 0;
            return 1;

        case 3: {
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
                m_defenderState = 0;
                return 1;
            }
            CGrunt* o = m_tileMgr->FindNearestEnemy(this);
            if (o == 0) {
                goto tail;
            }
            if (m_poweredUp == 0 && m_stamina >= 0x64
                && o->m_object->m_screenX == o->m_lastTilePx.m_x
                && o->m_object->m_screenY == o->m_lastTilePx.m_y
                && RectContains(o->m_object->m_screenX, o->m_object->m_screenY) != 0) {
                CommitNeighbor(
                    o->m_tileOwnerHi,
                    o->m_tileOwnerLo,
                    o->m_lastTilePx.m_x,
                    o->m_lastTilePx.m_y
                );
                m_defenderState = 2;
            }
            if (GruntInRadius(o->m_tileOwnerHi, o->m_tileOwnerLo) == 0) {
                goto tail;
            }
            m_arrivalCell.m_x = o->m_tileOwnerHi;
            m_arrivalCell.m_y = o->m_tileOwnerLo;
            m_defenderState = 1;
            {
                CWwdGameObjectA* h = m_object;
                i32 x = h->m_screenX;
                i32 y = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (x < rect->right && x >= rect->left && y < rect->bottom && y >= rect->top) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
            goto tail;
        }

        default:
            goto tail;
    }

resetState:
    m_defenderState = 3;
    return 1;

tail:
    return 1;
}

// @early-stop
RVA(0x000f26f0, 0x106)
i32 CGrunt::ResolveArrivalNeighbor() {
    switch (m_defenderState) {
        case 0:
            return 1;
        case 2:
            break;
        default:
            return 1;
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina < 0x64) {
            return 1;
        }
        FindGridNeighbor(1);
        return 1;
    }

    m_defenderState = 0;
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    if (occ == 0) {
        return 1;
    }
    if (m_poweredUp != 0) {
        return 1;
    }
    if (m_stamina < 0x64) {
        return 1;
    }
    if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
        return 1;
    }
    if (m_object->m_screenX != occ->m_lastTilePx.m_x) {
        return 1;
    }
    if (m_object->m_screenY != occ->m_lastTilePx.m_y) {
        return 1;
    }
    CommitNeighbor(
        occ->m_tileOwnerHi,
        occ->m_tileOwnerLo,
        occ->m_lastTilePx.m_x,
        occ->m_lastTilePx.m_y
    );
    return 1;
}

// @early-stop
RVA(0x000f2b20, 0x6e1)
i32 CGrunt::StepArrivalDefense() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CGrunt* occ;
    switch (m_defenderState) {
        case 2:
            if (m_poweredUp == 0) {
                m_defenderState = 1;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (occ == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto c2_occcheck;
            }
            if (occ->m_entranceCommitted == 0) {
                goto c2_occcheck;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
                goto c2_miss;
            }
            if (occ->m_object->m_screenX != occ->m_lastTilePx.m_x) {
                goto c2_miss;
            }
            if (occ->m_object->m_screenY != occ->m_lastTilePx.m_y) {
                goto c2_miss;
            }
            if (m_vehiclePickupType == 0x1e) {
                g_gameReg->m_cmdGrid->ApplyTriggerB(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    occ->m_object->m_screenX,
                    occ->m_object->m_screenY
                );
                return 1;
            }
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePx.m_x,
                occ->m_lastTilePx.m_y
            );
            return 1;
        c2_occcheck:
            if (occ == 0) {
                m_defenderState = 0;
                return 1;
            }
        c2_miss:
            m_defenderState = 1;
            {
                CWwdGameObjectA* h = m_object;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case 1: {
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != 0 && g != occ) {
                m_arrivalCell.m_x = -1;
                m_defenderState = 0;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (occ == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (occ->m_entranceCommitted == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 0x1f4) {
                StepArrivalDrop(
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                );
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
                return 1;
            }
            if (m_vehiclePickupType == 0x1e) {
                g_gameReg->m_cmdGrid->ApplyTriggerB(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    occ->m_object->m_screenX,
                    occ->m_object->m_screenY
                );
                m_defenderState = 2;
                return 1;
            }
            if (occ->m_object->m_screenX == occ->m_lastTilePx.m_x
                && occ->m_object->m_screenY == occ->m_lastTilePx.m_y) {
                CommitNeighbor(
                    occ->m_tileOwnerHi,
                    occ->m_tileOwnerLo,
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y
                );
            }
            m_defenderState = 2;
            return 1;
        }

        case 0:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (occ == 0) {
                goto L_f308a;
            }
            if (m_poweredUp == 0 && m_stamina >= 0x64
                && occ->m_object->m_screenX == occ->m_lastTilePx.m_x
                && occ->m_object->m_screenY == occ->m_lastTilePx.m_y
                && RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
                if (m_vehiclePickupType == 0x1e) {
                    g_gameReg->m_cmdGrid->ApplyTriggerB(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        occ->m_object->m_screenX,
                        occ->m_object->m_screenY
                    );
                    return 1;
                }
                if (occ->m_object->m_screenX != occ->m_lastTilePx.m_x) {
                    return 1;
                }
                if (occ->m_object->m_screenY != occ->m_lastTilePx.m_y) {
                    return 1;
                }
                CommitNeighbor(
                    occ->m_tileOwnerHi,
                    occ->m_tileOwnerLo,
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y
                );
                return 1;
            }
            if (occ == 0) {
                goto L_f308a;
            }
            if (static_cast<u32>(m_dwell) <= 0x3e8) {
                goto L_f308a;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto L_f318a;
            }
            {
                Coord sp;
                occ->GetScreenPos(&sp);
                if (TileSwitch(sp.m_x >> 5, sp.m_y >> 5, 0, m_arrivalFlags, 1, 0) == 0) {
                    goto L_f318a;
                }
                SetEntrancePos(1, 1);
                m_arrivalCell.m_x = occ->m_tileOwnerHi;
                m_arrivalCell.m_y = occ->m_tileOwnerLo;
                m_defenderState = 1;
                CWwdGameObjectA* h = m_object;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(rect, h->m_screenX, h->m_screenY) == 0) {
                    goto L_f318a;
                }
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
            }
        L_f318a:
            m_dwell = 0;
            return 1;
        L_f308a:
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_hasExtent == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= 0xbb8) {
                return 1;
            }
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_arrivalReroll64
                >= m_arrivalRerollWindow64) {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollLo = 0;
                m_arrivalRerollWindowLo = 0;
                m_arrivalRerollHi = 0;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
                m_dwell = 0;
                return 1;
            }
            {
                CWwdGameObjectA* h = m_object;
                i32 baseX = h->m_extent.left;
                i32 spanX = abs(h->m_extent.right - baseX);
                i32 baseY = h->m_extent.top;
                i32 spanY = abs(h->m_extent.bottom - baseY);
                i32 outX = baseX;
                if (spanX != 0) {
                    outX += rand() % spanX;
                }
                i32 outY = baseY;
                if (spanY != 0) {
                    outY += rand() % spanY;
                }
                if (outX < g_gameReg->m_tileGrid->m_width
                    && outY < g_gameReg->m_tileGrid->m_height) {
                    TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
            }
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
}

// @early-stop
RVA(0x000f36a0, 0x78e)
i32 CGrunt::StepDiggerBehavior() {
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0) {
        return 1;
    }
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    Coord c1[2];
    GetScreenPos(c1);
    i32 cx = c1[0].m_x >> 5;
    Coord c2[2];
    GetScreenPos(c2);
    i32 cy = c2[0].m_y >> 5;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = g->m_object->m_screenX;
        if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(x, g->m_object->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        if (atTarget) {
            return 1;
        }
        if (m_poweredUp == 0) {
            return 1;
        }
        if (m_neighborValid != 0) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (g == 0 || GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
        m_blockedVoicePending = 0;
        goto L_tailc;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
    }
    if (m_stamina >= 100 && g->m_object->m_screenX == g->m_lastTilePx.m_x
        && g->m_object->m_screenY == g->m_lastTilePx.m_y
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        CommitNeighbor(
            g->m_tileOwnerHi,
            g->m_tileOwnerLo,
            g->m_lastTilePx.m_x,
            g->m_lastTilePx.m_y
        );
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
    }
    if (static_cast<u32>(m_dwell) <= 0x1f4) {
        goto L_tailc;
    }
    if (TileSwitch(
            g->m_object->m_screenX >> 5,
            g->m_object->m_screenY >> 5,
            0,
            m_arrivalFlags,
            1,
            0
        )
        != 0) {
        if (m_blockedVoicePending != 0) {
            CCueRect* board = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
            i32 x = m_object->m_screenX;
            i32 y = m_object->m_screenY;
            if (x < board->right && board->left <= x && y < board->bottom && board->top <= y) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
            }
            m_blockedVoicePending = 0;
        }
        m_dwell = 0;
    }

L_tailc:
    if (CoordCount() != 0) {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        BrickzCell* cell = &grid->m_rows[row][col];
        if ((cell->m_flags & 0x40) != 0 || (cell->m_flags & 0x10000) != 0) {
            m_tileMgr
                ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, (col << 5) + 0x10, (row << 5) + 0x10);
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
        return 1;
    }
    if (m_poweredUp == 0 && static_cast<u32>(m_dwell) > 0x3e8) {
        i32 r = m_defenderRadius;
        RECT box;
        box.left = cx - r;
        box.right = cx + r;
        box.top = cy - r;
        box.bottom = cy + r;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_width;
        gb.bottom = grid->m_height;
        RECT isect;
        if (!IntersectRect(&isect, &box, &gb)) {
            isect = box;
        }
        {
            RECT lb;
            lb.left = isect.left;
            lb.top = isect.top;
            lb.right = isect.right + 1;
            lb.bottom = isect.bottom + 1;
            RECT gb2;
            gb2.left = 0;
            gb2.top = 0;
            gb2.right = grid->m_width;
            gb2.bottom = grid->m_height;
            if (!IntersectRect(&grid->m_bounds, &lb, &gb2)) {
                grid->m_bounds = lb;
            }
            grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
            grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
        }
        i32 best = 0x7fffffff;
        i32 bestCol = -1;
        i32 bestRow = -1;
        for (i32 row = isect.top; row < isect.bottom; row++) {
            BrickzCell* cell = &grid->m_rows[row][isect.left];
            for (i32 col = isect.left; col < isect.right; col++) {
                if ((cell->m_flags & 0x10000) != 0) {
                    i32 dr = row - cy;
                    IABS(dr);
                    i32 dc = col - cx;
                    IABS(dc);
                    i32 dist = dr + dc;
                    if (dist < best) {
                        best = dist;
                        bestCol = col;
                        bestRow = row;
                    }
                }
                cell++;
            }
        }
        if (best != 0x7fffffff) {
            i32 dc = bestCol - cx;
            IABS(dc);
            i32 dr = bestRow - cy;
            IABS(dr);
            if (dc <= 1 && dr <= 1) {
                m_tileMgr->ApplyTriggerA(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    (bestCol << 5) + 0x10,
                    (bestRow << 5) + 0x10
                );
                SetEntrancePos(1, 1);
            } else {
                TileSwitch(bestCol, bestRow, 0, m_arrivalFlags, 1, 0);
            }
        }
        GRID_RECT_INLINE(grid);
        m_dwell = 0;
    }
    return 1;
}

// @early-stop
RVA(0x000f60f0, 0xb30)
i32 CGrunt::PhaseStep() {
    CDWordArray acc;
    Coord pa;
    Coord pb;

    m_neighborScanEnabled = 0;
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "F") == 0) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

    if (m_defenderState == 0x19) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> 5;
        GetScreenPos(&pb);
        i32 gx = (pb.m_x >> 5) - m_arrivalCell.m_x + ax;
        GetScreenPos(&pa);
        i32 ay = pa.m_y >> 5;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> 5) - m_arrivalCell.m_y + ay;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        m_defenderState = 4;
    }
    if (m_defenderState == 0x1a) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> 5;
        GetScreenPos(&pb);
        GetScreenPos(&pa);
        i32 gx = (pb.m_x >> 5) - m_arrivalCell.m_x + ax;
        i32 ay = pa.m_x >> 5;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> 5) - m_arrivalCell.m_y + ay;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_defenderState = 0;
        return 1;
    }

    if (m_defenderState == 0) {
        goto state0;
    }
    if (m_defenderState == 2) {
        goto state2;
    }
    if (m_defenderState != 4) {
        goto common;
    }
    if (m_dwell <= 0x1f40) {
        return 1;
    }
    m_defenderState = 0;
    return 1;

state2: {
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "F") == 0) {
        goto common;
    }
    i32 x = m_arrivalCell.m_x;
    i32 y = m_arrivalCell.m_y;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    {
        RECT box;
        box.left = x - 4;
        box.top = y - 4;
        box.right = x + 5;
        box.bottom = y + 5;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_width;
        gb.bottom = grid->m_height;
        if (!IntersectRect(&grid->m_bounds, &box, &gb)) {
            grid->m_bounds = box;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
    }
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 1) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), (x << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 1) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 1) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), (x << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 1) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y - 1) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | (y & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y + 1) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y - 1) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | (y & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y + 1) & 0xffff));
    while (acc.GetSize() != 0) {
        i32 sel = rand() % acc.GetSize();
        i32 pt = acc.GetAt(sel);
        i32 px = static_cast<u32>(pt) >> 0x10;
        i32 py = pt & 0xffff;
        CMapMgr* pl = g_gameReg->m_tileGrid;
        i32 flag;
        if (static_cast<u32>(px) < static_cast<u32>(pl->m_width)
            && static_cast<u32>(py) < static_cast<u32>(pl->m_height) && px < pl->m_width
            && py < pl->m_height) {
            flag = pl->m_rows[py][px].m_flags;
        } else {
            flag = 1;
        }
        if ((flag & 0x939) == 0) {
            if (TileSwitch(px, py, 0, m_arrivalFlags, 1, 0) != 0) {
                m_defenderState = 4;
                m_dwell = 0;
                goto build_tail;
            }
        }
        acc.RemoveAt(sel, 1);
    }
build_tail: {
    CMapMgr* pl2 = g_gameReg->m_tileGrid;
    GRID_BOUNDS(pl2);
    acc.~CDWordArray();
    goto common;
}
}

state0: {
    CGrunt* nb = m_tileMgr->FindNearestEnemy(this);
    if (nb == 0) {
        goto common;
    }
    if (nb->m_entranceCommitted == 0) {
        goto common;
    }
    if (m_poweredUp == 0 && m_stamina >= 0x64 && nb->m_object->m_screenX == nb->m_lastTilePx.m_x
        && nb->m_object->m_screenY == nb->m_lastTilePx.m_y
        && RectContains(nb->m_object->m_screenX, nb->m_object->m_screenY) != 0) {
        CommitNeighbor(
            nb->m_tileOwnerHi,
            nb->m_tileOwnerLo,
            nb->m_lastTilePx.m_x,
            nb->m_lastTilePx.m_y
        );
        m_arrivalCell.m_x = nb->m_object->m_screenX >> 5;
        m_arrivalCell.m_y = nb->m_object->m_screenY >> 5;
        m_defenderState = 2;
        goto common;
    }
    if (m_dwell <= 0x1f4) {
        goto common;
    }
    if (GruntInRadius(nb->m_tileOwnerHi, nb->m_tileOwnerLo) == 0) {
        goto s0_reset;
    }
    if (TileSwitch(
            nb->m_object->m_screenX >> 5,
            nb->m_object->m_screenY >> 5,
            0,
            m_arrivalFlags,
            1,
            0
        )
        == 0) {
        m_passableMask |= 0x4020;
        TileSwitch(
            nb->m_object->m_screenX >> 5,
            nb->m_object->m_screenY >> 5,
            0,
            m_arrivalFlags,
            1,
            0
        );
        m_passableMask &= 0xffffbfdf;
    }
    m_dwell = 0;
    if (m_blockedVoicePending == 0) {
        goto common;
    }
    if (CGameLevel::PointInBounds(
            &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
            m_object->m_screenX,
            m_object->m_screenY
        )
        == 0) {
        goto s0_reset;
    }
    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
s0_reset:
    m_blockedVoicePending = 0;
    goto common;
}

common: {
    i32 st = m_defenderState;
    if (st != 4 && st != 0x19 && CoordCount() >= 2) {
        CoordNode* head = CoordHead();
        i32 bx = head->m_coord->m_x;
        i32 by = head->m_coord->m_y;
        Coord* nc = head->m_next->m_coord;
        i32 fx = nc->m_x;
        i32 fy = nc->m_y;
        CMapMgr* pl = g_gameReg->m_tileGrid;
        i32 flag;
        if (static_cast<u32>(fx) < static_cast<u32>(pl->m_width)
            && static_cast<u32>(fy) < static_cast<u32>(pl->m_height)) {
            flag = pl->m_rows[fy][fx].m_flags;
        } else {
            flag = 1;
        }
        if ((flag & 0x20) != 0) {
            if (CoordCount() != 0) {
                RECYCLE_COORDS(CoordHead());
                m_coordList.RemoveAll();
            }
            g_gameReg->m_cmdGrid
                ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, bx * 32 + 16, by * 32 + 16);
            m_arrivalCell.m_x = bx;
            m_arrivalCell.m_y = by;
            m_defenderState = 0x19;
            return 1;
        }
    }
    if (CoordCount() == 0) {
        return 1;
    }
    Coord* head = CoordHead()->m_coord;
    CMapMgr* pl2 = g_gameReg->m_tileGrid;
    i32 gx = head->m_x;
    i32 gy = head->m_y;
    i32 flag2;
    if (static_cast<u32>(gx) < static_cast<u32>(pl2->m_width)
        && static_cast<u32>(gy) < static_cast<u32>(pl2->m_height)) {
        flag2 = pl2->m_rows[gy][gx].m_flags;
    } else {
        flag2 = 1;
    }
    if ((flag2 & 0x20) == 0) {
        return 1;
    }
    m_arrivalCell.m_x = gx;
    m_arrivalCell.m_y = gy;
    if (CoordCount() != 0) {
        RECYCLE_COORDS(CoordHead());
        m_coordList.RemoveAll();
    }
    m_defenderState = 0x1a;
    return 1;
}
}

// @early-stop
RVA(0x000f71c0, 0x721)
i32 CGrunt::SeekTarget() {
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;
    if (this->CoordCount() != 0
        && g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + this->m_arrivalCell.m_x] == 0) {
        CoordNode* p = this->CoordHead();
        while (p != 0) {
            CoordNode* next = p->m_next;
            Coord** link = &p->m_coord;
            p = next;
            if (*link != 0) {
                g_coordPool.Push(*link);
            }
        }
        m_coordList.RemoveAll();
        this->m_arrivalCell.m_x = 0;
    }

    i32 reason = this->m_entranceReason;
    if (reason > 0x16) {
        reason = this->m_toolId;
    }
    if (reason == 0 && (reason = this->m_arrivalCell.m_x, reason >= 0) && reason < 0xf) {
        CGrunt* slot = g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + reason];
        if (slot == 0 || slot->m_entranceCommitted == 0) {
            if (this->CoordCount() != 0) {
                CoordNode* p = this->CoordHead();
                while (p != 0) {
                    CoordNode* next = p->m_next;
                    Coord** link = &p->m_coord;
                    p = next;
                    if (*link != 0) {
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
            i32 r2 = slot->m_entranceReason;
            if (r2 > 0x16) {
                r2 = slot->m_toolId;
            }
            if (r2 != 0x14 && r2 != 1) {
                slot->LoadGruntTypeTable(r2, 1, 0, 0);
                slot->LoadGruntTypeTable(0, 1, 0, 0);
                this->m_defenderState = 4;
                if (this->CoordCount() == 0) {
                    return 1;
                }
                CoordNode* p = this->CoordHead();
                while (p != 0) {
                    CoordNode* next = p->m_next;
                    Coord** link = &p->m_coord;
                    p = next;
                    if (*link != 0) {
                        g_coordPool.Push(*link);
                    }
                }
                m_coordList.RemoveAll();
                return 1;
            }
        }
    }

    reason = this->m_entranceReason;
    if (reason > 0x16) {
        reason = this->m_toolId;
    }
    if (reason == 0) {
        if (this->CoordCount() == 0) {
            if (this->m_defenderState != 0) {
                return 1;
            }
            i32 best = 0x7fffffff;
            i32 bestIdx = -1;
            CGrunt** slots = g_gameReg->m_cmdGrid->m_grid;
            i32 i = 0;
            do {
                CGrunt* sv = slots[i];
                if (sv != 0 && sv->m_entranceCommitted != 0) {
                    i32 k = sv->m_entranceReason;
                    i32 kk = k;
                    if (k > 0x16) {
                        kk = sv->m_toolId;
                    }
                    if (kk != 0 && kk != 0x14 && kk != 1
                        && !(k > 0x16 ? (sv->m_toolId == 0x14) : false)
                        && sv->m_gruntKind != 0x36) {
                        i32 ex = sv->m_object->m_screenX >> 5;
                        i32 ddx = ex - (this->m_object->m_screenX >> 5);
                        i32 ey = (sv->m_object->m_screenY >> 5) - (this->m_object->m_screenY >> 5);
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
                        base->m_screenX >> 5,
                        base->m_screenY >> 5,
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
        if (this->m_defenderState != 0) {
            return 1;
        }
        if (static_cast<u32>(this->m_dwell) < 0x3e9) {
            return 1;
        }
        CGameObject* base =
            g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + this->m_arrivalCell.m_x]->m_object;
        TileSwitch(base->m_screenX >> 5, base->m_screenY >> 5, 0, this->m_arrivalFlags, 1, 0);
    } else {
        CGrunt* g = m_tileMgr->FindNearestEnemy(this);
        bool atTarget = false;
        if (g != 0) {
            i32 x = g->m_object->m_screenX;
            if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y

                && RectContains(x, g->m_object->m_screenY) != 0) {
                atTarget = true;
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
            if (this->m_stamina < 100) {
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
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
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
        if (g == 0 || g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
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
                g->m_object->m_screenX >> 5,
                g->m_object->m_screenY >> 5,
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

// @early-stop
RVA(0x000f8240, 0x5b9)
i32 CGrunt::StepArrivalDefenseLean() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0) {
        return 1;
    }
    CGrunt* occ;
    switch (m_defenderState) {
        case 2:
            if (m_poweredUp == 0) {
                m_defenderState = 1;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (occ == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto c2_occcheck;
            }
            if (occ->m_entranceCommitted == 0) {
                goto c2_occcheck;
            }
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
                goto c2_miss;
            }
            if (occ->m_object->m_screenX != occ->m_lastTilePx.m_x) {
                goto c2_miss;
            }
            if (occ->m_object->m_screenY != occ->m_lastTilePx.m_y) {
                goto c2_miss;
            }
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePx.m_x,
                occ->m_lastTilePx.m_y
            );
            return 1;
        c2_miss: {
            CWwdGameObjectA* h = m_object;
            i32 vx = h->m_screenX;
            i32 vy = h->m_screenY;
            const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
            if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
            }
        }
            m_defenderState = 1;
            m_dwell = 0x1f4;
            return 1;
        c2_occcheck:
            if (occ == 0) {
                m_defenderState = 0;
                return 1;
            }
            m_defenderState = 1;
            m_dwell = 0x1f4;
            {
                CWwdGameObjectA* h = m_object;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case 1: {
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != 0 && g != occ) {
                m_arrivalCell.m_x = -1;
                m_defenderState = 0;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (occ == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (occ->m_entranceCommitted == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 0x1f4) {
                StepArrivalDrop(
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                );
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
                return 1;
            }
            if (occ->m_object->m_screenX != occ->m_lastTilePx.m_x) {
                return 1;
            }
            if (occ->m_object->m_screenY != occ->m_lastTilePx.m_y) {
                return 1;
            }
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePx.m_x,
                occ->m_lastTilePx.m_y
            );
            m_defenderState = 2;
            return 1;
        }

        case 0:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (rand() % 0x64 == 0 && m_health > 0x1a && occ != 0 && m_stamina >= 0x64
                && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
                m_tileMgr->ApplyTriggerA(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    m_lastTilePx.m_x,
                    m_lastTilePx.m_y
                );
                return 1;
            }
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_hasExtent == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= 0xbb8) {
                return 1;
            }
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_arrivalReroll64
                >= m_arrivalRerollWindow64) {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
                m_dwell = 0;
                return 1;
            }
            {
                CWwdGameObjectA* h = m_object;
                i32 baseX = h->m_extent.left;
                i32 spanX = abs(h->m_extent.right - baseX);
                i32 baseY = h->m_extent.top;
                i32 spanY = abs(h->m_extent.bottom - baseY);
                i32 outX = baseX;
                if (spanX != 0) {
                    outX += rand() % spanX;
                }
                i32 outY = baseY;
                if (spanY != 0) {
                    outY += rand() % spanY;
                }
                CMapMgr* bd = g_gameReg->m_tileGrid;
                if (static_cast<u32>(outX) < static_cast<u32>(bd->m_width)
                    && static_cast<u32>(outY) < static_cast<u32>(bd->m_height)) {
                    TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
            }
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
}
