#include <rva.h>
#include <new> // placement CRect ctor
#include <Wap32/ZVec.h>

#include <Ints.h>
#include <Mfc.h>        // RECT + IntersectRect (superset of Win32.h; Grunt.h needs MFC)
#include <Wap32/Rect.h> // canonical CRect (0x29ac0 direct-store ctor, was local QuadIntRecord); after Mfc.h (windows.h-first C1189)
#include <math.h>       // fild/fsqrt/__ftol board distance
#include <string.h>     // inline strcmp type-name gate
#include <stdlib.h>     // engine rand (0x11fee0)
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Brickz.h>     // canonical CMapMgr == CMapMgr (the board; was the CStepGrid view)
#include <Gruntz/Grunt.h>      // real CGrunt (step grunt is a CGrunt); m_10 + CAnimLookupNode m_14
#include <Gruntz/TriggerMgr.h> // CTriggerMgr (the board's 4x15 CGrunt* grid; was the CStepBoard view)
#include <Gruntz/BattlezMapConfig.h> // CBattlezMapConfig - the step mgr `this`
#include <Gruntz/TypeColl.h>         // the shared type-name collection
#include <Gruntz/TypeKeyColl.h>

#include <Gruntz/TypeKeyColl.h>

#define STEP_DRAIN(g)                                                                              \
    {                                                                                              \
        GruntCoordNode* nd = (g)->CoordHead();                                                     \
        if (nd != 0) {                                                                             \
            do {                                                                                   \
                void* r = ListNodeAdvance(reinterpret_cast<void**>(&nd));                          \
                if (*static_cast<i32*>(r) != 0) {                                                  \
                    g_coordPool.Push(reinterpret_cast<void*>(*static_cast<i32*>(r)));              \
                }                                                                                  \
            } while (nd != 0);                                                                     \
        }                                                                                          \
        (g)->m_31c.RemoveAll();                                                                    \
    }

#define STEP_BOUNDS(grid)                                                                          \
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
        Coord tp;
        g->GetScreenPos(&tp);
        CGrunt* nb = FindIdleGruntInBox(tp.m_x >> 5, tp.m_y >> 5, m_08c, m_090);
        if (nb != 0) {
            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }
            // board distance nb <-> g
            Coord np, gp, np2, gp2;
            nb->GetScreenPos(&np);
            g->GetScreenPos(&gp);
            nb->GetScreenPos(&np2);
            g->GetScreenPos(&gp2);
            i32 dist =
                iabs((np2.m_y >> 5) - (gp2.m_y >> 5)) + iabs((np2.m_x >> 5) - (gp2.m_x >> 5));
            if (dist <= 0xa) {
                // dirty-rect box around the grunt
                Coord b0, b1, b2, b3;
                g->GetScreenPos(&b0);
                g->GetScreenPos(&b1);
                g->GetScreenPos(&b2);
                g->GetScreenPos(&b3);
                CMapMgr* grid = m_board;
                RECT box;
                box.left = (b0.m_x >> 5) - 5;
                box.top = (b1.m_y >> 5) - 5;
                box.right = (b2.m_x >> 5) + 5;
                box.bottom = (b3.m_y >> 5) + 5;
                RECT gb;
                static_cast<RECT*>(new (&gb) CRect(0, 0, grid->m_width, grid->m_height));
                if (!IntersectRect(&grid->m_bounds, &box, &gb)) {
                    grid->m_bounds = box;
                }
                grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
                grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
            }
            Coord p;
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
        CGameObject* s = cur->m_object;
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
                    (reinterpret_cast<CTypeNode*>(
                         g_typeColl._zdvec::IndexToPtr(reinterpret_cast<i32>(g->m_objAux->m_1c))
                     ))
                        ->m_0;
                if (strcmp(nm, "I") != 0 && strcmp(nm, "G") != 0 && strcmp(nm, "L") != 0
                    && strcmp(nm, "P") != 0 && strcmp(nm, "J") != 0 && strcmp(nm, "C") != 0
                    && strcmp(nm, "R") != 0) {
                    Finish3e4f(g, cur);
                }
            }
            g->m_defenderState = 0;
            goto tail;
        }
        // not arrived: reroute by Euclidean board distance
        Coord here, np;
        g->GetTilePos(&here);
        cur->GetTilePos(&np);
        i32 dx = np.m_x - here.m_x;
        i32 dy = np.m_y - here.m_y;
        i32 dist = static_cast<i32>(
            sqrt(static_cast<double>((iabs(dx) * iabs(dx) + iabs(dy) * iabs(dy))))
        );
        if (dist > m_0a4) {
            if (m_0f0.GetSize() != 0) {
                GruntCoord* e =
                    (reinterpret_cast<GruntCoord**>(m_0f0.GetData()))[rand() % m_0f0.GetSize()];
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
        Coord c0, c1, c2, c3;
        cur->GetScreenPos(&c0);
        g->GetScreenPos(&c1);
        cur->GetScreenPos(&c2);
        g->GetScreenPos(&c3);
        i32 dist2 = iabs((c0.m_y >> 5) - (c1.m_y >> 5)) + iabs((c2.m_x >> 5) - (c3.m_x >> 5));
        if (dist2 <= 0xa) {
            Coord d0, d1, d2, d3;
            g->GetScreenPos(&d0);
            g->GetScreenPos(&d1);
            g->GetScreenPos(&d2);
            g->GetScreenPos(&d3);
            CMapMgr* grid = m_board;
            RECT box;
            box.left = (d0.m_x >> 5) - 5;
            box.top = (d1.m_y >> 5) - 5;
            box.right = (d2.m_x >> 5) + 5;
            box.bottom = (d3.m_y >> 5) + 5;
            RECT gb;
            static_cast<RECT*>(new (&gb) CRect(0, 0, grid->m_width, grid->m_height));
            if (!IntersectRect(&grid->m_bounds, &box, &gb)) {
                grid->m_bounds = box;
            }
            grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
            grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
        }
        Coord cp;
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
    if (CanPlaySpecialAnim(reinterpret_cast<i32>(g))) {
        if (g->CoordCount() == 0 && static_cast<u32>(g->m_dwell) > static_cast<u32>(m_0a0)
            && m_0f0.GetSize() != 0) {
            GruntCoord* e =
                (reinterpret_cast<GruntCoord**>(m_0f0.GetData()))[rand() % m_0f0.GetSize()];
            g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            g->m_dwell = 0;
        }
    }
    return 1;
}
