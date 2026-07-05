// GruntArriveResolve.cpp - CArriveMgr::Resolve2c690 (0x02c690, __thiscall ret 4, /GX).
// The grunt arrival/tile-effect resolver: `this` (edi) is the grunt manager (grid at
// m_c); the argument (ebp) is the CGrunt whose head pending-coord just arrived. After
// a gate (Method1a14) and the pending-coord latch (g->m_328) it copies the
// destination tile cell + the grunt's own cell into stack buffers, then dispatches on
// the cell flags (0x4 door / 0x8000 / 0x4000 gate / 0x2 / 0x8 / 0x20 / 0x40) and the
// grunt type (m_170/m_19c) into the door-open (CString type gate, g_freeList recycle),
// tile-trigger (0x14bf), teleport (0x2838) and neighbour-pick (random) handlers,
// recycling the pending-coord list onto g_coordPool / g_freeList on each exit. The
// stack CString (block 10, ctor 0x1b4867 / dtor 0x1b48c6) makes MSVC emit the /GX EH
// frame. Big body (0xdb4); shares the CGrunt coord-pool / grid family.
#include <rva.h>

#include <Ints.h>
#include <Win32.h>            // RECT + IntersectRect
#include <string.h>           // memset (out-of-bounds cell fill)
#include <Gruntz/StepList2.h> // the shared g_coordPool recycle pool

extern void* g_freeList;       // ?g_freeList@@3PAXA (0x645544)
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA (0x64554c)

extern CStepList2 g_coordPool; // ?g_coordPool@@3UCoordPool@@A (0x645540): Drop recycles a node

// --- offset-faithful views (offsets + called methods load-bearing; reloc-masked) ---
struct CArriveCoord {
    i32 x, y;
};
struct CArriveNode {     // g->m_320 node
    CArriveNode* m_next; // +0x00
    i32 _04;
    CArriveCoord* m_8; // +0x08 coord
};
struct CArriveCell { // 0x1c bytes/cell
    i32 m_0;         // +0x00 flags
    char _04[0x10 - 4];
    i32 m_10; // +0x10 type
    char _14[0x1c - 0x14];
};
struct CArriveSub10 { // g->m_10
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c, +0x60
};
struct CArriveList {           // g->m_31c
    void RemoveAll1b48a6();    // 0x1b48a6
    void* Find1de8(void** it); // 0x1de8
    char _00[4];
};
struct CArriveStr {     // scratch CString (block 10) - forces /GX
    CArriveStr(i32 n);  // 0x1b4867
    ~CArriveStr();      // 0x1b48c6
    void* Head1b4a03(); // 0x1b4a03
    char _00[0x18];
};
struct CArriveGrunt {                                            // g (ebp)
    void GetTilePos36c0(CArriveCoord* out);                      // 0x36c0
    i32 Move14bf(i32 col, i32 row, i32 a, i32 b);                // 0x14bf
    i32 Trigger1640(i32 a, i32 b, i32 c, i32 msg, i32 e, i32 f); // 0x1640
    i32 Probe1a4b(i32 a, i32 b, i32 c);                          // 0x1a4b
    void Effect374c(i32 kind, i32 x);                            // 0x374c
    i32 State1c21(i32 a, i32 b);                                 // 0x1c21 (via [g+0x14])
    i32 Check3c4c(i32 a, i32 b);                                 // 0x3c4c
    void Impact25e5(i32 a, i32 b, i32 c, i32 d);                 // 0x25e5
    void SelfImpact2b58(i32 a, i32 b, i32 c, i32 d);             // 0x2b58
    i32 Ready27ed(CArriveGrunt* g);                              // 0x27ed
    char _00[0x10];
    CArriveSub10* m_10; // +0x10
    char _14[0x170 - 0x14];
    i32 m_170; // +0x170 type
    i32 m_174, m_178;
    char _17c[0x19c - 0x17c];
    i32 m_19c; // +0x19c alt type
    char _1a0[0x1ec - 0x1a0];
    i32 m_1ec, m_1f0; // +0x1ec, +0x1f0
    char _1f4[0x2d4 - 0x1f4];
    i32 m_2d4; // +0x2d4 state
    i32 m_2d8; // +0x2d8 state2
    char _2dc[0x2ec - 0x2dc];
    i32 m_2ec; // +0x2ec
    char _2f0[0x31c - 0x2f0];
    CArriveList m_31c;  // +0x31c
    CArriveNode* m_320; // +0x320
    char _324[0x328 - 0x324];
    i32 m_328; // +0x328 pending latch
};

struct CArriveGrid { // this->m_c
    char _00[8];
    CArriveCell** m_8; // +0x08 rows
    i32 m_c, m_10;     // +0x0c width, +0x10 height
};
struct CArriveMgr { // this (edi)
    i32 Gate1a14(); // 0x1a14
    char _00[0xc];
    CArriveGrid* m_c; // +0x0c grid
    i32 Resolve2c690(CArriveGrunt* g);
};

// Recycle the grunt's pending-coord list onto g_coordPool via the CObList Find walk.
#define ARR_RECYCLE(g)                                                                             \
    {                                                                                              \
        CArriveNode* nd = (g)->m_320;                                                              \
        while (nd != 0) {                                                                          \
            CArriveNode* cur = nd;                                                                 \
            nd = nd->m_next;                                                                       \
            if (cur->m_8 != 0) {                                                                   \
                g_coordPool.Drop((i32)cur->m_8);                                                   \
            }                                                                                      \
        }                                                                                          \
        (g)->m_31c.RemoveAll1b48a6();                                                              \
    }

// @early-stop
// large grunt arrival/tile-effect resolver reconstruction (final-sweep candidate).
// 2026-07-05: fixed a structural placeholder - the OWN cell copy was wrongly reusing
// the dest coords (m_8[gy][gx]); retail indexes it by the head-coord fcx on both axes
// (m_8[fcx][fcx], bounds fcx<width && fcx<height, verified vs llvm-objdump -dr). Now
// the Gate1a14 + m_328 latch, BOTH cell copies (rep-movs 7-dword / out-of-bounds
// 0x01010101 rep-stos fill), the door(0x4)/gate(0x4000/0x8000)/0x2/0x8/0x20/0x40 flag +
// grunt-type dispatch into the Move14bf / Trigger1640 / teleport / neighbour handlers,
// the g_coordPool + g_freeList recycles and the /GX CString gate are correct in shape.
// Residual walls: the coalesced GetTilePos/cell-copy stack-slot schedule, the per-branch
// EH-state stamps and the heavy callee-saved regalloc (this->edi / g->ebp) diverge from
// retail - re-attack leaf-first in the final sweep.
RVA(0x0002c690, 0xdb4)
i32 CArriveMgr::Resolve2c690(CArriveGrunt* g) {
    if (Gate1a14()) {
        return 1;
    }
    if (g->m_328 == 0) {
        return 0;
    }

    CArriveGrid* grid = m_c;
    CArriveCoord* fc = g->m_320->m_8;
    i32 fcx = fc->x; // grunt head coord x (long-lived)

    CArriveCoord tp;
    g->GetTilePos36c0(&tp);
    i32 gx = tp.x >> 5;
    i32 gy = tp.y >> 5;

    // destination cell (the grunt's tile) copied into a local buffer
    CArriveCell dest;
    CArriveCell fill;
    CArriveCell* dsrc;
    if ((u32)gx < (u32)grid->m_c && (u32)gy < (u32)grid->m_10) {
        dsrc = &grid->m_8[gy][gx];
    } else {
        memset(&fill, 1, 0x1c);
        dsrc = &fill;
    }
    dest = *dsrc;

    // the grunt's own cell: retail indexes it by the head-coord fcx on BOTH axes
    // (m_8[fcx][fcx], in-bounds iff fcx<width && fcx<height); OOB fills 0x01010101
    // (rep stosl). (llvm-objdump -dr 0xfe/0x10b: cmp width,fcx / cmp height,fcx; the
    // row load and the *7 column offset both use fcx.)
    CArriveCell own;
    CArriveCell fill2;
    CArriveCell* osrc;
    if ((u32)fcx < (u32)grid->m_c && (u32)fcx < (u32)grid->m_10) {
        osrc = &grid->m_8[fcx][fcx];
    } else {
        memset(&fill2, 1, 0x1c);
        osrc = &fill2;
    }
    own = *osrc;
    i32 flags = own.m_0;

    i32 maskFlags = flags & 0xdfffffff;
    i32 type = (g->m_170 > 0x16) ? g->m_19c : g->m_170;

    // ---- door (0x4 high byte) ----
    if ((dest.m_0 & 0x4000000) && g->m_2d4 == 3) {
        i32 t2 = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t2 != 8 && (own.m_0 & 0x4000)) {
            // open-door corner transform + type gate (CString), g_freeList recycle
            RECT ra, rb;
            ((CArriveStr*)&ra)->Head1b4a03();
            CArriveStr cs(0xa);
            IntersectRect(&rb, &ra, &ra);
            void* elem = cs.Head1b4a03();
            if (elem != 0) {
                void** node = (void**)((char*)elem - g_freeListNodeBias);
                *node = g_freeList;
                g_freeList = node;
            }
            return 1;
        }
    }

    // ---- door body flag (0x4 low byte) ----
    if ((dest.m_0 & 0x4) && g->m_2d8 != 0xb) {
        if (g->State1c21(0, 0) == 2) {
            g->m_2d4 = 0;
            ARR_RECYCLE(g);
            g->m_2d8 = 0xb;
            g->m_2ec = 0;
            return 0;
        }
    }

    // ---- 0x8000 gate, arriving type 3 ----
    if ((maskFlags & 0x8000) && type == 3 && g->m_2d8 == 0xa) {
        g->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (gx << 5) + 0x10);
        ARR_RECYCLE(g);
        return 0;
    }

    // ---- 0x4000 gate, arriving type 3 ----
    if ((maskFlags & 0x4000) && type == 3 && g->m_2d8 == 0xa) {
        if (grid->m_8[gy][gx].m_10 != 0x99) {
            g->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (gx << 5) + 0x10);
        }
        ARR_RECYCLE(g);
        return 0;
    }

    // ---- 0x2 -> done ----
    if (maskFlags & 0x200) {
        return 1;
    }

    // ---- 0x8 -> probe/effect ----
    if (maskFlags & 0x8) {
        if (g->Probe1a4b(fcx, gx, gy) != 0) {
            return 1;
        }
        g->Effect374c(0x12, gx);
    }

    // ---- 0x20 -> type dispatch ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 1 || t == 0x11) {
            g->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (gx << 5) + 0x10);
            return 1;
        }
    }

    // ---- 0x8000 secondary / 0x40 / neighbour fallback ----
    if (maskFlags & 0x8000) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 0xf) {
            return 1;
        }
    }

    if (maskFlags & 0x40) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 0x16 || t == 0xd) {
            g->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (gx << 5) + 0x10);
            return 0;
        }
        g->Effect374c(0xd, gx);
        return 0;
    }

    // ---- neighbour pick fallback ----
    g->SelfImpact2b58(0, 0, 0, 0);
    if (g->Ready27ed(g) != 0) {
        return 1;
    }
    return 1;
}

SIZE_UNKNOWN(CArriveCell);
SIZE_UNKNOWN(CArriveCoord);
SIZE_UNKNOWN(CArriveGrid);
SIZE_UNKNOWN(CArriveGrunt);
SIZE_UNKNOWN(CArriveList);
SIZE_UNKNOWN(CArriveMgr);
SIZE_UNKNOWN(CArriveNode);
SIZE_UNKNOWN(CArriveStr);
SIZE_UNKNOWN(CArriveSub10);
