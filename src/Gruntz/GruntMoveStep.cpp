// GruntMoveStep.cpp - CGruntMover::Step (0x031610, __thiscall ret 4). The per-tick
// grunt move-resolution step: depending on the grunt's arrival state (m_2d4) and the
// pending-coord flag (m_328) it either (a) re-queries the move grid for the target
// tile, probes it, and commits the arrival, (b) advances along the in-flight path
// (m_2d4==2), rerouting around a blocking neighbour by board-distance, or (c) drains
// the pending-coord list (g_coordPool recycle + m_31c RemoveAll) and clears the
// arrival latches. `this` (edi) is the move-grid helper; the argument (esi) is the
// CGrunt being moved (and its neighbours).
//
// Large (0x501 B, ~15 internal calls); modeled with offset-faithful local views.
// The recognised idioms - the /3 grid-dim divides, the >>5 tile coords, the
// g_coordPool recycle loops, the abs()+sqrt board-distance - are byte-shaped; the
// internal-helper signatures (QueryTile/Probe/Plan/Check/Commit/Finish) are best-
// read. Real home is the CGrunt path/move family; a final-sweep re-home is harmless.
#include <rva.h>

#include <Ints.h>
#include <Gruntz/StepList2.h> // the shared g_coordPool recycle pool

// --- views (offsets + called methods load-bearing; reloc-masked, no body) ---
struct CGruntSub10 { // grunt->m_10
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c, +0x60
};
struct CMoveListNode {     // pending-coord node
    CMoveListNode* m_next; // +0x00
    i32 _04;
    i32 m_8; // +0x08  recycled coord-node handle (fed to g_coordPool.Drop)
};
struct CMoveObList {           // CGrunt::m_31c (CObList view)
    void* Find1de8(void** it); // 0x1de8
    void RemoveAll1b48a6();    // 0x1b48a6
    char _00[4];
};
struct CCoordXY {
    i32 x, y;
};
struct CGruntM {                                             // the grunt (esi) + its neighbours
    void GetTilePos36c0(CCoordXY* out);                      // 0x36c0
    i32 Probe1640(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x1640
    i32 Check3c4c(i32 a, i32 b);                             // 0x3c4c
    void Probe3143(CCoordXY* out);                           // 0x3143

    char _00[0x10];
    CGruntSub10* m_10; // +0x10
    char _14[0x1ec - 0x14];
    i32 m_1ec, m_1f0; // +0x1ec, +0x1f0
    char _1f4[0x2d4 - 0x1f4];
    i32 m_2d4; // +0x2d4
    char _2d8[0x2ec - 0x2d8];
    i32 m_2ec, m_2f0, m_2f4; // +0x2ec..+0x2f4
    char _2f8[0x31c - 0x2f8];
    CMoveObList m_31c;    // +0x31c
    CMoveListNode* m_320; // +0x320
    char _324[0x328 - 0x324];
    i32 m_328; // +0x328
};

struct CMoveGridDims { // mover->m_c
    char _00[0xc];
    i32 m_c, m_10; // +0x0c grid width, +0x10 grid height
};

// The mover's board (m_8): a 4x15 grunt-pointer grid at +0x1c, indexed [15*col+row].
struct CMoveBoard {
    char _00[0x1c];
    CGruntM* m_grid[60]; // +0x1c
};

struct CGruntMover {                                              // this (edi)
    i32 Step(CGruntM* g);                                         // 0x031610
    CGruntM* QueryTile4098(i32 x, i32 y, i32 dx, i32 dy);         // 0x4098
    void Commit42e1(CGruntM* g);                                  // 0x42e1
    void Plan293c(CGruntM* g, i32 x, i32 y, i32 a, i32 b, i32 c); // 0x293c
    void Finish3e4f(CGruntM* g, CGruntM* a);                      // 0x3e4f

    char _00[0x8];
    CMoveBoard* m_8;    // +0x08  board (CGruntM*[] grid at +0x1c)
    CMoveGridDims* m_c; // +0x0c
    char _10[0x94 - 0x10];
    i32 m_94, m_98; // +0x94, +0x98
    char _9c[0xac - 0x9c];
    i32 m_ac, m_b0, m_b4, m_b8; // +0xac..+0xb8
    char _bc[0xc0 - 0xbc];
    i32 m_c0; // +0xc0
};

// The shared coord-node pool (0x645540): Drop returns a node to the pool.
DATA(0x00245540)
extern CStepList2 g_coordPool;

// Integer board-distance: the retail form is `fild;fsqrt;call __ftol`; modeled as a
// plain int sqrt here (no /Oi) - see the @early-stop note.
static i32 isqrt(i32 v) {
    return (i32)(double)v;
}

#define MOVE_RECYCLE(g)                                                                            \
    {                                                                                              \
        CMoveListNode* nd = (g)->m_320;                                                            \
        while (nd != 0) {                                                                          \
            CMoveListNode* cur = nd;                                                               \
            nd = nd->m_next;                                                                       \
            if (cur->m_8 != 0) {                                                                   \
                g_coordPool.Drop(cur->m_8);                                                        \
            }                                                                                      \
        }                                                                                          \
        (g)->m_31c.RemoveAll1b48a6();                                                              \
    }

// @early-stop
// large movement-step reconstruction (final-sweep candidate): the m_2d4/m_328 state
// branch, the /3 grid-dim divides, the >>5 tile coords, the QueryTile/Probe/Plan/
// Commit/Finish dispatch, the g_coordPool recycle loops + m_31c RemoveAll and the
// abs()+board-distance reroute are the correct shape. Residual: the inline fsqrt/
// __ftol integer distance (modeled as a placeholder int sqrt without /Oi), several
// best-read internal-helper signatures, and the heavy shared-landing-pad regalloc -
// re-attack leaf-first in the final sweep.
RVA(0x00031610, 0x501)
i32 CGruntMover::Step(CGruntM* g) {
    if (g->m_328 != 0 && g->m_2d4 != 2) {
        return 1;
    }

    if (g->m_2d4 == 2) {
        // ---- in-flight: advance / reroute along the path ----
        i32 col = g->m_2f0;
        i32 row = g->m_2f4;
        CGruntM* cur = m_8->m_grid[15 * col + row];
        i32 dimX3 = (i32)((u32)m_c->m_c / 3);
        i32 dimY3 = (i32)((u32)m_c->m_10 / 3);
        CCoordXY c0;
        g->GetTilePos36c0(&c0);
        CGruntM* nb = QueryTile4098(c0.x >> 5, c0.y >> 5, dimX3, dimY3);

        if (cur == 0) {
            goto L_clear;
        }
        if (nb != 0 && cur != nb) {
            if (g->m_328 != 0) {
                MOVE_RECYCLE(g);
            }
            g->m_2f0 = nb->m_1ec;
            g->m_2f4 = nb->m_1f0;
            g->m_2d4 = 2;
            g->m_2ec = 0;
            {
                CGruntSub10* s = nb->m_10;
                if (g->Probe1640(s->m_5c >> 5, s->m_60 >> 5, 0xd87, 0, 0, 0) == 0) {
                    return 1;
                }
            }
            cur = nb; // loc34
        }
        // L_900
        if (cur == 0) {
            goto L_clear;
        }
        {
            CGruntSub10* s = cur->m_10;
            if (g->Check3c4c(s->m_5c, s->m_60) != 0) {
                // arrived on this tile: latch the move
                g->m_2f0 = -1;
                g->m_2f4 = -1;
                Finish3e4f(g, cur);
                g->m_2d4 = 0;
                return 1;
            }
        }
        // 3198f: not arrived - reroute by board distance
        if (g->m_2ec <= m_b4) {
            return 1;
        }
        {
            CCoordXY here;
            g->GetTilePos36c0(&here);
            i32 x5 = here.x >> 5;
            i32 y5 = here.y >> 5;
            CCoordXY nbpos;
            cur->Probe3143(&nbpos);
            i32 dx = nbpos.x - x5;
            i32 dy = nbpos.y - y5;
            i32 adx = dx < 0 ? -dx : dx;
            i32 ady = dy < 0 ? -dy : dy;
            i32 dist = isqrt(adx * adx + ady * ady);
            if (dist > m_c0) {
                if (g->m_328 != 0) {
                    MOVE_RECYCLE(g);
                }
                goto L_clearAt;
            }
            if (g->m_328 != 0) {
                MOVE_RECYCLE(g);
            }
            CGruntSub10* s = cur->m_10;
            if (g->Probe1640(s->m_5c >> 5, s->m_60 >> 5, 0xd87, 0, 0, 0) != 0) {
                g->m_2ec = 0;
                return 1;
            }
        }
    L_clearAt:
        g->m_2f0 = -1;
        g->m_2f4 = -1;
        g->m_2d4 = 0;
        g->m_2ec = 0;
        return 1;

    L_clear:
        g->m_2f0 = -1;
        g->m_2d4 = 0;
        g->m_2f4 = -1;
        return 1;
    }

    // ---- fresh: re-query the move grid for the target tile ----
    {
        i32 dimX3 = (i32)((u32)m_c->m_c / 3);
        i32 dimY3 = (i32)((u32)m_c->m_10 / 3);
        CCoordXY c0;
        g->GetTilePos36c0(&c0);
        CGruntM* nb = QueryTile4098(c0.x >> 5, c0.y >> 5, dimX3, dimY3);
        if (nb != 0) {
            CCoordXY c1;
            nb->GetTilePos36c0(&c1);
            if (g->Probe1640(c1.x >> 5, c1.y >> 5, 0xd87, 0, 1, 0) == 0) {
                return 1;
            }
            g->m_2f0 = nb->m_1ec;
            g->m_2f4 = nb->m_1f0;
            g->m_2d4 = 2;
            g->m_2ec = 0;
            Commit42e1(g);
            return 1;
        }
        // nb == 0: replan / drain
        if (g->m_2ec > m_b8) {
            CCoordXY here;
            g->GetTilePos36c0(&here);
            Plan293c(g, here.x >> 5, here.y >> 5, m_ac, m_b0, -1);
            if (g->m_328 > m_98 + m_94 && g->m_328 != 0) {
                CMoveListNode* nd = g->m_320;
                if (nd != 0) {
                    do {
                        void* r = g->m_31c.Find1de8((void**)&nd);
                        if (*(i32*)r != 0) {
                            g_coordPool.Drop(*(i32*)r);
                        }
                    } while (nd != 0);
                }
                g->m_31c.RemoveAll1b48a6();
            }
            g->m_2ec = 0;
        }
        return 1;
    }
}

SIZE_UNKNOWN(CCoordXY);
SIZE_UNKNOWN(CGruntM);
SIZE_UNKNOWN(CGruntMover);
SIZE_UNKNOWN(CGruntSub10);
SIZE_UNKNOWN(CMoveBoard);
SIZE_UNKNOWN(CMoveGridDims);
SIZE_UNKNOWN(CMoveListNode);
SIZE_UNKNOWN(CMoveObList);
