// GruntArrivalScan.cpp - three per-grunt-type arrival/scan update steps that sit just
// after CGrunt_0ef6b0 in retail (0xecc90 + 0xf0e20 + 0xf36a0). All __thiscall, no
// explicit args, return 1. Direct siblings of GruntUpdateStep.cpp's UpdateArrival
// (0xf0130) / SeekTarget (0xf71c0) and GruntArrivalStep.cpp's StepArrivalDefenseAlt
// (0xf1c70): the head gates on the grunt's type name (g_typeColl.Lookup(...)->m_0) -
// each returns 1 immediately when the grunt IS an "I" - and the body drives the
// tile-to-tile move: resolve the grunt under the HUD center (m_tileMgr->GetOccupant),
// gate on the powered-up / arrival state words, recompute the board dirty rect, scan a
// box of grid cells (or the tile-mgr live-grunt list) for the nearest live target and
// CommitTileSlot2 / TileSwitch6 to it, and on commit fire the on-screen cue (CueA).
//
// FOLDED onto the canonical CGrunt (<Gruntz/Grunt.h>): the local CGruntStep this-alias
// + CGruntTileMgr(FindGrunt/Scatter/m_4)/CGruntCueSink(PlayCue) views are gone. Real
// CGrunt methods; the tile-mgr is the canonical CGruntTileMgr (GetOccupant == the old
// FindGrunt, CommitTileSlot2 == Scatter, m_4 == the live-grunt list); the cue is
// m_cueSink->CueA. The grid/coord node views are the shared CScanGrid family. The CGrunt
// field bag stays raw F()/P() offset (codegen-neutral naming, load-bearing offsets).
#include <Gruntz/Grunt.h> // canonical CGrunt / CGruntTileMgr / CGruntCueSink / CGameRegistry

#include <rva.h>
#include <string.h> // inline strcmp of the grunt type name
#include <Gruntz/StepList2.h>
#include <Gruntz/ScanRectInit.h>
#include <Gruntz/ScanGrid.h>
#include <Gruntz/TypeColl.h>

#pragma intrinsic(strcmp)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))
#define IABS(v) ((v) = ((v) ^ ((v) >> 31)) - ((v) >> 31)) // MSVC cdq/xor/sub abs

// Type-name collection (g_typeColl @0x6bf650): Lookup(key)->node, node->m_0 = name.
extern CTypeColl g_typeColl; // (DATA-bound in GruntUpdateStep.cpp)

// The owned pending-coord recycle pool (g_dropList @0x645540; DATA-bound elsewhere).
extern CStepList2 g_dropList;

// The board/cell targetable gate (0xf0db0, __cdecl reloc-masked): the active-move
// cell predicate used by ArrivalScanB.
extern "C" i32 CellTargetable(i32 col, i32 row); // 0x40107d -> 0xf0db0

// The shared game-manager singleton (*0x64556c); reached typed as CGameRegistry: the
// board base via m_world->m_24->+0x5c, the +0x60 cue receiver (m_cueSink), and the
// +0x70 tile grid downcast to the richer CScanGrid view (dirty rect + row table).
extern CGameRegistry* g_pGameRegistry; // ?g_gameReg@@3PAUWwdGameReg@@A (0x64556c)

// Recompute the board dirty rect (m_60) as {0,0,w,h} intersected with a copy of
// itself; m_70/m_74 = the resulting size.
#define GRID_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        ((CScanRectInit*)&ra)->Set34a4(0, 0, (grid)->m_c, (grid)->m_10);                           \
        RECT* pb = ((CScanRectInit*)&rb)->Set34a4(0, 0, (grid)->m_c, (grid)->m_10);                \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect(&(grid)->m_60, &ra, &rb)) {                                             \
            (grid)->m_60 = ra;                                                                     \
        }                                                                                          \
        (grid)->m_70 = (grid)->m_60.right - (grid)->m_60.left;                                     \
        (grid)->m_74 = (grid)->m_60.bottom - (grid)->m_60.top;                                     \
    }

// Same, but built with inline rect field stores (no Set34a4) - the box-scan exit form.
#define GRID_RECT_INLINE(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        ra.left = 0;                                                                               \
        ra.top = 0;                                                                                \
        ra.right = (grid)->m_c;                                                                    \
        ra.bottom = (grid)->m_10;                                                                  \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_c;                                                                    \
        rb.bottom = (grid)->m_10;                                                                  \
        if (!IntersectRect(&(grid)->m_60, &ra, &rb)) {                                             \
            (grid)->m_60 = ra;                                                                     \
        }                                                                                          \
        (grid)->m_70 = (grid)->m_60.right - (grid)->m_60.left;                                     \
        (grid)->m_74 = (grid)->m_60.bottom - (grid)->m_60.top;                                     \
    }

// Drain the pending-coord list (recycle each node's link, then RemoveAll).
#define DRAIN_COORDS()                                                                             \
    if (F(this, 0x328) != 0) {                                                                     \
        CScanListNode* n = (CScanListNode*)P(this, 0x320);                                         \
        while (n != 0) {                                                                           \
            CScanListNode* cur = n;                                                                \
            n = cur->m_next;                                                                       \
            if (cur->m_8 != 0) {                                                                   \
                g_dropList.Drop(cur->m_8);                                                         \
            }                                                                                      \
        }                                                                                          \
        m_31c.RemoveAll();                                                                         \
    }

// ===========================================================================
// @early-stop
// Deep per-tick arrival/scan step (non-"I" grunt types). Logic reconstructed fully;
// same deep-regalloc + slot-recycle + cold-block wall family as GruntUpdateStep.cpp's
// UpdateArrival/SeekTarget and GruntArrivalStep.cpp's StepArrivalDefenseAlt.
// Final-sweep candidate.
RVA(0x000ecc90, 0x86a)
i32 CGrunt::ArrivalScanA() {
    if (strcmp(g_typeColl.Lookup((i32)m_14->m_1c)->m_0, "I") == 0) {
        return 1;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    CScanGrid* grid = (CScanGrid*)g_pGameRegistry->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    GetScreenPos((GruntTilePos*)c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    GetScreenPos((GruntTilePos*)c2);
    i32 cy = c2[1] >> 5;

    CGrunt* g = m_tileMgr->GetOccupant(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && RectContains(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (g == 0) {
        F(this, 0x390) = 0;
        goto L_ed006;
    }
    if (F(this, 0x21c) != 0) {
        return 1;
    }
    if (F(this, 0x218) == 0 && F(this, 0x3f0) >= 100) {
        if (atTarget) {
            CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
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
    if (g == 0 || (u32)F(this, 0x2ec) <= 0x1f4 || GruntInRadius(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
        F(this, 0x390) = 0;
        goto L_ed153;
    }
    if (F(this, 0x220) != 0) {
        goto L_ed153;
    }
    if (F(this, 0x3f0) >= 100 && F(P(g, 0x10), 0x5c) == F(g, 0x17c)
        && F(P(g, 0x10), 0x60) == F(g, 0x180)
        && RectContains(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
        CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
        F(this, 0x2ec) = 0;
        return 1;
    }
    if (F(this, 0x220) != 0) {
        goto L_ed153;
    }
    if (TileSwitch6(F(P(g, 0x10), 0x5c) >> 5, F(P(g, 0x10), 0x60) >> 5, 0, F(this, 0x248), 1, 0)
        == 0) {
        goto L_ed153;
    }
    if (F(this, 0x390) != 0) {
        char* board = P(g_pGameRegistry->m_world->m_24, 0x5c) + 0x40;
        i32 x = F(P(this, 0x10), 0x5c);
        i32 y = F(P(this, 0x10), 0x60);
        if (x < F(board, 8) && F(board, 0) <= x && y < F(board, 0xc) && F(board, 4) <= y) {
            g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
        }
        F(this, 0x390) = 0;
    }
    F(this, 0x2ec) = 0;

L_ed153:
    if (F(this, 0x328) != 0) {
        CScanCoord* coord = ((CScanNode324*)P(this, 0x320))->m_8;
        i32 col = coord->x;
        i32 row = coord->y;
        CScanCell* cell = &grid->m_8[row][col];
        if ((cell->m_flags & 0x8000) != 0 || cell->m_type == 0x97 || cell->m_type == 0x98) {
            m_tileMgr->CommitTileSlot2(
                F(this, 0x1ec),
                F(this, 0x1f0),
                (col << 5) + 0x10,
                (row << 5) + 0x10
            );
            SetEntrancePos(1, 1);
            F(this, 0x2ec) = 0;
        }
        return 1;
    }
    if ((u32)F(this, 0x2ec) <= 0x3e8) {
        return 1;
    }

    i32 r = F(this, 0x2dc);
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_c;
    gb.bottom = grid->m_10;
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
        gb2.right = grid->m_c;
        gb2.bottom = grid->m_10;
        if (!IntersectRect(&grid->m_60, &lb, &gb2)) {
            grid->m_60 = lb;
        }
        grid->m_70 = grid->m_60.right - grid->m_60.left;
        grid->m_74 = grid->m_60.bottom - grid->m_60.top;
    }
    for (i32 row = isect.top; row < isect.bottom; row++) {
        CScanCell* cell = &grid->m_8[row][isect.left];
        for (i32 col = isect.left; col < isect.right; col++) {
            if ((cell->m_flags & 0x8000) != 0 || cell->m_type == 0x97 || cell->m_type == 0x98) {
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
            cell = (CScanCell*)((char*)cell + 0x1c);
        }
    }
    if (best != 0x7fffffff) {
        i32 dc = bestCol - cx;
        IABS(dc);
        i32 dr = bestRow - cy;
        IABS(dr);
        if (dc <= 1 && dr <= 1) {
            m_tileMgr->CommitTileSlot2(
                F(this, 0x1ec),
                F(this, 0x1f0),
                (bestCol << 5) + 0x10,
                (bestRow << 5) + 0x10
            );
            SetEntrancePos(1, 1);
        } else {
            TileSwitch6(bestCol, bestRow, 0, F(this, 0x248), 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    F(this, 0x2ec) = 0;
    return 1;
}

// ===========================================================================
// @early-stop
// Sibling of ArrivalScanA with a __cdecl board test (CellTargetable 0xf0db0) instead
// of the inlined point-in-rect, and a live-grunt-LIST scan (m_tileMgr->m_4, PtInRect
// membership) instead of the grid-cell box scan. Logic reconstructed fully; same
// deep-regalloc + slot-recycle wall family. Final-sweep candidate.
RVA(0x000f0e20, 0x928)
i32 CGrunt::ArrivalScanB() {
    if (strcmp(g_typeColl.Lookup((i32)m_14->m_1c)->m_0, "I") == 0) {
        return 1;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    CScanGrid* grid = (CScanGrid*)g_pGameRegistry->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    GetScreenPos((GruntTilePos*)c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    GetScreenPos((GruntTilePos*)c2);
    i32 cy = c2[1] >> 5;

    CGrunt* g = m_tileMgr->GetOccupant(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && RectContains(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (g == 0) {
        F(this, 0x390) = 0;
        goto L_ed006b;
    }
    if (F(this, 0x21c) != 0) {
        return 1;
    }
    if (F(this, 0x218) == 0 && F(this, 0x3f0) >= 100) {
        if (atTarget) {
            CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
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
    if (g == 0 || GruntInRadius(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
        F(this, 0x390) = 0;
        goto L_scanb;
    }
    if (F(this, 0x220) != 0) {
        goto L_scanb;
    }
    if (F(this, 0x3f0) >= 100 && F(P(g, 0x10), 0x5c) == F(g, 0x17c)
        && F(P(g, 0x10), 0x60) == F(g, 0x180)
        && RectContains(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
        CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
    }
    if (F(this, 0x220) != 0) {
        goto L_scanb;
    }
    if ((u32)F(this, 0x2ec) <= 0x1f4) {
        goto L_scanb;
    }
    {
        i32 cc[4];
        g->GetScreenPos((GruntTilePos*)cc);
        if (TileSwitch6(cc[0] >> 5, cc[1] >> 5, 0, F(this, 0x248), 1, 0) != 0) {
            if (F(this, 0x390) != 0) {
                i32 x = F(P(this, 0x10), 0x5c);
                i32 y = F(P(this, 0x10), 0x60);
                if (GruntPointVisible((i32)(P(g_pGameRegistry->m_world->m_24, 0x5c) + 0x40), x, y)
                    != 0) {
                    g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                }
                F(this, 0x390) = 0;
            }
            F(this, 0x2ec) = 0;
        }
    }

L_scanb:
    if (F(this, 0x328) != 0) {
        CScanCoord* coord = ((CScanNode324*)P(this, 0x320))->m_8;
        i32 col = coord->x;
        i32 row = coord->y;
        if (CellTargetable(col, row) != 0) {
            m_tileMgr->CommitTileSlot2(
                F(this, 0x1ec),
                F(this, 0x1f0),
                (col << 5) + 0x10,
                (row << 5) + 0x10
            );
            SetEntrancePos(1, 1);
            F(this, 0x2ec) = 0;
        }
        return 1;
    }
    if ((u32)F(this, 0x2ec) <= 0x3e8) {
        return 1;
    }

    i32 r = F(this, 0x2dc);
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_c;
    gb.bottom = grid->m_10;
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
        gb2.right = grid->m_c;
        gb2.bottom = grid->m_10;
        if (!IntersectRect(&grid->m_60, &lb, &gb2)) {
            grid->m_60 = lb;
        }
        grid->m_70 = grid->m_60.right - grid->m_60.left;
        grid->m_74 = grid->m_60.bottom - grid->m_60.top;
    }

    i32 best = 0x7fffffff;
    i32 bestX = 0;
    i32 bestY = 0;
    CGruntLiveNode* node = m_tileMgr->m_4;
    while (node != 0) {
        char* gg = node->m_entry;
        node = node->m_next;
        if (F(gg, 0x5c) == 0) {
            i32 gx = F(gg, 0x54);
            i32 gy = F(gg, 0x58);
            if (RectContains((gx << 5) + 0x10, (gy << 5) + 0x10) != 0) {
                m_tileMgr->CommitTileSlot2(
                    F(this, 0x1ec),
                    F(this, 0x1f0),
                    (gx << 5) + 0x10,
                    (gy << 5) + 0x10
                );
                GRID_RECT_BOUNDS(grid);
                return 1;
            }
            i32 dx = gx - (F(P(this, 0x10), 0x5c) >> 5);
            IABS(dx);
            i32 dy = gy - (F(P(this, 0x10), 0x60) >> 5);
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
            m_tileMgr->CommitTileSlot2(
                F(this, 0x1ec),
                F(this, 0x1f0),
                (bestX << 5) + 0x10,
                (bestY << 5) + 0x10
            );
            SetEntrancePos(1, 1);
        } else {
            TileSwitch6(bestX, bestY, 0, F(this, 0x248), 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    F(this, 0x2ec) = 0;
    return 1;
}

// ===========================================================================
// @early-stop
// Sibling of ArrivalScanA: stamps m_300/m_304 AFTER the atTarget probe, duplicates
// the powered-up reset per branch, gates the box scan on m_220==0 too, and matches
// grid cells by flag 0x10000 (active-move by 0x40|0x10000). Logic reconstructed fully;
// same deep-regalloc + slot-recycle wall family. Final-sweep candidate.
RVA(0x000f36a0, 0x78e)
i32 CGrunt::ArrivalScanC() {
    if (strcmp(g_typeColl.Lookup((i32)m_14->m_1c)->m_0, "I") == 0) {
        return 1;
    }
    CScanGrid* grid = (CScanGrid*)g_pGameRegistry->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    GetScreenPos((GruntTilePos*)c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    GetScreenPos((GruntTilePos*)c2);
    i32 cy = c2[1] >> 5;

    CGrunt* g = m_tileMgr->GetOccupant(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && RectContains(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
            if (F(this, 0x21c) != 0) {
                return 1;
            }
            F(this, 0x1e4) = 0;
            F(this, 0x218) = 0;
            F(this, 0x21c) = 0;
            F(this, 0x220) = 0;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        if (atTarget) {
            return 1;
        }
        if (F(this, 0x220) == 0) {
            return 1;
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    if (g == 0 || GruntInRadius(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
        F(this, 0x390) = 0;
        goto L_tailc;
    }
    if (F(this, 0x220) != 0) {
        goto L_tailc;
    }
    if (F(this, 0x3f0) >= 100 && F(P(g, 0x10), 0x5c) == F(g, 0x17c)
        && F(P(g, 0x10), 0x60) == F(g, 0x180)
        && RectContains(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
        CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
        F(this, 0x2ec) = 0;
        return 1;
    }
    if (F(this, 0x220) != 0) {
        goto L_tailc;
    }
    if ((u32)F(this, 0x2ec) <= 0x1f4) {
        goto L_tailc;
    }
    if (TileSwitch6(F(P(g, 0x10), 0x5c) >> 5, F(P(g, 0x10), 0x60) >> 5, 0, F(this, 0x248), 1, 0)
        != 0) {
        if (F(this, 0x390) != 0) {
            char* board = P(g_pGameRegistry->m_world->m_24, 0x5c) + 0x40;
            i32 x = F(P(this, 0x10), 0x5c);
            i32 y = F(P(this, 0x10), 0x60);
            if (x < F(board, 8) && F(board, 0) <= x && y < F(board, 0xc) && F(board, 4) <= y) {
                g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
            }
            F(this, 0x390) = 0;
        }
        F(this, 0x2ec) = 0;
    }

L_tailc:
    if (F(this, 0x328) != 0) {
        CScanCoord* coord = ((CScanNode324*)P(this, 0x320))->m_8;
        i32 col = coord->x;
        i32 row = coord->y;
        CScanCell* cell = &grid->m_8[row][col];
        if ((cell->m_flags & 0x40) != 0 || (cell->m_flags & 0x10000) != 0) {
            m_tileMgr->CommitTileSlot2(
                F(this, 0x1ec),
                F(this, 0x1f0),
                (col << 5) + 0x10,
                (row << 5) + 0x10
            );
            SetEntrancePos(1, 1);
            F(this, 0x2ec) = 0;
        }
        return 1;
    }
    if (F(this, 0x220) == 0 && (u32)F(this, 0x2ec) > 0x3e8) {
        i32 r = F(this, 0x2dc);
        RECT box;
        box.left = cx - r;
        box.right = cx + r;
        box.top = cy - r;
        box.bottom = cy + r;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_c;
        gb.bottom = grid->m_10;
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
            gb2.right = grid->m_c;
            gb2.bottom = grid->m_10;
            if (!IntersectRect(&grid->m_60, &lb, &gb2)) {
                grid->m_60 = lb;
            }
            grid->m_70 = grid->m_60.right - grid->m_60.left;
            grid->m_74 = grid->m_60.bottom - grid->m_60.top;
        }
        i32 best = 0x7fffffff;
        i32 bestCol = -1;
        i32 bestRow = -1;
        for (i32 row = isect.top; row < isect.bottom; row++) {
            CScanCell* cell = &grid->m_8[row][isect.left];
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
                cell = (CScanCell*)((char*)cell + 0x1c);
            }
        }
        if (best != 0x7fffffff) {
            i32 dc = bestCol - cx;
            IABS(dc);
            i32 dr = bestRow - cy;
            IABS(dr);
            if (dc <= 1 && dr <= 1) {
                m_tileMgr->CommitTileSlot2(
                    F(this, 0x1ec),
                    F(this, 0x1f0),
                    (bestCol << 5) + 0x10,
                    (bestRow << 5) + 0x10
                );
                SetEntrancePos(1, 1);
            } else {
                TileSwitch6(bestCol, bestRow, 0, F(this, 0x248), 1, 0);
            }
        }
        GRID_RECT_INLINE(grid);
        F(this, 0x2ec) = 0;
    }
    return 1;
}
