#include <Mfc.h>
#include <Gruntz/Grunt.h>
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
#include <math.h> // sqrt (intrinsic fsqrt for the board-distance)
#include <Gruntz/FreeNodePool.h>

#pragma intrinsic(sqrt)

// --- views (offsets + called methods load-bearing; reloc-masked, no body) ---
struct CGruntSub10 { // grunt->m_10
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c, +0x60
};
struct CCoordXY {
    i32 x, y;
};
struct CMoveGridDims { // mover->m_c
    char _00[0xc];
    i32 m_c, m_10; // +0x0c grid width, +0x10 grid height
};

// The mover's board (m_8): a 4x15 grunt-pointer grid at +0x1c, indexed [15*col+row].
struct CMoveBoard {
    char _00[0x1c];
    CGrunt* m_grid[60]; // +0x1c
};

struct CGruntMover {                                             // this (edi)
    i32 Step(CGrunt* g);                                         // 0x031610
    CGrunt* QueryTile4098(i32 x, i32 y, i32 dx, i32 dy);         // 0x4098
    void Commit42e1(CGrunt* g);                                  // 0x42e1
    void Plan293c(CGrunt* g, i32 x, i32 y, i32 a, i32 b, i32 c); // 0x293c
    void Finish3e4f(CGrunt* g, CGrunt* a);                       // 0x3e4f

    char _00[0x8];
    CMoveBoard* m_8;    // +0x08  board (CGrunt*[] grid at +0x1c)
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
extern FreeNodePool g_coordPool;

#define MOVE_RECYCLE(g)                                                                            \
    {                                                                                              \
        GruntCoordNode* nd = (g)->m_320;                                                           \
        while (nd != 0) {                                                                          \
            GruntCoordNode* cur = nd;                                                              \
            nd = nd->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push((void*)(cur->m_coord));                                           \
            }                                                                                      \
        }                                                                                          \
        (g)->m_31c.RemoveAll();                                                                    \
    }

// @early-stop
// CRACKED 18%->72% (2026-07-05). The 18% park was STRUCTURAL, not a wall: my source
// laid the in-flight path first, but retail lays the FRESH path as the fall-through.
// Fixes applied, each verified against llvm-objdump -dr:
//   * block order: wrap fresh in `if(m_328==0){...}` so cl emits `jne handle328` and
//     falls into fresh (was `if(m_328!=0)goto` which cl inverted to fall into the short
//     handle328) - the single change that moved 18->69;
//   * board distance is real `(int)sqrt((double)(adx*adx+ady*ady))` inlined -> retail's
//     `fild [sum]; fsqrt; call __ftol` (the fake identity isqrt both mis-computed AND
//     dropped the [esp] spill that sizes the frame to 0x20);
//   * W/H read raw before GetTilePos, /3 divided after (deferred-division);
//   * `c0.x/c0.y/c1.x/c1.y >>= 5` in place (retail stores the shifted coords back);
//   * m_2ec vs m_b8/m_b4 are UNSIGNED compares (jbe, not jle).
// Residual ~28% is a genuine register-COLORING cascade: retail colors `this`(mover)->edi
// and `g`(arg)->esi; this cl colors `this`->ebx, freeing one reg so it spills/reloads
// fewer temps (base 352 insns vs retail 395 - retail re-materializes push-0/or-1 and
// reloads spills that this cl keeps in the extra reg). No source spelling reassigns the
// callee-saved `this` register. Final-sweep candidate.
RVA(0x00031610, 0x501)
i32 CGruntMover::Step(CGrunt* g) {
    if (g->m_coordCount == 0) {
        if (g->m_defenderState == 2) {
            goto inflight;
        }

        // ---- fresh: re-query the move grid for the target tile ----
        i32 W = m_c->m_c;
        i32 H = m_c->m_10;
        CCoordXY c0;
        g->GetScreenPos((GruntTilePos*)&c0);
        c0.x >>= 5;
        c0.y >>= 5;
        CGrunt* nb = QueryTile4098(c0.x, c0.y, (i32)((u32)W / 3), (i32)((u32)H / 3));
        if (nb != 0) {
            CCoordXY c1;
            nb->GetScreenPos((GruntTilePos*)&c1);
            c1.x >>= 5;
            c1.y >>= 5;
            if (CGrunt_TileSwitch(c1.x, c1.y, 0xd87, 0, 1, 0) == 0) {
                return 1;
            }
            g->m_arrivalCol = nb->m_tileOwnerHi;
            g->m_arrivalRow = nb->m_tileOwnerLo;
            g->m_defenderState = 2;
            g->m_dwell = 0;
            Commit42e1(g);
            return 1;
        }
        // nb == 0: replan / drain
        if ((u32)g->m_dwell > (u32)m_b8) {
            CCoordXY here;
            g->GetScreenPos((GruntTilePos*)&here);
            Plan293c(g, here.x >> 5, here.y >> 5, m_ac, m_b0, -1);
            if (g->m_coordCount > m_98 + m_94 && g->m_coordCount != 0) {
                GruntCoordNode* nd = g->m_320;
                if (nd != 0) {
                    do {
                        void* r = g->m_31c.Find1de8((void**)&nd);
                        if (*(i32*)r != 0) {
                            g_coordPool.Push((void*)(*(i32*)r));
                        }
                    } while (nd != 0);
                }
                g->m_31c.RemoveAll();
            }
            g->m_dwell = 0;
        }
        return 1;
    }

    // m_328 != 0
    if (g->m_defenderState != 2) {
        return 1;
    }
inflight: {
    // ---- in-flight: advance / reroute along the path ----
    i32 col = g->m_arrivalCol;
    i32 row = g->m_arrivalRow;
    CGrunt* cur = m_8->m_grid[15 * col + row];
    i32 W = m_c->m_c;
    i32 H = m_c->m_10;
    CCoordXY c0;
    g->GetScreenPos((GruntTilePos*)&c0);
    c0.x >>= 5;
    c0.y >>= 5;
    CGrunt* nb = QueryTile4098(c0.x, c0.y, (i32)((u32)W / 3), (i32)((u32)H / 3));

    if (cur == 0) {
        goto L_clear;
    }
    if (nb != 0 && cur != nb) {
        if (g->m_coordCount != 0) {
            MOVE_RECYCLE(g);
        }
        g->m_arrivalCol = nb->m_tileOwnerHi;
        g->m_arrivalRow = nb->m_tileOwnerLo;
        g->m_defenderState = 2;
        g->m_dwell = 0;
        {
            CGruntSub10* s = (CGruntSub10*)nb->m_10;
            if (CGrunt_TileSwitch(s->m_5c >> 5, s->m_60 >> 5, 0xd87, 0, 0, 0) == 0) {
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
        CGruntSub10* s = (CGruntSub10*)cur->m_10;
        if (g->RectContains(s->m_5c, s->m_60) != 0) {
            // arrived on this tile: latch the move
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            Finish3e4f(g, cur);
            g->m_defenderState = 0;
            return 1;
        }
    }
    // 3198f: not arrived - reroute by board distance
    if ((u32)g->m_dwell <= (u32)m_b4) {
        return 1;
    }
    {
        CCoordXY here;
        g->GetScreenPos((GruntTilePos*)&here);
        i32 x5 = here.x >> 5;
        i32 y5 = here.y >> 5;
        CCoordXY nbpos;
        cur->GetTilePos((GruntTilePos*)&nbpos);
        i32 dx = nbpos.x - x5;
        i32 dy = nbpos.y - y5;
        i32 adx = dx < 0 ? -dx : dx;
        i32 ady = dy < 0 ? -dy : dy;
        i32 dist = (i32)sqrt((double)(adx * adx + ady * ady));
        if (dist > m_c0) {
            if (g->m_coordCount != 0) {
                MOVE_RECYCLE(g);
            }
            goto L_clearAt;
        }
        if (g->m_coordCount != 0) {
            MOVE_RECYCLE(g);
        }
        CGruntSub10* s = (CGruntSub10*)cur->m_10;
        if (CGrunt_TileSwitch(s->m_5c >> 5, s->m_60 >> 5, 0xd87, 0, 0, 0) != 0) {
            g->m_dwell = 0;
            return 1;
        }
    }
L_clearAt:
    g->m_arrivalCol = -1;
    g->m_arrivalRow = -1;
    g->m_defenderState = 0;
    g->m_dwell = 0;
    return 1;

L_clear:
    g->m_arrivalCol = -1;
    g->m_defenderState = 0;
    g->m_arrivalRow = -1;
    return 1;
}
}

SIZE_UNKNOWN(CCoordXY);
SIZE_UNKNOWN(CGruntMover);
SIZE_UNKNOWN(CGruntSub10);
SIZE_UNKNOWN(CMoveBoard);
SIZE_UNKNOWN(CMoveGridDims);
SIZE_UNKNOWN(GruntCoordNode);
SIZE_UNKNOWN(GruntListSub);
