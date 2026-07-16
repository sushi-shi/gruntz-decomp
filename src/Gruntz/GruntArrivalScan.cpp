// GruntArrivalScan.cpp - the CGrunt arrival/scan/step obj (retail Grunt.obj arrival
// band, RVA 0xec670-0xf8240). MERGED (operation REHOME M2) from the over-split
// GruntArrivalScan / WanderIdleStep / GruntReticle / GruntUpdateStep / GruntArrivalStep
// / GruntPhaseStep files: they interleave in retail RVA order (ArrivalScanA...B...C
// with WanderStep/Reticle/UpdateArrival/DefenseAlt/PhaseStep/SeekTarget between), so
// they are pieces of ONE retail obj. All folded onto the canonical CGrunt
// (<Gruntz/Grunt.h>), CGameRegistry singleton facet. Placeholder field names; only
// offsets + code bytes are load-bearing. The retail obj ALSO contains the interleaved
// helper-class bodies (Grunt::ChargeStep, CGruntScan::ScanNearestTarget, CObjectTracker,
// the MgrListFind free fn) + CGrunt's four Grunt.cpp arrival fns (ResolveArrivalReposition,
// ResolveArrivalNeighbor, StepArrivalDefense, StepArrivalDefenseLean) - those stay in
// their own files pending the WwdGameReg/CGameRegistry singleton dual-view reconciliation
// (see the M2 report); they interleave this unit's span until then.
#include <Mfc.h> // afx-first (Reticle's /GX EH frame builds a local CByteArray; RECT/IntersectRect)
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/Grunt.h>      // canonical CGrunt / CGruntCueSink / CGameRegistry
#include <Gruntz/TriggerMgr.h>  // the ONE CTriggerMgr
#include <Gruntz/GruntPuddle.h> // CGruntPuddle (the live-candidate list element)
#include <Gruntz/GameLevel.h>  // canonical CGameLevel (m_world->m_level) + CLevelPlane visible rect
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h> // inline strcmp of the grunt type name
#include <stdlib.h> // abs (branchless cdq/xor/sub)
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/ScanRectInit.h>
#include <Gruntz/ScanGrid.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <rva.h>

#pragma intrinsic(strcmp)

#define IABS(v) ((v) = ((v) ^ ((v) >> 31)) - ((v) >> 31)) // MSVC cdq/xor/sub abs

// The shared game-manager singleton (*0x64556c); reached typed as CGameRegistry.

// Anim-name registry (g_typeColl @0x6bf650, RTTI ?g_typeColl@@3VCTypeKeyColl@@A):
// GetNameRecord(m_14->m_1c) -> char**; *rec = the grunt-type name char* (retail call
// 0x437c -> zDArray::IndexToPtr @0x310f0). Canonical CTypeKeyColl model, no casts.

// g_clock was a SECOND NAME for g_frameTime (0x245588 frame clock) - same address,
// so nothing ever defined it. Unified onto the canonical.
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) are
// fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].

// The former `g_dropList` was a SECOND name for 0x245540 - the same FreeNodePool the
// rest of the tree calls g_coordPool (<Gruntz/Grunt.h>); its DATA pin here was the one
// binding at
// that rva, which left every `?g_coordPool@@3VFreeNodePool@@A` reference UNBOUND. Folded
// onto g_coordPool (defined in GameText.cpp, the pool's reset/clear owner TU).

extern "C" i32 CellTargetable(i32 col, i32 row); // 0x40107d -> 0xf0db0 (MgrListFind)

extern "C" {
    i32 GameRand(); // 0x51fee0 (__cdecl)
}

// (CGruntPtAcc is GONE: PhaseStep's /GX-forcing point accumulator IS MFC ::CDWordArray.
//  PROVEN from the binary - its ctor 0x1b4b43 stamps vtable 0x1ec29c, whose MFC
//  CRuntimeClass names it "CDWordArray".  The old note "Dtor @0x1b4b76 IS
//  CByteArray::~CByteArray" was a FID AMBIG mislabel: 0x1b4b76 is inside the CDWordArray
//  band [0x1b4b43, 0x1b4f0b), not CByteArray's [0x1b527e, 0x1b55e9).)

// Recompute the grid dirty rect (m_60) as {0,0,w,h} intersected with a copy, then
// m_70/m_74 = the resulting size (the shared GruntTileScan dirty-rect idiom).
#define GRID_BOUNDS(grid)                                                                          \
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

// Recycle the visited-coord CPtrList nodes (head) back onto the shared free list.
#define RECYCLE_COORDS(head)                                                                       \
    {                                                                                              \
        GruntCoordNode* n = (head);                                                                \
        while (n != 0) {                                                                           \
            GruntCoordNode* next = n->m_next;                                                      \
            void* pay = n->m_coord;                                                                \
            if (pay != 0) {                                                                        \
                void** slot = (void**)((char*)pay - g_coordPool.m_linkOffset);                     \
                *slot = g_coordPool.m_freeHead;                                                    \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
            n = next;                                                                              \
        }                                                                                          \
    }

// Recompute the board dirty rect (m_60) as {0,0,w,h} intersected with a copy of itself.
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
    if (CoordCount() != 0) {                                                                       \
        CScanListNode* n = (CScanListNode*)m_31c.GetHeadPosition();                                \
        while (n != 0) {                                                                           \
            CScanListNode* cur = n;                                                                \
            n = cur->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push(cur->m_coord);                                                    \
            }                                                                                      \
        }                                                                                          \
        m_31c.RemoveAll();                                                                         \
    }


// ---------------------------------------------------------------------------
// CGrunt::ResolveArrivalReposition()   @0xec670   (__thiscall, ret 0 -> 1)
// The per-tick arrival-reposition step. Latch the defender position to the last
// tile; if the tile occupant is in radius and the +0x2ec dwell exceeds 0xfa, try to
// tile-switch onto it and commit its slot - on a -1 commit (slot not yet free) fire
// the on-screen entrance cue (0x366) when in view. With no in-radius occupant, once
// the +0x2ec dwell window passes the thresholds and the (m_arrivalRerollLo..m_arrivalRerollWindowHi) idle timer has
// elapsed, reset that timer to a fresh rand%0x7530; otherwise re-roll a random target
// inside the HUD scroll region (m_134..m_140) and tile-switch onto it, escalating to
// SetEntrancePos when the spread exceeds CoordCount(). Returns 1.
// @early-stop
// idiv/rand + abs + shared-tail-zero-reg + 64-bit-sbb-timer plateau: the occupant
// resolve, the in-radius/dwell gates, the random-region re-roll (two rand()%span via
// abs spans), the max-spread SetEntrancePos escalation, the idle-timer reset, and the
// structure-1 on-screen cue are all reconstructed in shape/order. Residue is the
// MSVC /O2 idiv scheduling + the ebx zero-register tail sharing. Final sweep.
RVA(0x000ec670, 0x298)
i32 CGrunt::ResolveArrivalReposition() {
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    if (occ != 0 && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
        if (static_cast<u32>(m_dwell) > 0xfa) {
            CGameObject* oh = occ->m_10;
            if (TileSwitch(oh->m_screenX >> 5, oh->m_screenY >> 5, 0, m_arrivalFlags, 1, 0) != 0) {
                CGameObject* oh2 = occ->m_10;
                if (m_tileMgr->ApplyTriggerA(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        oh2->m_screenX,
                        oh2->m_screenY
                    )
                    == -1) {
                    m_dwell = 0;
                    if (m_390 != 0) {
                        CGameObject* h = m_10;
                        i32 vx = h->m_screenX;
                        i32 vy = h->m_screenY;
                        char* sc = *(char**)((char*)g_gameReg->m_world + 0x24);
                        i32* rect = (i32*)(*(char**)(sc + 0x5c) + 0x40);
                        if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                            g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                        }
                        m_390 = 0;
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
        u32 dwell = *(u32*)&m_dwell;
        if (dwell > 0x3e8 && m_resetApplied == 0 && m_318 != 0 && dwell > 0xbb8) {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_arrivalRerollLo
                >= *(i64*)&m_arrivalRerollWindowLo) {
                goto L8b5;
            }
            CGameObject* h = m_10;
            i32 baseX = h->m_extentL;
            i32 spanX = abs(h->m_extentR - baseX);
            i32 baseY = h->m_extentT;
            i32 spanY = abs(h->m_extentB - baseY);
            i32 outX = baseX;
            if (spanX != 0) {
                outX += GruntRand() % spanX;
            }
            i32 outY = baseY;
            if (spanY != 0) {
                outY += GruntRand() % spanY;
            }
            TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
            i32 m328 = CoordCount();
            if (m328 != 0) {
                i32 mx = spanX > spanY ? spanX : spanY;
                if (m328 > mx) {
                    SetEntrancePos(1, 1);
                }
            }
            m_390 = 1;
            goto L8a2;
        }
    }
    return 1;

L8a2:
    m_dwell = 0;
    return 1;

L8b5:
    ResetEntranceAnimation(1, 1, 0);
    m_arrivalRerollLo = 0;
    m_arrivalRerollWindowLo = 0;
    m_arrivalRerollHi = 0;
    m_arrivalRerollWindowHi = 0;
    m_arrivalRerollWindowLo = GruntRand() % 0x7530 + 0x7530;
    m_arrivalRerollWindowHi = 0;
    m_arrivalRerollLo = static_cast<i32>(g_frameTime);
    m_arrivalRerollHi = 0;
    m_390 = 1;
    goto L8a2;
}

// ===========================================================================
// @early-stop
// Deep per-tick arrival/scan step (non-"I" grunt types). Logic reconstructed fully;
// same deep-regalloc + slot-recycle + cold-block wall family as GruntUpdateStep.cpp's
// UpdateArrival/SeekTarget and GruntArrivalStep.cpp's StepArrivalDefenseAlt.
// Final-sweep candidate.
RVA(0x000ecc90, 0x86a)
i32 CGrunt::ArrivalScanA() {
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), "I") == 0) {
        return 1;
    }
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    GetScreenPos((GruntTilePos*)c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    GetScreenPos((GruntTilePos*)c2);
    i32 cy = c2[1] >> 5;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = g->m_10->m_screenX;
        if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
            && RectContains(x, g->m_10->m_screenY) != 0) {
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
        m_390 = 0;
        goto L_ed006;
    }
    if (m_neighborValid != 0) {
        return 1;
    }
    if (m_combatActive == 0 && m_stamina >= 100) {
        if (atTarget) {
            CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, g->m_lastTilePxX, g->m_lastTilePxY);
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
    if (g == 0 || static_cast<u32>(m_dwell) <= 0x1f4 || GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
        m_390 = 0;
        goto L_ed153;
    }
    if (m_poweredUp != 0) {
        goto L_ed153;
    }
    if (m_stamina >= 100 && g->m_10->m_screenX == g->m_lastTilePxX
        && g->m_10->m_screenY == g->m_lastTilePxY
        && RectContains(g->m_10->m_screenX, g->m_10->m_screenY) != 0) {
        CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, g->m_lastTilePxX, g->m_lastTilePxY);
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != 0) {
        goto L_ed153;
    }
    if (TileSwitch(g->m_10->m_screenX >> 5, g->m_10->m_screenY >> 5, 0, m_arrivalFlags, 1, 0)
        == 0) {
        goto L_ed153;
    }
    if (m_390 != 0) {
        CCueRect* board = (CCueRect*)&g_gameReg->m_world->m_level->m_mainPlane->m_originX;
        i32 x = m_10->m_screenX;
        i32 y = m_10->m_screenY;
        if (x < board->right && board->left <= x && y < board->bottom && board->top <= y) {
            g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
        }
        m_390 = 0;
    }
    m_dwell = 0;

L_ed153:
    if (CoordCount() != 0) {
        CScanCoord* coord = ((CScanListNode*)m_31c.GetHeadPosition())->m_coord;
        i32 col = coord->x;
        i32 row = coord->y;
        CScanCell* cell = &grid->m_8[row][col];
        if ((cell->m_flags & 0x8000) != 0 || cell->m_type == 0x97 || cell->m_type == 0x98) {
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

// ===========================================================================
// @early-stop
// Deep per-tick idle/wander AI step: ~18 reloc-masked engine calls (mixed __thiscall
// receivers on this/other-grunt + the on-screen cue + __cdecl frees), the manager-grid
// chain, the free-list splice/drop, a 6-way phase jump table and a PtInRect box-clip.
// Logic reconstructed in full from the disasm. Banked at ~74% fuzzy (stub was 28.6%).
// Two propagating codegen walls, same family as the GruntUpdateStep siblings:
//   (1) frame-size shift: cl reserves `sub esp,0x14`, retail `sub esp,0x18` (retail keeps
//       one extra 4-byte local slot for the CommitNeighbor m_17c/m_180 spill), which
//       offsets every [esp+N] displacement + the epilogue `add esp,N`;
//   (2) prologue zero-register colouring swap: retail zeroes ebx (the CSE'd 0 used by all
//       the `cmp field,0` / `mov field,0`) before ebp (the atTarget flag); cl picks the
//       opposite, so every `cmp X,ebx` <-> `cmp X,ebp` flips downstream.
// Both are cl register-allocator / stack-slot tie-breaks structured C++ cannot force.
// Final-sweep candidate.
RVA(0x000ed9f0, 0x8dd)
i32 CGrunt::WanderStep() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;

    i32 flag = 0;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    if (g != 0) {
        i32 gx = g->m_10->m_screenX;
        if (gx == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
            && RectContains(gx, g->m_10->m_screenY) != 0) {
            flag = 1;
        }
    }

    // Powered-up / arrival gate: never returns except through FindGridNeighbor;
    // otherwise it forces the phase to 5 and falls into the switch.
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
                if (m_poweredUp == 0 && m_stamina >= 0x64 && g->m_10->m_screenX == g->m_lastTilePxX
                    && g->m_10->m_screenY == g->m_lastTilePxY
                    && RectContains(g->m_10->m_screenX, g->m_10->m_screenY) != 0) {
                    CommitNeighbor(
                        g->m_tileOwnerHi,
                        g->m_tileOwnerLo,
                        g->m_lastTilePxX,
                        g->m_lastTilePxY
                    );
                    m_358 = 0;
                    if (CoordCount() != 0) {
                        void* node = m_31c.GetHeadPosition();
                        if (node != 0) {
                            do {
                                void* cur = node;
                                node = *(void**)node;
                                i32 data = *(i32*)((char*)cur + 8);
                                if (data != 0) {
                                    g_coordPool.Push((void*)(data));
                                }
                            } while (node != 0);
                        }
                        m_31c.RemoveAll();
                    }
                    m_defenderState = 5;
                    return 1;
                }
                if (static_cast<u32>(m_dwell) > 0x3e8) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        i32 c[4];
                        g->GetScreenPos((GruntTilePos*)c);
                        if (TileSwitch(c[0] >> 5, c[1] >> 5, 0, m_arrivalFlags, 1, 0) != 0) {
                            SetEntrancePos(1, 1);
                            m_arrivalCol = g->m_tileOwnerHi;
                            m_arrivalRow = g->m_tileOwnerLo;
                            m_defenderState = 1;
                            if (GruntPointVisible(
                                    (i32)&g_gameReg->m_world->m_level->m_mainPlane->m_originX,
                                    m_10->m_screenX,
                                    m_10->m_screenY
                                )
                                != 0) {
                                g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            goto timeout;

        case 1: {
            CGrunt* slot = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            CGrunt* active = m_tileMgr->FindNearestEnemy(this);
            if (active != 0 && active != slot) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
                return 1;
            }
            if (slot == 0 || slot->m_entranceCommitted == 0
                || GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 0x1f4) {
                StepArrivalDrop(slot->m_lastTilePxX, slot->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(slot->m_10->m_screenX, slot->m_10->m_screenY) == 0) {
                return 1;
            }
            if (slot->m_10->m_screenX != slot->m_lastTilePxX) {
                return 1;
            }
            if (slot->m_10->m_screenY != slot->m_lastTilePxY) {
                return 1;
            }
            CommitNeighbor(
                slot->m_tileOwnerHi,
                slot->m_tileOwnerLo,
                slot->m_lastTilePxX,
                slot->m_lastTilePxY
            );
            m_358 = 0;
            if (CoordCount() != 0) {
                void* node = m_31c.GetHeadPosition();
                if (node != 0) {
                    do {
                        void* cur = node;
                        node = *(void**)node;
                        i32 data = *(i32*)((char*)cur + 8);
                        if (data != 0) {
                            g_coordPool.Push((void*)(data));
                        }
                    } while (node != 0);
                }
                m_31c.RemoveAll();
            }
            m_defenderState = 5;
            return 1;
        }

        case 2: {
            if (m_poweredUp == 0) {
                m_defenderState = 0;
                return 1;
            }
            CGrunt* slot = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
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
            if (RectContains(slot->m_10->m_screenX, slot->m_10->m_screenY) == 0) {
                goto ph1;
            }
            if (slot->m_10->m_screenX != slot->m_lastTilePxX) {
                goto ph1;
            }
            if (slot->m_10->m_screenY != slot->m_lastTilePxY) {
                goto ph1;
            }
            CommitNeighbor(
                slot->m_tileOwnerHi,
                slot->m_tileOwnerLo,
                slot->m_lastTilePxX,
                slot->m_lastTilePxY
            );
            m_358 = 0;
            if (CoordCount() != 0) {
                void* node = m_31c.GetHeadPosition();
                if (node != 0) {
                    i32 prev = (i32)g_coordPool.m_freeHead;
                    do {
                        void* cur = node;
                        node = *(void**)node;
                        i32 data = *(i32*)((char*)cur + 8);
                        if (data != 0) {
                            i32* fslot = (i32*)(data - g_coordPool.m_linkOffset);
                            *fslot = prev;
                            prev = (i32)fslot;
                            g_coordPool.m_freeHead = fslot;
                        }
                    } while (node != 0);
                }
                m_31c.RemoveAll();
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
            CGameObject* base = m_10;
            i32 clip = 1;
            i32 py = GameRand() % 4 + (base->m_screenY >> 5) - 2;
            i32 px = GameRand() % 4 + (base->m_screenX >> 5) - 2;
            if (static_cast<u32>(m_arrivalCol) < 4 && static_cast<u32>(m_arrivalRow) < 0xf) {
                CGrunt* entry =
                    g_gameReg->m_cmdGrid->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
                if (entry != 0) {
                    CGameObject* e10 = entry->m_10;
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
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(px) >= static_cast<u32>(grid->m_c)) {
                return 1;
            }
            if (static_cast<u32>(py) >= static_cast<u32>(grid->m_10)) {
                return 1;
            }
            TileSwitch(px, py, 0, m_arrivalFlags, 1, 0);
            return 1;
        }

        default:
            return 1;
    }

timeout:
    if (m_resetApplied == 0 && m_318 != 0 && static_cast<u32>(m_dwell) > 0xbb8) {
        i32 hi = -static_cast<i32>((static_cast<u32>(g_frameTime) < static_cast<u32>(m_arrivalRerollLo))) - m_arrivalRerollHi;
        i32 lo = static_cast<i32>((g_frameTime - static_cast<u32>(m_arrivalRerollLo)));
        if (m_arrivalRerollWindowHi < hi
            || (m_arrivalRerollWindowHi == hi && static_cast<u32>(lo) >= static_cast<u32>(m_arrivalRerollWindowLo))) {
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
            CGameObject* base = m_10;
            u32 lx = static_cast<u32>(base->m_extentL);
            i32 dxr = base->m_extentR - static_cast<i32>(lx);
            i32 ax = (dxr ^ (dxr >> 31)) - (dxr >> 31);
            u32 ly = static_cast<u32>(base->m_extentT);
            i32 dyr = base->m_extentB - static_cast<i32>(ly);
            i32 ay = (dyr ^ (dyr >> 31)) - (dyr >> 31);
            if (ax != 0) {
                lx += GameRand() % ax;
            }
            if (ay != 0) {
                ly += GameRand() % ay;
            }
            if (lx < static_cast<u32>(((GruntBoard*)g_gameReg->m_tileGrid)->m_c)
                && ly < static_cast<u32>(((GruntBoard*)g_gameReg->m_tileGrid)->m_10)) {
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
    i32 defTX = m_defenderX >> 5;
    i32 defTY = m_defenderY >> 5;

    GruntTilePos pt;
    GetScreenPos(&pt);
    i32 dTX = abs((pt.m_x >> 5) - defTX);
    GetScreenPos(&pt);
    i32 dTY = abs((pt.m_y >> 5) - defTY);
    i32 dist = dTX > dTY ? dTX : dTY;
    if (dist > m_defenderRadius) {
        m_defenderX = m_lastTilePxX;
        m_defenderY = m_lastTilePxY;
        return 1;
    }

    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    i32 occOnTile = 0;
    if (occ) {
        CGameObject* oo = occ->m_object;
        if (oo->m_screenX == occ->m_lastTilePxX && oo->m_screenY == occ->m_lastTilePxY) {
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

    // --- m_poweredUp == 0 ---
    if (occ == 0) {
        m_390 = 0;
    } else {
        if (m_neighborValid) {
            return 1;
        }
        if (m_combatActive == 0 && m_stamina >= 0x64 && occOnTile) {
            CommitCombatMove(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePxX,
                occ->m_lastTilePxY
            );
            if (CoordCount()) {
                for (GruntCoordNode* n = CoordHead(); n; n = n->m_next) {
                    if (n->m_coord) {
                        g_coordPool.Push(n->m_coord);
                    }
                }
                m_31c.RemoveAll();
            }
            return 1;
        }
        if (occOnTile) {
            if (CoordCount()) {
                for (GruntCoordNode* n = CoordHead(); n; n = n->m_next) {
                    if (n->m_coord) {
                        g_coordPool.Push(n->m_coord);
                    }
                }
                m_31c.RemoveAll();
            }
            return 1;
        }
    }

    // --- reach-box grid marking tail (DECOMPILER-GATED; see @early-stop above) ---
    // The confident branch structure is retained; the CByteArray snapshot + radius
    // mark loops + the shared IntersectRect/coord-rebuild are deferred.
    return 1;
}

// ===========================================================================
// @early-stop
// 23%->34% (2026-07-05): the phase dispatch is a `switch (m_defenderState)`, not
// if/else - that produced retail's `sub eax; je phase0; dec; je phase1; dec; jne tail;
// [phase2 fall-through]` ladder and the phase2/phase1/phase0 reverse layout, and the
// `goto tail`s became `break`s. Residual walls (final sweep):
//   * phase-2 powered-up recheck (~78 insns, retail 0x1cb-0x24d) is DEAD in-source (the
//     switch runs only with m_220==0, which the top powered-up early-out proves) so THIS
//     cl dead-code-eliminates it while retail emits it - the same DCE artifact as
//     GruntChargeStep's state-2 recheck; no clean C spelling forces the dead block.
//   * `if (!strcmp(name,"I"))` sete-bool vs my `!=0` branch, and the deep regalloc across
//     the grunt-under-HUD pointer / clock / grid bases.
RVA(0x000f0130, 0x7c0)
i32 CGrunt::UpdateArrival() {
    char* name = *g_typeColl.GetNameRecord(m_14->m_1c);
    if (strcmp(name, "I") != 0) {
        return 1;
    }
    this->m_defenderX = this->m_lastTilePxX;
    this->m_defenderY = this->m_lastTilePxY;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    bool atTarget = false;
    if (g != 0) {
        i32 x = g->m_10->m_screenX;
        if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
            && g->RectContains(x, g->m_10->m_screenY) != 0) {
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
                    i32 x = g->m_10->m_screenX;
                    if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
                        && g->RectContains(x, g->m_10->m_screenY) != 0) {
                        CommitNeighbor(
                            g->m_tileOwnerHi,
                            g->m_tileOwnerLo,
                            g->m_lastTilePxX,
                            g->m_lastTilePxY
                        );
                        break;
                    }
                }
                if (g != 0 && static_cast<u32>(this->m_dwell) > 1000) {
                    if (g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        i32 c[4];
                        GetScreenPos((GruntTilePos*)c);
                        if (TileSwitch(c[1] >> 5, c[0] >> 5, 0, this->m_arrivalFlags, 0, 0x20)
                            != 0) {
                            SetEntrancePos(1, 1);
                            this->m_arrivalCol = g->m_tileOwnerHi;
                            this->m_arrivalRow = g->m_tileOwnerLo;
                            this->m_defenderState = 1;
                            i32 r = GruntPointVisible(
                                (i32)&g_gameReg->m_world->m_level->m_mainPlane->m_originX,
                                this->m_10->m_screenX,
                                this->m_10->m_screenY
                            );
                            if (r != 0) {
                                g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    this->m_dwell = 0;
                    break;
                }
            }
            if (this->m_resetApplied == 0 && this->m_318 != 0 && static_cast<u32>(this->m_dwell) > 3000) {
                i32 cmp = -static_cast<i32>((static_cast<u32>(g_frameTime) < static_cast<u32>(this->m_arrivalRerollLo)))
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
                    this->m_arrivalRerollWindowLo = GruntRand() % 30000 + 30000;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                    this->m_arrivalRerollHi = 0;
                } else {
                    CGameObject* base = this->m_10;
                    u32 lo = base->m_extentL;
                    i32 dx = base->m_extentR - static_cast<i32>(lo);
                    i32 ax = (dx ^ (dx >> 31)) - (dx >> 31);
                    u32 lo2 = base->m_extentT;
                    i32 dy = base->m_extentB - static_cast<i32>(lo2);
                    i32 ay = (dy ^ (dy >> 31)) - (dy >> 31);
                    if (ax != 0) {
                        lo = lo + GruntRand() % ax;
                    }
                    if (ay != 0) {
                        lo2 = lo2 + GruntRand() % ay;
                    }
                    if (lo < static_cast<u32>(((GruntBoard*)g_gameReg->m_tileGrid)->m_c)
                        && lo2 < static_cast<u32>(((GruntBoard*)g_gameReg->m_tileGrid)->m_10)) {
                        TileSwitch(static_cast<i32>(lo), static_cast<i32>(lo2), 0, this->m_arrivalFlags, 1, 0);
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
                m_tileMgr->m_grid[this->m_arrivalCol * TM_GRID_COLS + this->m_arrivalRow];
            i32 cur = m_tileMgr->FindNearestEnemy(this) ? 1 : 0;
            CGrunt* found = m_tileMgr->FindNearestEnemy(this);
            (void)cur;
            if (found == 0 || found == slot) {
                if (slot == 0 || slot->m_entranceCommitted == 0
                    || slot->GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
                    this->m_defenderState = 0;
                } else {
                    StepArrivalDrop(
                        slot->m_lastTilePxX,
                        slot->m_lastTilePxY,
                        0,
                        this->m_arrivalFlags,
                        0,
                        0x20
                    );
                    if (this->m_poweredUp == 0 && this->m_stamina > 99
                        && slot->RectContains(slot->m_10->m_screenX, slot->m_10->m_screenY) != 0
                        && slot->m_10->m_screenX == slot->m_lastTilePxX
                        && slot->m_10->m_screenY == slot->m_lastTilePxY) {
                        CommitNeighbor(
                            slot->m_tileOwnerHi,
                            slot->m_tileOwnerLo,
                            slot->m_lastTilePxX,
                            slot->m_lastTilePxY
                        );
                        this->m_defenderState = 2;
                    }
                }
            } else {
                this->m_arrivalCol = -1;
                this->m_defenderState = 0;
                this->m_arrivalRow = -1;
            }
            break;
        }
        case 2:
            this->m_defenderState = 1;
            break;
    }

    if (this->CoordCount() != 0) {
        // The active-move cell: (head node)->link is a [col,row]; gate on the grid
        // cell's flag byte (&0x20).
        i32* cell = (i32*)this->CoordHead()->m_coord;
        u8* flags = (u8*)(((GruntBoard*)g_gameReg->m_tileGrid)->m_8[cell[1]] + cell[0] * 0x1c);
        if ((flags[0] & 0x20) != 0) {
            SetEntrancePos(1, 1);
            if (this->CoordCount() != 0) {
                void* p = (void*)this->CoordHead();
                void* prev = g_coordPool.m_freeHead;
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_coordPool.m_freeHead = (void*)(*link - g_coordPool.m_linkOffset);
                        *(void**)g_coordPool.m_freeHead = prev;
                        prev = g_coordPool.m_freeHead;
                    }
                }
                m_31c.RemoveAll();
            }
            SetEntrancePos(cell[0] * 0x20 + 0x10, cell[1] * 0x20 + 0x10);
        }
    }
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
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), "I") == 0) {
        return 1;
    }
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    GetScreenPos((GruntTilePos*)c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    GetScreenPos((GruntTilePos*)c2);
    i32 cy = c2[1] >> 5;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = g->m_10->m_screenX;
        if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
            && RectContains(x, g->m_10->m_screenY) != 0) {
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
        m_390 = 0;
        goto L_ed006b;
    }
    if (m_neighborValid != 0) {
        return 1;
    }
    if (m_combatActive == 0 && m_stamina >= 100) {
        if (atTarget) {
            CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, g->m_lastTilePxX, g->m_lastTilePxY);
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
        m_390 = 0;
        goto L_scanb;
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (m_stamina >= 100 && g->m_10->m_screenX == g->m_lastTilePxX
        && g->m_10->m_screenY == g->m_lastTilePxY
        && RectContains(g->m_10->m_screenX, g->m_10->m_screenY) != 0) {
        CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, g->m_lastTilePxX, g->m_lastTilePxY);
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (static_cast<u32>(m_dwell) <= 0x1f4) {
        goto L_scanb;
    }
    {
        i32 cc[4];
        g->GetScreenPos((GruntTilePos*)cc);
        if (TileSwitch(cc[0] >> 5, cc[1] >> 5, 0, m_arrivalFlags, 1, 0) != 0) {
            if (m_390 != 0) {
                i32 x = m_10->m_screenX;
                i32 y = m_10->m_screenY;
                if (GruntPointVisible(
                        (i32)((CCueRect*)&g_gameReg->m_world->m_level->m_mainPlane->m_originX),
                        x,
                        y
                    )
                    != 0) {
                    g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                }
                m_390 = 0;
            }
            m_dwell = 0;
        }
    }

L_scanb:
    if (CoordCount() != 0) {
        CScanCoord* coord = ((CScanListNode*)m_31c.GetHeadPosition())->m_coord;
        i32 col = coord->x;
        i32 row = coord->y;
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
    CGruntLiveNode* node = (CGruntLiveNode*)m_tileMgr->m_baseList.GetHeadPosition();
    while (node != 0) {
        CGruntPuddle* gg = node->m_entry;
        node = node->m_next;
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
            i32 dx = gx - (m_10->m_screenX >> 5);
            IABS(dx);
            i32 dy = gy - (m_10->m_screenY >> 5);
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
// arrival-defender regalloc/redundant-recheck wall (~big body): the prologue (m_248
// dirty stamp, GetOccupant settle + RectContains in-range latch), the powered-up release
// gate (FindGridNeighbor + the m_220/m_21c/m_218/m_1e4 clear-state with its cached
// ecx=m_220/eax=m_21c re-reads), and every m_defenderState (0/1/2/3) case (the grid-occupant
// CommitNeighbor commits, the 4-way StepArrivalDrop tile-walk toward m_defenderX/Y, the
// on-screen CueA 0x366) are byte-faithful in shape/offsets/symbols/constants. Residue:
// retail caches m_220/m_21c in callee-saved regs across the GetOccupant call and folds
// the switch-bound 3 into the m_defenderState=3 store (ebx pin); structured C++ re-reads the members
// + materializes the immediate, permuting the register pins across the redundant arrival
// re-checks. Source-invariant (the documented regalloc/recheck wall); deferred to the
// final sweep.
RVA(0x000f1c70, 0x60d)
i32 CGrunt::StepArrivalDefenseAlt() {
    m_arrivalFlags |= 0x40000;
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    i32 inRange = 0;
    if (occ != 0 && occ->m_10->m_screenX == occ->m_lastTilePxX
        && occ->m_10->m_screenY == occ->m_lastTilePxY
        && RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) != 0) {
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
                if (m_stamina >= 0x64 && o->m_10->m_screenX == o->m_lastTilePxX
                    && o->m_10->m_screenY == o->m_lastTilePxY
                    && RectContains(o->m_10->m_screenX, o->m_10->m_screenY) != 0) {
                    CommitNeighbor(
                        o->m_tileOwnerHi,
                        o->m_tileOwnerLo,
                        o->m_lastTilePxX,
                        o->m_lastTilePxY
                    );
                    return 1;
                }
            }
            if (m_poweredUp != 0) {
                goto tail;
            }
            if (m_lastTilePxX != m_entrancePxX || m_lastTilePxY != m_entrancePxY) {
                goto tail;
            }
            {
                i32 tx = m_lastTilePxX >> 5;
                i32 ty = m_lastTilePxY >> 5;
                i32 gx = m_defenderX >> 5;
                i32 gy = m_defenderY >> 5;
                if (tx < gx) {
                    if (ty < gy) {
                        StepArrivalDrop(
                            m_lastTilePxX + 0x40,
                            m_lastTilePxY,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        return 1;
                    }
                    if (ty > gy) {
                        StepArrivalDrop(
                            m_lastTilePxX,
                            m_lastTilePxY - 0x40,
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
                            m_lastTilePxX,
                            m_lastTilePxY + 0x40,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        return 1;
                    }
                    if (ty > gy) {
                        StepArrivalDrop(
                            m_lastTilePxX - 0x40,
                            m_lastTilePxY,
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
            CGrunt* o = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != 0 && g != o) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
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
            if (GruntInRadius(m_arrivalCol, m_arrivalRow) == 0) {
                goto resetState;
            }
            StepArrivalDrop(o->m_lastTilePxX, o->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
            if (m_poweredUp != 0) {
                goto tail;
            }
            if (m_stamina < 0x64) {
                goto tail;
            }
            if (RectContains(o->m_10->m_screenX, o->m_10->m_screenY) == 0) {
                goto tail;
            }
            if (o->m_10->m_screenX != o->m_lastTilePxX) {
                goto tail;
            }
            if (o->m_10->m_screenY != o->m_lastTilePxY) {
                goto tail;
            }
            CommitNeighbor(o->m_tileOwnerHi, o->m_tileOwnerLo, o->m_lastTilePxX, o->m_lastTilePxY);
            m_defenderState = 2;
            return 1;
        }

        case 2:
            m_defenderState = 0;
            return 1;

        case 3: {
            StepArrivalDrop(m_defenderX - 0x20, m_defenderY - 0x20, 0, m_arrivalFlags, 1, 0);
            if (m_10->m_screenX == m_defenderX - 0x20 && m_10->m_screenY == m_defenderY - 0x20) {
                m_defenderState = 0;
                return 1;
            }
            CGrunt* o = m_tileMgr->FindNearestEnemy(this);
            if (o == 0) {
                goto tail;
            }
            if (m_poweredUp == 0 && m_stamina >= 0x64 && o->m_10->m_screenX == o->m_lastTilePxX
                && o->m_10->m_screenY == o->m_lastTilePxY
                && RectContains(o->m_10->m_screenX, o->m_10->m_screenY) != 0) {
                CommitNeighbor(
                    o->m_tileOwnerHi,
                    o->m_tileOwnerLo,
                    o->m_lastTilePxX,
                    o->m_lastTilePxY
                );
                m_defenderState = 2;
            }
            if (GruntInRadius(o->m_tileOwnerHi, o->m_tileOwnerLo) == 0) {
                goto tail;
            }
            m_arrivalCol = o->m_tileOwnerHi;
            m_arrivalRow = o->m_tileOwnerLo;
            m_defenderState = 1;
            {
                CGameObject* h = m_10;
                i32 x = h->m_screenX;
                i32 y = h->m_screenY;
                i32* rect =
                    &g_gameReg->m_world->m_level->m_mainPlane->m_originX; // the +0x40 visible rect
                if (x < rect[2] && x >= rect[0] && y < rect[3] && y >= rect[1]) {
                    g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
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

// ---------------------------------------------------------------------------
// CGrunt::ResolveArrivalNeighbor() @0xf26f0 - the per-frame arrival follow-up,
// active only while the grunt is mid-arrival (m_defenderState==2). When powered-up
// (m_poweredUp) it re-resolves the stored grid neighbour once stamina is full;
// otherwise it clears the arrival latch, looks up the grunt currently occupying
// its tile (m_tileMgr->GetOccupant), and - if that occupant is settled on its own
// tile and on-screen (RectContains) - commits a neighbour link to it. __thiscall,
// ret 0, always returns 1.
// @early-stop
// regalloc wall: logic/CFG/cue-paths exact, the m_defenderState dispatch is the retail
// switch subtract-chain (sub 0/sub 2, see switch-subtract-chain-vs-ifelse). Residue
// = retail pins `this` in edi + occupant in esi (mine flips them) and pre-stages
// the CommitNeighbor stack args (sub esp,8; redundant [esp+8]/[esp+1c] copies) -
// pure register/scheduling placement, no source lever flips it. ~75%.
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
    if (RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) == 0) {
        return 1;
    }
    if (m_10->m_screenX != occ->m_lastTilePxX) {
        return 1;
    }
    if (m_10->m_screenY != occ->m_lastTilePxY) {
        return 1;
    }
    CommitNeighbor(occ->m_tileOwnerHi, occ->m_tileOwnerLo, occ->m_lastTilePxX, occ->m_lastTilePxY);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalDefense()   @0xf2b20   (__thiscall, ret 0 -> 1)
// The multi-state arrival-defender step (the big sibling of ResolveArrival-
// Reposition / ResolveArrivalNeighbor). Latch the defender position to the last
// tile, then dispatch on m_defenderState:
//   state 2: resolve the stored grid occupant; if it is in radius + committed +
//            settled on its own tile + on-screen, commit the tile slot (m_198==0x1e)
//            or neighbour-link onto it; on a gate miss mark m_defenderState=1 and cue.
//   state 1: re-resolve the grid occupant + GetOccupant agreement; gated on dwell
//            run a StepArrivalDrop, then commit/neighbour-link and advance to state 2.
//   state 0: GetOccupant; if settled + on-screen commit/neighbour onto it, else
//            (dwell elapsed) tile-switch to the occupant + advance to state 1 + cue,
//            or (no occupant) reset the idle timer / re-roll a random in-region target.
// @early-stop
// shared ResolveArrivalReposition wall: the m_defenderState switch subtract-chain, the grid
// index math, the in-radius/settled/on-screen gates, the CommitTileSlot/CommitNeighbor
// paths, the 64-bit sbb idle-timer, the idiv/rand re-roll + abs spans and the
// structure-1 cue are all reconstructed in shape/order. Residue is the MSVC /O2
// idiv/rand scheduling, the shared-tail zero-register sharing and the redundant
// CommitNeighbor stack spills - pure regalloc/scheduling placement. Final sweep.
RVA(0x000f2b20, 0x6e1)
i32 CGrunt::StepArrivalDefense() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    CGrunt* occ;
    switch (m_defenderState) {
        case 2:
            if (m_poweredUp == 0) {
                m_defenderState = 1;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
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
            if (RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) == 0) {
                goto c2_miss;
            }
            if (occ->m_10->m_screenX != occ->m_lastTilePxX) {
                goto c2_miss;
            }
            if (occ->m_10->m_screenY != occ->m_lastTilePxY) {
                goto c2_miss;
            }
            if (m_198 == 0x1e) {
                g_gameReg->m_cmdGrid->ApplyTriggerB(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    occ->m_10->m_screenX,
                    occ->m_10->m_screenY
                );
                return 1;
            }
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePxX,
                occ->m_lastTilePxY
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
                CGameObject* h = m_10;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                char* m24 = *(char**)((char*)g_gameReg->m_world + 0x24);
                i32* rect = (i32*)(*(char**)(m24 + 0x5c) + 0x40);
                if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                    g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case 1: {
            occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != 0 && g != occ) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
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
                StepArrivalDrop(occ->m_lastTilePxX, occ->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) == 0) {
                return 1;
            }
            if (m_198 == 0x1e) {
                g_gameReg->m_cmdGrid->ApplyTriggerB(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    occ->m_10->m_screenX,
                    occ->m_10->m_screenY
                );
                m_defenderState = 2;
                return 1;
            }
            if (occ->m_10->m_screenX == occ->m_lastTilePxX
                && occ->m_10->m_screenY == occ->m_lastTilePxY) {
                CommitNeighbor(
                    occ->m_tileOwnerHi,
                    occ->m_tileOwnerLo,
                    occ->m_lastTilePxX,
                    occ->m_lastTilePxY
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
            if (m_poweredUp == 0 && m_stamina >= 0x64 && occ->m_10->m_screenX == occ->m_lastTilePxX
                && occ->m_10->m_screenY == occ->m_lastTilePxY
                && RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) != 0) {
                if (m_198 == 0x1e) {
                    g_gameReg->m_cmdGrid->ApplyTriggerB(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        occ->m_10->m_screenX,
                        occ->m_10->m_screenY
                    );
                    return 1;
                }
                if (occ->m_10->m_screenX != occ->m_lastTilePxX) {
                    return 1;
                }
                if (occ->m_10->m_screenY != occ->m_lastTilePxY) {
                    return 1;
                }
                CommitNeighbor(
                    occ->m_tileOwnerHi,
                    occ->m_tileOwnerLo,
                    occ->m_lastTilePxX,
                    occ->m_lastTilePxY
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
                GruntTilePos sp;
                occ->GetScreenPos(&sp);
                if (TileSwitch(sp.m_x >> 5, sp.m_y >> 5, 0, m_arrivalFlags, 1, 0) == 0) {
                    goto L_f318a;
                }
                SetEntrancePos(1, 1);
                m_arrivalCol = occ->m_tileOwnerHi;
                m_arrivalRow = occ->m_tileOwnerLo;
                m_defenderState = 1;
                CGameObject* h = m_10;
                char* m24 = *(char**)((char*)g_gameReg->m_world + 0x24);
                i32* rect = (i32*)(*(char**)(m24 + 0x5c) + 0x40);
                if (CueVisible((i32)rect, h->m_screenX, h->m_screenY) == 0) {
                    goto L_f318a;
                }
                g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
            }
        L_f318a:
            m_dwell = 0;
            return 1;
        L_f308a:
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_318 == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= 0xbb8) {
                return 1;
            }
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_arrivalRerollLo
                >= *(i64*)&m_arrivalRerollWindowLo) {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollLo = 0;
                m_arrivalRerollWindowLo = 0;
                m_arrivalRerollHi = 0;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollWindowLo = GruntRand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
                m_dwell = 0;
                return 1;
            }
            {
                CGameObject* h = m_10;
                i32 baseX = h->m_extentL;
                i32 spanX = abs(h->m_extentR - baseX);
                i32 baseY = h->m_extentT;
                i32 spanY = abs(h->m_extentB - baseY);
                i32 outX = baseX;
                if (spanX != 0) {
                    outX += GruntRand() % spanX;
                }
                i32 outY = baseY;
                if (spanY != 0) {
                    outY += GruntRand() % spanY;
                }
                if (outX < ((GruntBoard*)g_gameReg->m_tileGrid)->m_c
                    && outY < ((GruntBoard*)g_gameReg->m_tileGrid)->m_10) {
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

// ===========================================================================
// @early-stop
// Sibling of ArrivalScanA: stamps m_300/m_304 AFTER the atTarget probe, duplicates
// the powered-up reset per branch, gates the box scan on m_220==0 too, and matches
// grid cells by flag 0x10000 (active-move by 0x40|0x10000). Logic reconstructed fully;
// same deep-regalloc + slot-recycle wall family. Final-sweep candidate.
RVA(0x000f36a0, 0x78e)
i32 CGrunt::ArrivalScanC() {
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), "I") == 0) {
        return 1;
    }
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    GetScreenPos((GruntTilePos*)c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    GetScreenPos((GruntTilePos*)c2);
    i32 cy = c2[1] >> 5;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = g->m_10->m_screenX;
        if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
            && RectContains(x, g->m_10->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;

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
        m_390 = 0;
        goto L_tailc;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
    }
    if (m_stamina >= 100 && g->m_10->m_screenX == g->m_lastTilePxX
        && g->m_10->m_screenY == g->m_lastTilePxY
        && RectContains(g->m_10->m_screenX, g->m_10->m_screenY) != 0) {
        CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, g->m_lastTilePxX, g->m_lastTilePxY);
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
    }
    if (static_cast<u32>(m_dwell) <= 0x1f4) {
        goto L_tailc;
    }
    if (TileSwitch(g->m_10->m_screenX >> 5, g->m_10->m_screenY >> 5, 0, m_arrivalFlags, 1, 0)
        != 0) {
        if (m_390 != 0) {
            CCueRect* board = (CCueRect*)&g_gameReg->m_world->m_level->m_mainPlane->m_originX;
            i32 x = m_10->m_screenX;
            i32 y = m_10->m_screenY;
            if (x < board->right && board->left <= x && y < board->bottom && board->top <= y) {
                g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
            }
            m_390 = 0;
        }
        m_dwell = 0;
    }

L_tailc:
    if (CoordCount() != 0) {
        CScanCoord* coord = ((CScanListNode*)m_31c.GetHeadPosition())->m_coord;
        i32 col = coord->x;
        i32 row = coord->y;
        CScanCell* cell = &grid->m_8[row][col];
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
// regalloc + region-build wall. Complete reconstruction folded onto the canonical
// CGrunt: the type-name gate (inline strcmp of g_typeColl.Lookup(m_14->m_1c) vs "F"),
// the m_defenderState state dispatch (0x19/0x1a re-mark, 0/2/4), the 5x5-border
// 16-point accumulator build + random-free-cell relocation with tile marking
// (TileSwitch), the state-0 neighbour resolve (GetOccupant + RectContains/CommitNeighbor/
// GruntInRadius + m_cueSink->CueA on-screen cue), and the common tail's coord recycle +
// CommitTileSlot2 arrival commit all align by shape (llvm-objdump -dr). Residual: MSVC5
// pins the tile coords/loop indices across esi/edi/ebp/ebx and schedules the 16 unrolled
// packed-point stores + IntersectRect rect temporaries at [esp+N] slots a source
// transcription can't reproduce exactly.
RVA(0x000f60f0, 0xb30)
i32 CGrunt::PhaseStep() {
    ::CDWordArray acc;
    GruntTilePos pa;
    GruntTilePos pb;

    m_358 = 0;
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), g_codeF) == 0) {
        return 1;
    }
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;

    if (m_defenderState == 0x19) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> 5;
        GetScreenPos(&pb);
        i32 gx = (pb.m_x >> 5) - m_arrivalCol + ax;
        GetScreenPos(&pa);
        i32 ay = pa.m_y >> 5;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> 5) - m_arrivalRow + ay;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        m_defenderState = 4;
    }
    if (m_defenderState == 0x1a) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> 5;
        GetScreenPos(&pb);
        GetScreenPos(&pa);
        i32 gx = (pb.m_x >> 5) - m_arrivalCol + ax;
        i32 ay = pa.m_x >> 5;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> 5) - m_arrivalRow + ay;
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
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), g_codeF) == 0) {
        goto common;
    }
    i32 x = m_arrivalCol;
    i32 y = m_arrivalRow;
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    {
        RECT box;
        box.left = x - 4;
        box.top = y - 4;
        box.right = x + 5;
        box.bottom = y + 5;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_c;
        gb.bottom = grid->m_10;
        if (!IntersectRect(&grid->m_60, &box, &gb)) {
            grid->m_60 = box;
        }
        grid->m_70 = grid->m_60.right - grid->m_60.left;
        grid->m_74 = grid->m_60.bottom - grid->m_60.top;
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
        i32 sel = GruntRand() % acc.GetSize();
        i32 pt = acc.GetAt(sel);
        i32 px = static_cast<u32>(pt) >> 0x10;
        i32 py = pt & 0xffff;
        CScanGrid* pl = (CScanGrid*)g_gameReg->m_tileGrid;
        i32 flag;
        if (static_cast<u32>(px) < static_cast<u32>(pl->m_c) && static_cast<u32>(py) < static_cast<u32>(pl->m_10) && px < pl->m_c && py < pl->m_10) {
            flag = ((i32*)pl->m_8[py])[px * 8 - px];
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
    CScanGrid* pl2 = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_BOUNDS(pl2);
    ((CByteArray*)&acc)->~CByteArray();
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
    if (m_poweredUp == 0 && m_stamina >= 0x64 && nb->m_10->m_screenX == nb->m_lastTilePxX
        && nb->m_10->m_screenY == nb->m_lastTilePxY
        && RectContains(nb->m_10->m_screenX, nb->m_10->m_screenY) != 0) {
        CommitNeighbor(nb->m_tileOwnerHi, nb->m_tileOwnerLo, nb->m_lastTilePxX, nb->m_lastTilePxY);
        m_arrivalCol = nb->m_10->m_screenX >> 5;
        m_arrivalRow = nb->m_10->m_screenY >> 5;
        m_defenderState = 2;
        goto common;
    }
    if (m_dwell <= 0x1f4) {
        goto common;
    }
    if (GruntInRadius(nb->m_tileOwnerHi, nb->m_tileOwnerLo) == 0) {
        goto s0_reset;
    }
    if (TileSwitch(nb->m_10->m_screenX >> 5, nb->m_10->m_screenY >> 5, 0, m_arrivalFlags, 1, 0)
        == 0) {
        m_24c |= 0x4020;
        TileSwitch(nb->m_10->m_screenX >> 5, nb->m_10->m_screenY >> 5, 0, m_arrivalFlags, 1, 0);
        m_24c &= 0xffffbfdf;
    }
    m_dwell = 0;
    if (m_390 == 0) {
        goto common;
    }
    if (GruntPointVisible(
            (i32)&g_gameReg->m_world->m_level->m_mainPlane->m_originX,
            m_10->m_screenX,
            m_10->m_screenY
        )
        == 0) {
        goto s0_reset;
    }
    g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
s0_reset:
    m_390 = 0;
    goto common;
}

common: {
    i32 st = m_defenderState;
    if (st != 4 && st != 0x19 && CoordCount() >= 2) {
        GruntCoordNode* head = CoordHead();
        i32 bx = head->m_coord->m_x;
        i32 by = head->m_coord->m_y;
        GruntCoord* nc = head->m_next->m_coord;
        i32 fx = nc->m_x;
        i32 fy = nc->m_y;
        CScanGrid* pl = (CScanGrid*)g_gameReg->m_tileGrid;
        i32 flag;
        if (static_cast<u32>(fx) < static_cast<u32>(pl->m_c) && static_cast<u32>(fy) < static_cast<u32>(pl->m_10)) {
            flag = ((i32*)pl->m_8[fy])[fx * 8 - fx];
        } else {
            flag = 1;
        }
        if ((flag & 0x20) != 0) {
            if (CoordCount() != 0) {
                RECYCLE_COORDS(CoordHead());
                m_31c.RemoveAll();
            }
            g_gameReg->m_cmdGrid
                ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, bx * 32 + 16, by * 32 + 16);
            m_arrivalCol = bx;
            m_arrivalRow = by;
            m_defenderState = 0x19;
            return 1;
        }
    }
    if (CoordCount() == 0) {
        return 1;
    }
    GruntCoord* p1 = CoordHead()->m_coord;
    CScanGrid* pl2 = (CScanGrid*)g_gameReg->m_tileGrid;
    i32 gx = p1->m_x;
    i32 gy = p1->m_y;
    i32 flag2;
    if (static_cast<u32>(gx) < static_cast<u32>(pl2->m_c) && static_cast<u32>(gy) < static_cast<u32>(pl2->m_10)) {
        flag2 = ((i32*)pl2->m_8[gy])[gx * 8 - gx];
    } else {
        flag2 = 1;
    }
    if ((flag2 & 0x20) == 0) {
        return 1;
    }
    m_arrivalCol = gx;
    m_arrivalRow = gy;
    if (CoordCount() != 0) {
        RECYCLE_COORDS(CoordHead());
        m_31c.RemoveAll();
    }
    m_defenderState = 0x1a;
    return 1;
}
}

// ===========================================================================
// @early-stop
// Seek variant of UpdateArrival (scan the 15 grid slots for the nearest live
// target, drop the queued move nodes, re-probe). Same reloc-masked engine-call set +
// the deep regalloc / cold-block wall. Logic reconstructed faithfully. Final-sweep
// candidate.
RVA(0x000f71c0, 0x721)
i32 CGrunt::SeekTarget() {
    this->m_defenderX = this->m_lastTilePxX;
    this->m_defenderY = this->m_lastTilePxY;
    if (this->CoordCount() != 0
        && g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + this->m_arrivalCol] == 0) {
        void* p = (void*)this->CoordHead();
        while (p != 0) {
            void* next = *(void**)p;
            i32* link = (i32*)((char*)p + 8);
            p = next;
            if (*link != 0) {
                g_coordPool.Push((void*)(*link));
            }
        }
        m_31c.RemoveAll();
        this->m_arrivalCol = 0;
    }

    i32 reason = this->m_entranceReason;
    if (reason > 0x16) {
        reason = this->m_19c;
    }
    if (reason == 0 && (reason = this->m_arrivalCol, reason >= 0) && reason < 0xf) {
        CGrunt* slot = g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + reason];
        if (slot == 0 || slot->m_entranceCommitted == 0) {
            if (this->CoordCount() != 0) {
                void* p = (void*)this->CoordHead();
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_coordPool.Push((void*)(*link));
                    }
                }
                m_31c.RemoveAll();
            }
            this->m_arrivalCol = -1;
            return 1;
        }
        // Adjacency probe: read this grunt's HUD center + the slot's, in tile units,
        // and require both axis deltas < 2 (the slot is the immediate neighbor).
        i32 c0[4];
        GetScreenPos((GruntTilePos*)c0);
        i32 cy = c0[1] >> 5;
        i32 d0[4];
        GetScreenPos((GruntTilePos*)d0);
        i32 e0[4];
        GetScreenPos((GruntTilePos*)e0);
        i32 f0[4];
        GetScreenPos((GruntTilePos*)f0);
        i32 dx = (f0[1] >> 5) - (f0[3] >> 5);
        i32 dy = cy - (e0[3] >> 5);
        if (((dy ^ (dy >> 31)) - (dy >> 31)) < 2 && ((dx ^ (dx >> 31)) - (dx >> 31)) < 2) {
            i32 r2 = slot->m_entranceReason;
            if (r2 > 0x16) {
                r2 = slot->m_19c;
            }
            if (r2 != 0x14 && r2 != 1) {
                slot->LoadGruntTypeTable(r2, 1, 0, 0);
                slot->LoadGruntTypeTable(0, 1, 0, 0);
                this->m_defenderState = 4;
                if (this->CoordCount() == 0) {
                    return 1;
                }
                void* p = (void*)this->CoordHead();
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_coordPool.Push((void*)(*link));
                    }
                }
                m_31c.RemoveAll();
                return 1;
            }
        }
    }

    reason = this->m_entranceReason;
    if (reason > 0x16) {
        reason = this->m_19c;
    }
    if (reason == 0) {
        if (this->CoordCount() == 0) {
            if (this->m_defenderState != 0) {
                return 1;
            }
            i32 best = 0x7fffffff;
            i32 bestIdx = -1;
            CGrunt** slots = g_gameReg->m_cmdGrid->m_grid; // row 0 (the flat 4x15 board)
            i32 i = 0;
            do {
                CGrunt* sv = slots[i];
                if (sv != 0 && sv->m_entranceCommitted != 0) {
                    i32 k = sv->m_entranceReason;
                    i32 kk = k;
                    if (k > 0x16) {
                        kk = sv->m_19c;
                    }
                    if (kk != 0 && kk != 0x14 && kk != 1
                        && !(k > 0x16 ? (sv->m_19c == 0x14) : false) && sv->m_gruntKind != 0x36) {
                        i32 ex = sv->m_10->m_screenX >> 5;
                        i32 ddx = ex - (this->m_10->m_screenX >> 5);
                        i32 ey = (sv->m_10->m_screenY >> 5) - (this->m_10->m_screenY >> 5);
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
                this->m_arrivalCol = bestIdx;
                CGameObject* base = slots[bestIdx]->m_10;
                if (TileSwitch(
                        base->m_screenX >> 5,
                        base->m_screenY >> 5,
                        0,
                        this->m_arrivalFlags,
                        1,
                        0
                    )
                    != 0) {
                    i32 by = this->m_10->m_screenY;
                    i32 bx = this->m_10->m_screenX;
                    CCueRect* board =
                        (CCueRect*)&g_gameReg->m_world->m_level->m_mainPlane->m_originX;
                    if (bx < board->right && board->left <= bx && by < board->bottom
                        && board->top <= by) {
                        g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
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
            g_gameReg->m_cmdGrid->m_grid[0 * TM_GRID_COLS + this->m_arrivalCol]->m_10;
        TileSwitch(base->m_screenX >> 5, base->m_screenY >> 5, 0, this->m_arrivalFlags, 1, 0);
    } else {
        CGrunt* g = m_tileMgr->FindNearestEnemy(this);
        bool atTarget = false;
        if (g != 0) {
            i32 x = g->m_10->m_screenX;
            if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
                && g->RectContains(x, (i32)g->m_10) != 0) {
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
        this->m_defenderX = this->m_lastTilePxX;
        this->m_defenderY = this->m_lastTilePxY;
        if (g == 0 || g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
            this->m_390 = 0;
            return 1;
        }
        if (this->m_poweredUp == 0 && this->m_stamina > 99) {
            i32 x = g->m_10->m_screenX;
            if (x == g->m_lastTilePxX && g->m_10->m_screenY == g->m_lastTilePxY
                && g->RectContains(x, (i32)g->m_10) != 0) {
                CommitNeighbor(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    g->m_lastTilePxX,
                    g->m_lastTilePxY
                );
            }
        }
        if (static_cast<u32>(this->m_dwell) < 0x1f5) {
            return 1;
        }
        if (TileSwitch(
                g->m_10->m_screenX >> 5,
                g->m_10->m_screenY >> 5,
                0,
                this->m_arrivalFlags,
                1,
                0
            )
            == 0) {
            return 1;
        }
        if (this->m_390 != 0) {
            i32 r = GruntPointVisible(
                (i32)&g_gameReg->m_world->m_level->m_mainPlane->m_originX,
                this->m_10->m_screenX,
                this->m_10->m_screenY
            );
            if (r != 0) {
                g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
            }
            this->m_390 = 0;
            this->m_dwell = 0;
            return 1;
        }
    }
    this->m_dwell = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalDefenseLean()   @0xf8240   (__thiscall, ret 0 -> 1)
// The leaner twin of StepArrivalDefense. First gate: if the current anim name is
// "I" (arrival pose), do nothing. Latch the defender position to the last tile,
// then dispatch on m_defenderState (0/1/2) over the stored grid occupant. Unlike the big
// sibling there are no m_neighborValid / m_198==0x1e CommitTileSlot arms - the
// settled-on-screen occupant goes straight to CommitNeighbor; the gate-miss /
// not-in-radius paths latch m_defenderState=1 + the +0x2ec dwell=0x1f4 and fire the
// on-screen cue. State 0 commits the occupant's tile slot on a rand%100 roll, else
// (dwell elapsed) re-rolls a random in-region target / resets the idle timer.
// @early-stop
// shared StepArrivalDefense regalloc/scheduling wall: the m_defenderState subtract-chain
// switch, the 15-wide grid index, the in-radius/committed/settled/on-screen gates,
// the dual cue blocks (store-before vs store-after), the 64-bit sbb idle-timer, the
// idiv/rand in-region re-roll + abs spans and the max-spread SetEntrancePos
// escalation are reconstructed in shape/order. Residue is the MSVC /O2 idiv/rand
// scheduling + the ebx zero-register tail sharing (same family as the twin).
RVA(0x000f8240, 0x5b9)
i32 CGrunt::StepArrivalDefenseLean() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), g_codeI) == 0) {
        return 1;
    }
    CGrunt* occ;
    switch (m_defenderState) {
        case 2:
            if (m_poweredUp == 0) {
                m_defenderState = 1;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
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
            if (RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) == 0) {
                goto c2_miss;
            }
            if (occ->m_10->m_screenX != occ->m_lastTilePxX) {
                goto c2_miss;
            }
            if (occ->m_10->m_screenY != occ->m_lastTilePxY) {
                goto c2_miss;
            }
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePxX,
                occ->m_lastTilePxY
            );
            return 1;
        c2_miss: {
            CGameObject* h = m_10;
            i32 vx = h->m_screenX;
            i32 vy = h->m_screenY;
            char* m24 = *(char**)((char*)g_gameReg->m_world + 0x24);
            i32* rect = (i32*)(*(char**)(m24 + 0x5c) + 0x40);
            if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
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
                CGameObject* h = m_10;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                char* m24 = *(char**)((char*)g_gameReg->m_world + 0x24);
                i32* rect = (i32*)(*(char**)(m24 + 0x5c) + 0x40);
                if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                    g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case 1: {
            occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != 0 && g != occ) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
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
                StepArrivalDrop(occ->m_lastTilePxX, occ->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < 0x64) {
                return 1;
            }
            if (RectContains(occ->m_10->m_screenX, occ->m_10->m_screenY) == 0) {
                return 1;
            }
            if (occ->m_10->m_screenX != occ->m_lastTilePxX) {
                return 1;
            }
            if (occ->m_10->m_screenY != occ->m_lastTilePxY) {
                return 1;
            }
            CommitNeighbor(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePxX,
                occ->m_lastTilePxY
            );
            m_defenderState = 2;
            return 1;
        }

        case 0:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (GruntRand() % 0x64 == 0 && m_health > 0x1a && occ != 0 && m_stamina >= 0x64
                && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
                m_tileMgr
                    ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, m_lastTilePxX, m_lastTilePxY);
                return 1;
            }
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_318 == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= 0xbb8) {
                return 1;
            }
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_arrivalRerollLo
                >= *(i64*)&m_arrivalRerollWindowLo) {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollWindowLo = GruntRand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
                m_dwell = 0;
                return 1;
            }
            {
                CGameObject* h = m_10;
                i32 baseX = h->m_extentL;
                i32 spanX = abs(h->m_extentR - baseX);
                i32 baseY = h->m_extentT;
                i32 spanY = abs(h->m_extentB - baseY);
                i32 outX = baseX;
                if (spanX != 0) {
                    outX += GruntRand() % spanX;
                }
                i32 outY = baseY;
                if (spanY != 0) {
                    outY += GruntRand() % spanY;
                }
                GruntBoard* bd = (GruntBoard*)g_gameReg->m_tileGrid;
                if (static_cast<u32>(outX) < static_cast<u32>(bd->m_c) && static_cast<u32>(outY) < static_cast<u32>(bd->m_10)) {
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
