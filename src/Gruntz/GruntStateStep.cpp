// GruntStateStep.cpp - CGruntStepMgr::Step33520 (0x033520, __thiscall ret 4). A
// grunt AI move-resolution step, sibling of CGruntMover::Step (GruntMoveStep.cpp)
// and the region scan (GruntTileScan.cpp). `this` (ebp) is the grunt move manager
// (board m_8, grid m_c, thresholds m_8c/m_90/m_a0/m_a4, random-goal table
// m_f4/m_f8); the argument (esi) is the CGrunt being moved. Dispatches on the
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
#include <math.h>   // fild/fsqrt/__ftol board distance
#include <string.h> // inline strcmp type-name gate
#include <stdlib.h> // engine rand (0x11fee0)
#include <Globals.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>    // real CGrunt (step grunt is a CGrunt)
#include <Gruntz/TypeColl.h> // the shared type-name collection

// --- offset-faithful views (offsets + called methods load-bearing; reloc-masked) ---
struct CStepCoord {
    i32 x, y;
};
struct CStepNode { // g->m_320 pending-coord node
    CStepNode* m_next;
    i32 _04;
    void* m_8; // +0x08
};
struct CStepList {             // g->m_31c (CObList view)
    void* Find1de8(void** it); // 0x1de8
    void RemoveAll1b48a6();    // 0x1b48a6
    char _00[4];
};
struct CStepSub10 { // g->m_10
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c, +0x60
};
struct CStepOwner { // g->m_14
    char _00[0x1c];
    i32 m_1c; // +0x1c type key
};
extern CTypeColl g_typeColl; // ?g_typeColl@@3UCTypeKeyColl@@A (0x6bf650)

// The single-char type keys pooled in .rdata (named in Globals.cpp).
extern char k_60cc94[]; // "J"

// SetStepFlag @0x43ea is ApiMisc::ClipHost_02b340::Clip(const RECT*); TU-local decl, cast at the call.
namespace ApiMisc {
    class ClipHost_02b340 {
    public:
        void Clip(const RECT* r);
    };
} // namespace ApiMisc
struct CStepGrid { // this->m_c
    char _00[0xc];
    i32 m_c, m_10; // +0x0c width, +0x10 height
    char _14[0x60 - 0x14];
    i32 m_60, m_64, m_68, m_6c; // +0x60 dirty rect (l,t,r,b)
    i32 m_70, m_74;             // +0x70 width, +0x74 height
};
struct CStepGoal { // this->m_f4[] element
    i32 m_0, m_4;
};
// The step mgr's board (m_8): a 4x15 grunt-pointer grid at +0x1c, indexed [15*col+row].
struct CStepBoard {
    char _00[0x1c];
    CGrunt* m_grid[60]; // +0x1c
};
struct CStepMgr {                                      // this (ebp)
    CGrunt* QueryTile4098(i32 x, i32 y, i32 a, i32 b); // 0x4098
    void Finish3e4f(CGrunt* g, CGrunt* cur);           // 0x3e4f
    i32 ShouldStepGrunt(CGrunt* g);                    // 0x2626
    char _00[0x8];
    CStepBoard* m_8; // +0x08 board (CGrunt*[] grid at +0x1c)
    CStepGrid* m_c;  // +0x0c grid
    char _10[0x8c - 0x10];
    i32 m_8c, m_90; // +0x8c, +0x90
    char _94[0xa0 - 0x94];
    i32 m_a0, m_a4; // +0xa0 idle threshold, +0xa4 reroute distance
    char _a8[0xf4 - 0xa8];
    CStepGoal** m_f4; // +0xf4 goal table
    i32 m_f8;         // +0xf8 goal count
    i32 Step33520(CGrunt* g);
};

// @0x29ac0 (thunk 0x34a4) IS the engine CRect(l,t,r,b) direct-store ctor (Ghidra/
// FID: ??0CRect@@QAE@HHHH@Z), out-of-line so it is CALLed here. Modeled by the
// canonical CRect (<Wap32/Rect.h>); the old local QuadIntRecord view folded onto it.

extern FreeNodePool g_coordPool; // ?g_coordPool@@... (0x645540): Drop recycles a node

// Drain the pending-coord list onto g_coordPool via the CObList Find walk, then
// empty the list.
#define STEP_DRAIN(g)                                                                              \
    {                                                                                              \
        CStepNode* nd = (CStepNode*)(g)->m_320;                                                    \
        if (nd != 0) {                                                                             \
            do {                                                                                   \
                void* r = ((CStepList*)&(g)->m_31c)->Find1de8((void**)&nd);                        \
                if (*(i32*)r != 0) {                                                               \
                    g_coordPool.Push((void*)(*(i32*)r));                                           \
                }                                                                                  \
            } while (nd != 0);                                                                     \
        }                                                                                          \
        ((CStepList*)&(g)->m_31c)->RemoveAll1b48a6();                                              \
    }

// Recompute the grid dirty rect (m_60) as the {0,0,w,h} box intersected with a copy
// of itself.
#define STEP_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        (RECT*)new (&ra) CRect(0, 0, (grid)->m_c, (grid)->m_10);                           \
        RECT* pb = (RECT*)new (&rb) CRect(0, 0, (grid)->m_c, (grid)->m_10);                \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect((RECT*)&(grid)->m_60, &ra, &rb)) {                                      \
            *(RECT*)&(grid)->m_60 = ra;                                                            \
        }                                                                                          \
        (grid)->m_70 = (grid)->m_68 - (grid)->m_60;                                                \
        (grid)->m_74 = (grid)->m_6c - (grid)->m_64;                                                \
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
i32 CStepMgr::Step33520(CGrunt* g) {
    i32 state = g->m_defenderState;
    if (state == 3) {
        return 1;
    }
    if (state != 2) {
        // ---- fresh: re-query the move grid ----
        CStepCoord tp;
        g->GetScreenPos((GruntTilePos*)&tp);
        CGrunt* nb = QueryTile4098(tp.x >> 5, tp.y >> 5, m_8c, m_90);
        if (nb != 0) {
            if (g->m_coordCount != 0) {
                STEP_DRAIN(g);
            }
            // board distance nb <-> g
            CStepCoord np, gp, np2, gp2;
            nb->GetScreenPos((GruntTilePos*)&np);
            g->GetScreenPos((GruntTilePos*)&gp);
            nb->GetScreenPos((GruntTilePos*)&np2);
            g->GetScreenPos((GruntTilePos*)&gp2);
            i32 dist = iabs((np2.y >> 5) - (gp2.y >> 5)) + iabs((np2.x >> 5) - (gp2.x >> 5));
            if (dist <= 0xa) {
                // dirty-rect box around the grunt
                CStepCoord b0, b1, b2, b3;
                g->GetScreenPos((GruntTilePos*)&b0);
                g->GetScreenPos((GruntTilePos*)&b1);
                g->GetScreenPos((GruntTilePos*)&b2);
                g->GetScreenPos((GruntTilePos*)&b3);
                CStepGrid* grid = m_c;
                RECT box;
                box.left = (b0.x >> 5) - 5;
                box.top = (b1.y >> 5) - 5;
                box.right = (b2.x >> 5) + 5;
                box.bottom = (b3.y >> 5) + 5;
                RECT gb;
                (RECT*)new (&gb) CRect(0, 0, grid->m_c, grid->m_10);
                if (!IntersectRect((RECT*)&grid->m_60, &box, &gb)) {
                    *(RECT*)&grid->m_60 = box;
                }
                grid->m_70 = grid->m_68 - grid->m_60;
                grid->m_74 = grid->m_6c - grid->m_64;
            }
            CStepCoord p;
            nb->GetScreenPos((GruntTilePos*)&p);
            if (CGrunt_TileSwitch(p.x >> 5, p.y >> 5, 0, 0x20000dc7, 0, 0)) {
                g->m_defenderState = 2;
                g->m_arrivalCol = nb->m_tileOwnerHi;
                g->m_arrivalRow = nb->m_tileOwnerLo;
                g->m_dwell = 0;
            }
            if (dist <= 0xa) {
                STEP_BOUNDS(m_c);
            }
        }
        goto tail;
    }

    // ---- state 2: in-flight advance ----
    {
        i32 col = g->m_arrivalCol;
        i32 row = g->m_arrivalRow;
        CGrunt* cur = m_8->m_grid[15 * col + row];
        if (cur == 0) {
            // clear path
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            g->m_defenderState = 0;
            GruntRecycleCoords(g);
            goto tail;
        }
        CStepSub10* s = (CStepSub10*)cur->m_10;
        if (g->RectContains(s->m_5c, s->m_60) != 0) {
            // arrived on this tile
            if (g->m_coordCount != 0) {
                STEP_DRAIN(g);
            }
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            if (g != 0 && g->IsAtSavedScreenPos() && g->m_entranceCommitted != 0
                && g->m_deathAnimStarted == 0 && g->m_entranceActive == 0 && g->m_poweredUp == 0) {
                const char* nm =
                    ((CTypeNode*)((zDArray*)&g_typeColl)->IndexToPtr(((CStepOwner*)g->m_14)->m_1c))
                        ->m_0;
                if (strcmp(nm, k_60cca0) != 0 && strcmp(nm, k_60cc9c) != 0
                    && strcmp(nm, k_60cc98) != 0 && strcmp(nm, k_60beb8) != 0
                    && strcmp(nm, k_60cc94) != 0 && strcmp(nm, k_60cc90) != 0
                    && strcmp(nm, k_60bebc) != 0) {
                    Finish3e4f(g, cur);
                }
            }
            g->m_defenderState = 0;
            goto tail;
        }
        // not arrived: reroute by Euclidean board distance
        CStepCoord here, np;
        g->GetTilePos((GruntTilePos*)&here);
        cur->GetTilePos((GruntTilePos*)&np);
        i32 dx = np.x - here.x;
        i32 dy = np.y - here.y;
        i32 dist = (i32)sqrt((double)(iabs(dx) * iabs(dx) + iabs(dy) * iabs(dy)));
        if (dist > m_a4) {
            if (m_f8 != 0) {
                CStepGoal* e = m_f4[rand() % m_f8];
                CGrunt_TileSwitch(e->m_0, e->m_4, 0, 0x983, 0, 0);
            }
            g->m_arrivalCol = -1;
            g->m_dwell = 0;
            g->m_arrivalRow = -1;
            g->m_defenderState = 0;
            if (g->m_coordCount != 0) {
                STEP_DRAIN(g);
            }
            g->m_dwell = 0;
            goto tail;
        }
        // dist <= m_a4: drain + recompute dirty rect + retarget
        if (g->m_coordCount != 0) {
            STEP_DRAIN(g);
        }
        CStepCoord c0, c1, c2, c3;
        cur->GetScreenPos((GruntTilePos*)&c0);
        g->GetScreenPos((GruntTilePos*)&c1);
        cur->GetScreenPos((GruntTilePos*)&c2);
        g->GetScreenPos((GruntTilePos*)&c3);
        i32 dist2 = iabs((c0.y >> 5) - (c1.y >> 5)) + iabs((c2.x >> 5) - (c3.x >> 5));
        if (dist2 <= 0xa) {
            CStepCoord d0, d1, d2, d3;
            g->GetScreenPos((GruntTilePos*)&d0);
            g->GetScreenPos((GruntTilePos*)&d1);
            g->GetScreenPos((GruntTilePos*)&d2);
            g->GetScreenPos((GruntTilePos*)&d3);
            CStepGrid* grid = m_c;
            RECT box;
            box.left = (d0.x >> 5) - 5;
            box.top = (d1.y >> 5) - 5;
            box.right = (d2.x >> 5) + 5;
            box.bottom = (d3.y >> 5) + 5;
            RECT gb;
            (RECT*)new (&gb) CRect(0, 0, grid->m_c, grid->m_10);
            if (!IntersectRect((RECT*)&grid->m_60, &box, &gb)) {
                *(RECT*)&grid->m_60 = box;
            }
            grid->m_70 = grid->m_68 - grid->m_60;
            grid->m_74 = grid->m_6c - grid->m_64;
        }
        CStepCoord cp;
        cur->GetScreenPos((GruntTilePos*)&cp);
        if (!CGrunt_TileSwitch(cp.x >> 5, cp.y >> 5, 0, 0x20000dc7, 0, 0)) {
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            g->m_defenderState = 0;
        }
        if (dist2 <= 0xa) {
            ((ApiMisc::ClipHost_02b340*)m_c)->Clip(0);
        }
        g->m_dwell = 0;
        goto tail;
    }

tail:
    if (ShouldStepGrunt(g)) {
        if (g->m_coordCount == 0 && (u32)g->m_dwell > (u32)m_a0 && m_f8 != 0) {
            CStepGoal* e = m_f4[rand() % m_f8];
            CGrunt_TileSwitch(e->m_0, e->m_4, 0, 0x983, 0, 0);
            g->m_dwell = 0;
        }
    }
    return 1;
}

SIZE_UNKNOWN(CStepBoard);
SIZE_UNKNOWN(CStepCoord);
SIZE_UNKNOWN(CStepGoal);
SIZE_UNKNOWN(CStepGrid);
SIZE_UNKNOWN(CStepMgr);
SIZE_UNKNOWN(CStepNode);
SIZE_UNKNOWN(CStepOwner);
SIZE_UNKNOWN(CStepRectInit);
SIZE_UNKNOWN(CStepSub10);
