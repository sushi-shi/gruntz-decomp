// GruntStateStep.cpp - CBattlezMapConfig::Step33520 (0x033520, __thiscall ret 4). A
// grunt AI move-resolution step, sibling of CBattlezMapConfig::Step (0x031610,
// BattlezMapConfig.cpp) and the region scan (GruntTileScan.cpp). `this` (ebp) is the
// same CBattlezMapConfig object (board m_triggerMgr, grid m_board, thresholds
// m_08c/m_090/m_0a0/m_0a4, random-goal CPtrArray m_0f0); the argument (esi) is the
// CGrunt being moved. Dispatches on the
// grunt arrival state (g->m_defenderState):
//   state 3 -> return immediately;
//   state 2 -> in-flight: fetch the path node, test arrival (Check3c4c). On arrival
//              latch (-1,-1), and - gated on a set of flags + the grunt NOT being a
//              type in {I,G,L,P,J,C,R} - run Finish3e4f; else clear. Not arrived ->
//              reroute by Euclidean board distance (fild/fsqrt/__ftol);
//   fresh   -> re-query the move grid, recycle the pending-coord list onto
//              g_coordPool, recompute the grid dirty rect and re-target.
// Big /base body (0xbc3, ~30 internal calls). Shares the CGrunt coord-pool / grid
// family with GruntMoveStep.cpp + GruntTileScan.cpp.
#include <rva.h>
#include <new> // placement CRect ctor
#include <Wap32/ZVec.h>

#include <Ints.h>
#include <Mfc.h>        // RECT + IntersectRect (superset of Win32.h; Grunt.h needs MFC)
#include <Wap32/Rect.h> // canonical CRect (0x29ac0 direct-store ctor, was local QuadIntRecord); after Mfc.h (windows.h-first C1189)
#include <math.h>       // fild/fsqrt/__ftol board distance
#include <string.h>     // inline strcmp type-name gate
#include <stdlib.h>     // engine rand (0x11fee0)
#include <Globals.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Brickz.h> // canonical CBrickzGrid == CMapMgr (the board; was the CStepGrid view)
#include <Gruntz/Grunt.h>  // real CGrunt (step grunt is a CGrunt); m_10 + CAnimLookupNode m_14
#include <Gruntz/TriggerMgr.h> // CTriggerMgr (the board's 4x15 CGrunt* grid; was the CStepBoard view)
#include <Gruntz/BattlezMapConfig.h> // CBattlezMapConfig - the step mgr `this`
#include <Gruntz/TypeColl.h>         // the shared type-name collection
#include <Gruntz/TypeKeyColl.h>

// --- offset-faithful views (offsets + called methods load-bearing; reloc-masked) ---
// (CStepList was a third fake name for the REAL MFC CPtrList at g->m_31c - the same
// fabrication Grunt.h records as GruntListSub.  Find1de8 @0x1de8 thunks to the free
// __stdcall ListNodeAdvance @0x29a30; RemoveAll1b48a6 @0x1b48a6 IS CPtrList::RemoveAll.)
void* __stdcall ListNodeAdvance(void** pos); // 0x29a30 (thunk 0x1de8)
// (g->m_10 is the real CGruntHud (m_screenX/m_screenY); g->m_14 is the real
// CAnimLookupNode (m_1c) - both are CGrunt's already-typed sub-objects.)
// CTypeColl was a fake view of the REAL CTypeKeyColl at 0x6bf650 - and it mangled to a
// DIFFERENT symbol, so these three TUs were emitting a divergent name for the same object.
#include <Gruntz/TypeKeyColl.h>

// The single-char type keys pooled in .rdata (named in Globals.cpp).
// k_60cc94 was a SECOND NAME for s_codeJ (0x20cc94) - same address,
// so nothing ever defined it. Unified onto the canonical.

// (the ApiMisc::ClipHost_02b340 shell + the CStepGrid view are GONE - XREF-settled, both of
// them, and they were the same object. The 0x43ea thunk lands on
// ?Clip@CMapMgr@@QAEXPBUtagRECT@@@Z (0x2b340, already reconstructed in
// src/Gruntz/BrickzClip_02b340.cpp), so "SetStepFlag" was CMapMgr::Clip all along; and the
// board this TU reaches through m_c has CMapMgr's layout field for field - +0x0c/+0x10 are
// m_width/m_height, +0x60..+0x6c the bound RECT, +0x70/+0x74 the clipped extents. It IS the
// CBrickzGrid/CMapMgr board. Typing the member with the real class made both the shell and
// the cast at the call fall out.)
// `this` (ebp) is the canonical CBattlezMapConfig: its board
// is m_triggerMgr (CTriggerMgr, the 4x15 CGrunt* m_grid at +0x1c), the pathfinding grid
// is m_board (CBrickzGrid/CMapMgr), the thresholds are m_08c/m_090/m_0a0/m_0a4, and the
// random-goal "table" is the CPtrArray m_0f0 (data @ +0xf4 via GetData, count @ +0xf8
// via GetSize). QueryTile4098/Finish3e4f/Method_034460 are its declared methods.

// @0x29ac0 (thunk 0x34a4) IS the engine CRect(l,t,r,b) direct-store ctor (Ghidra/
// FID: ??0CRect@@QAE@HHHH@Z), out-of-line so it is CALLed here. Modeled by the
// canonical CRect (<Wap32/Rect.h>).

// Drain the pending-coord list onto g_coordPool via the CObList Find walk, then
// empty the list.
#define STEP_DRAIN(g)                                                                              \
    {                                                                                              \
        GruntCoordNode* nd = (g)->CoordHead();                                                     \
        if (nd != 0) {                                                                             \
            do {                                                                                   \
                void* r = ListNodeAdvance((void**)&nd);                                            \
                if (*(i32*)r != 0) {                                                               \
                    g_coordPool.Push((void*)(*(i32*)r));                                           \
                }                                                                                  \
            } while (nd != 0);                                                                     \
        }                                                                                          \
        (g)->m_31c.RemoveAll();                                                                    \
    }

// Recompute the grid dirty rect (m_60) as the {0,0,w,h} box intersected with a copy
// of itself.
#define STEP_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        (RECT*)new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height);                           \
        RECT* pb = (RECT*)new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height);                \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect((RECT*)&(grid)->m_originX, &ra, &rb)) {                                 \
            *(RECT*)&(grid)->m_originX = ra;                                                       \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_boundRight - (grid)->m_originX;                                \
        (grid)->m_gridH = (grid)->m_boundBottom - (grid)->m_originY;                               \
    }

static i32 iabs(i32 v) {
    return v < 0 ? -v : v;
}

// @early-stop
// large grunt state-step reconstruction (final-sweep candidate): the m_2d4 3/2/fresh
// dispatch, the path-node arrival (Check3c4c) + flag-gated type-name {I,G,L,P,J,C,R}
// Finish3e4f, the Euclidean fild/fsqrt/__ftol board-distance reroute, the manhattan
// board distance, the g_coordPool recycle drains, the grid dirty-rect recompute and
// the random-goal retarget are byte-shaped and the DATA refs (g_coordPool /
// g_freeList family / g_typeColl / the k_60c* char keys / IntersectRect) pair.
// Residual walls: retail re-calls GetTilePos36c0 several times into overlapping
// coalesced stack slots feeding the box (the optimizer folds the redundant calls
// here), the sqrt is modeled without /Oi, and the shared-landing-pad regalloc /
// slot schedule diverges - re-attack leaf-first in the final sweep.
RVA(0x00033520, 0xbc3)
i32 CBattlezMapConfig::Step33520(CGrunt* g) {
    i32 state = g->m_defenderState;
    if (state == 3) {
        return 1;
    }
    if (state != 2) {
        // ---- fresh: re-query the move grid ----
        GruntTilePos tp;
        g->GetScreenPos(&tp);
        CGrunt* nb = QueryTile4098(tp.m_x >> 5, tp.m_y >> 5, m_08c, m_090);
        if (nb != 0) {
            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }
            // board distance nb <-> g
            GruntTilePos np, gp, np2, gp2;
            nb->GetScreenPos(&np);
            g->GetScreenPos(&gp);
            nb->GetScreenPos(&np2);
            g->GetScreenPos(&gp2);
            i32 dist =
                iabs((np2.m_y >> 5) - (gp2.m_y >> 5)) + iabs((np2.m_x >> 5) - (gp2.m_x >> 5));
            if (dist <= 0xa) {
                // dirty-rect box around the grunt
                GruntTilePos b0, b1, b2, b3;
                g->GetScreenPos(&b0);
                g->GetScreenPos(&b1);
                g->GetScreenPos(&b2);
                g->GetScreenPos(&b3);
                CBrickzGrid* grid = m_board;
                RECT box;
                box.left = (b0.m_x >> 5) - 5;
                box.top = (b1.m_y >> 5) - 5;
                box.right = (b2.m_x >> 5) + 5;
                box.bottom = (b3.m_y >> 5) + 5;
                RECT gb;
                static_cast<RECT*>(new (&gb) CRect(0, 0, grid->m_width, grid->m_height));
                if (!IntersectRect(reinterpret_cast<RECT*>(&grid->m_originX), &box, &gb)) {
                    *reinterpret_cast<RECT*>(&grid->m_originX) = box;
                }
                grid->m_gridW = grid->m_boundRight - grid->m_originX;
                grid->m_gridH = grid->m_boundBottom - grid->m_originY;
            }
            GruntTilePos p;
            nb->GetScreenPos(&p);
            if (g->TileSwitch(p.m_x >> 5, p.m_y >> 5, 0, 0x20000dc7, 0, 0)) {
                g->m_defenderState = 2;
                g->m_arrivalCol = nb->m_tileOwnerHi;
                g->m_arrivalRow = nb->m_tileOwnerLo;
                g->m_dwell = 0;
            }
            if (dist <= 0xa) {
                STEP_BOUNDS(m_board);
            }
        }
        goto tail;
    }

    // ---- state 2: in-flight advance ----
    {
        i32 col = g->m_arrivalCol;
        i32 row = g->m_arrivalRow;
        CGrunt* cur = m_triggerMgr->m_grid[15 * col + row];
        if (cur == 0) {
            // clear path
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            g->m_defenderState = 0;
            GruntRecycleCoords(g);
            goto tail;
        }
        CGameObject* s = cur->m_10;
        if (g->RectContains(s->m_screenX, s->m_screenY) != 0) {
            // arrived on this tile
            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            if (g != 0 && g->IsAtSavedScreenPos() && g->m_entranceCommitted != 0
                && g->m_deathAnimStarted == 0 && g->m_entranceActive == 0 && g->m_poweredUp == 0) {
                const char* nm =
                    (reinterpret_cast<CTypeNode*>((static_cast<zDArray*>(&g_typeColl))->IndexToPtr(reinterpret_cast<i32>(g->m_14->m_1c))))->m_0;
                if (strcmp(nm, "I") != 0 && strcmp(nm, "G") != 0 && strcmp(nm, "L") != 0
                    && strcmp(nm, "P") != 0 && strcmp(nm, "J") != 0
                    && strcmp(nm, "C") != 0 && strcmp(nm, "R") != 0) {
                    Finish3e4f(g, cur);
                }
            }
            g->m_defenderState = 0;
            goto tail;
        }
        // not arrived: reroute by Euclidean board distance
        GruntTilePos here, np;
        g->GetTilePos(&here);
        cur->GetTilePos(&np);
        i32 dx = np.m_x - here.m_x;
        i32 dy = np.m_y - here.m_y;
        i32 dist = static_cast<i32>(sqrt(static_cast<double>((iabs(dx) * iabs(dx) + iabs(dy) * iabs(dy)))));
        if (dist > m_0a4) {
            if (m_0f0.GetSize() != 0) {
                GruntCoord* e = (reinterpret_cast<GruntCoord**>(m_0f0.GetData()))[rand() % m_0f0.GetSize()];
                g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            }
            g->m_arrivalCol = -1;
            g->m_dwell = 0;
            g->m_arrivalRow = -1;
            g->m_defenderState = 0;
            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }
            g->m_dwell = 0;
            goto tail;
        }
        // dist <= m_0a4: drain + recompute dirty rect + retarget
        if (g->CoordCount() != 0) {
            STEP_DRAIN(g);
        }
        GruntTilePos c0, c1, c2, c3;
        cur->GetScreenPos(&c0);
        g->GetScreenPos(&c1);
        cur->GetScreenPos(&c2);
        g->GetScreenPos(&c3);
        i32 dist2 = iabs((c0.m_y >> 5) - (c1.m_y >> 5)) + iabs((c2.m_x >> 5) - (c3.m_x >> 5));
        if (dist2 <= 0xa) {
            GruntTilePos d0, d1, d2, d3;
            g->GetScreenPos(&d0);
            g->GetScreenPos(&d1);
            g->GetScreenPos(&d2);
            g->GetScreenPos(&d3);
            CBrickzGrid* grid = m_board;
            RECT box;
            box.left = (d0.m_x >> 5) - 5;
            box.top = (d1.m_y >> 5) - 5;
            box.right = (d2.m_x >> 5) + 5;
            box.bottom = (d3.m_y >> 5) + 5;
            RECT gb;
            static_cast<RECT*>(new (&gb) CRect(0, 0, grid->m_width, grid->m_height));
            if (!IntersectRect(reinterpret_cast<RECT*>(&grid->m_originX), &box, &gb)) {
                *reinterpret_cast<RECT*>(&grid->m_originX) = box;
            }
            grid->m_gridW = grid->m_boundRight - grid->m_originX;
            grid->m_gridH = grid->m_boundBottom - grid->m_originY;
        }
        GruntTilePos cp;
        cur->GetScreenPos(&cp);
        if (!g->TileSwitch(cp.m_x >> 5, cp.m_y >> 5, 0, 0x20000dc7, 0, 0)) {
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            g->m_defenderState = 0;
        }
        if (dist2 <= 0xa) {
            m_board->Clip(0);
        }
        g->m_dwell = 0;
        goto tail;
    }

tail:
    if (Method_034460(reinterpret_cast<i32>(g))) {
        if (g->CoordCount() == 0 && static_cast<u32>(g->m_dwell) > static_cast<u32>(m_0a0) && m_0f0.GetSize() != 0) {
            GruntCoord* e = (reinterpret_cast<GruntCoord**>(m_0f0.GetData()))[rand() % m_0f0.GetSize()];
            g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            g->m_dwell = 0;
        }
    }
    return 1;
}
