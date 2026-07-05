// GruntArriveResolve.cpp - CArriveMgr::Resolve2c690 (0x02c690, __thiscall ret 4, /GX).
// The grunt arrival/tile-effect resolver. `this` (the board/logic object, kept spilled)
// IS g->m_10 - the caller does g->m_10->Resolve2c690(g); grid = this->m_c, the origin is
// g->m_10->m_5c/m_60, the physics-call owners are this->m_8 (Move14bf) / this->m_14
// (Find/FindByField0C) / this itself (Effect/Probe/Impact/Ready). The argument (ebp) is
// the CGrunt whose head pending-coord just arrived.
//
// After a gate (Gate1a14) and the pending-coord latch (g->m_328), it copies the
// destination tile cell (grid[gy][gx] from GetTilePos>>5) and the grunt's own cell
// (grid[coord->y][coord->x], the head pending coordinate) into stack buffers, masks the
// own-cell flags (& 0xdfffffff) and computes type = (m_170>0x16 ? m_19c : m_170), then
// dispatches on dest/own flags + type into: door-open transform (CString/CObList +
// g_freeList recycle), tile-trigger (Move14bf), teleport (FindByField0C 0x2838),
// neighbour-pick (rand%3) handlers - recycling the pending-coord list onto
// g_coordPool / RemoveAll on each exit. The stack CString (block, ctor 0x1b4867 / dtor
// 0x1b48c6) forces the /GX EH frame. Big body (0xdb4, frame 0x94).
#include <rva.h>

#include <Ints.h>
#include <Win32.h>            // RECT + IntersectRect
#include <stdlib.h>           // rand
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
    i32 m_8; // +0x08 coord ptr (head) / recycled coord-node handle (fed to g_coordPool.Drop)
};
struct CArriveCell { // 0x1c bytes/cell
    i32 m_0;         // +0x00 flags
    char _04[0x10 - 4];
    i32 m_10; // +0x10 type
    char _14[0x1c - 0x14];
};
struct CArriveQuad {                                  // QuadIntRecord (0x34a4) target
    i32 left, top, right, bottom;                     // +0x00..+0x0c
    CArriveQuad* Set34a4(i32 a, i32 b, i32 c, i32 d); // 0x34a4 (__thiscall, returns this)
};
struct CArriveGrid { // this->m_c
    char _00[8];
    CArriveCell** m_8; // +0x08 rows
    i32 m_c, m_10;     // +0x0c width, +0x10 height
    char _14[0x60 - 0x14];
    CArriveQuad m_60; // +0x60 clip region (left/top/right/bottom)
    i32 m_70, m_74;   // +0x70 width, +0x74 height of clip
};
struct CArriveFind { // FindByField0C (0x2838) result
    i32 m_0;         // +0x00
    char _04[0x18 - 4];
    i32 m_arr[1]; // +0x18 indexed by this->m_18
};
struct CArriveFind2 { // Find1c21 result
    i32 _00;
    i32 m_4; // +0x04
};
struct CArriveCellSetter {                     // this->m_10->m_2e4
    void SetCell2f45(i32 col, i32 row, i32 v); // 0x2f45
};
struct CArriveSub10b { // this->m_10
    char _00[0x2e4];
    CArriveCellSetter* m_2e4; // +0x2e4
};
struct CArriveList {           // g->m_31c (CObList) - spans +0x31c..+0x328
    void RemoveAll1b48a6();    // 0x1b48a6
    void AddTail1b4991(i32 v); // 0x1b4991
    i32 _00;                   // +0x00 (CObList head @ g+0x31c)
    CArriveNode* m_320;        // +0x04 (g+0x320)
    CArriveNode* m_324;        // +0x08 (g+0x324)
    i32 m_328;                 // +0x0c (g+0x328) pending latch
};
struct CArriveStr {     // scratch CString/CObList (block) - forces /GX
    CArriveStr(i32 n);  // 0x1b4867
    ~CArriveStr();      // 0x1b48c6
    void* Head1b4a03(); // 0x1b4a03
    char _00[0x18];
};
struct CArriveGrunt;
struct CArriveMover {                          // this->m_8
    void Move14bf(i32 a, i32 b, i32 x, i32 y); // 0x14bf
};
struct CArriveFinder {                       // this->m_14
    CArriveFind2* Find1c21(i32 key, i32 n);  // 0x1c21
    CArriveFind* FindByField0C2838(i32 key); // 0x2838
};
struct CArriveMgr {                                            // this (== g->m_10)
    i32 Gate1a14(CArriveGrunt* g);                             // 0x1a14
    void Effect374c(CArriveGrunt* g, i32 kind);                // 0x374c
    i32 Probe1a4b(CArriveGrunt* g, i32 a, i32 b);              // 0x1a4b
    void Impact25e5(CArriveGrunt* g, i32 a, i32 b, i32 c);     // 0x25e5
    void SelfImpact2b58(CArriveGrunt* g, i32 a, i32 b, i32 c); // 0x2b58
    i32 Ready27ed(CArriveGrunt* g);                            // 0x27ed

    char _00[8];
    CArriveMover* m_8;   // +0x08
    CArriveGrid* m_c;    // +0x0c grid
    CArriveSub10b* m_10; // +0x10
    CArriveFinder* m_14; // +0x14
    i32 m_18;            // +0x18 index
    char _1c[0x5c - 0x1c];
    i32 m_5c, m_60; // +0x5c, +0x60 origin coords (raw px)

    i32 Resolve2c690(CArriveGrunt* g);
};
struct CArriveGrunt {                                          // g (ebp)
    void GetTilePos36c0(CArriveCoord* out);                    // 0x36c0
    i32 Trigger1640(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x1640
    i32 Check3c4c(i32 a, i32 b);                               // 0x3c4c

    char _00[0x10];
    CArriveMgr* m_10; // +0x10  (== this)
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
    CArriveList m_31c; // +0x31c..+0x328 (list head + m_320/m_324/m_328)
};

// Recycle the grunt's pending-coord list onto g_coordPool (guarded by m_328).
#define ARR_RECYCLE(list)                                                                          \
    if ((list)->m_328 != 0) {                                                                      \
        CArriveNode* nd = (list)->m_320;                                                           \
        while (nd != 0) {                                                                          \
            CArriveNode* cur = nd;                                                                 \
            nd = nd->m_next;                                                                       \
            if (cur->m_8 != 0) {                                                                   \
                g_coordPool.Drop(cur->m_8);                                                        \
            }                                                                                      \
        }                                                                                          \
        (list)->RemoveAll1b48a6();                                                                 \
    }

// cell flags at (col,row), out-of-bounds -> 0x01010101.
static __inline i32 arrCell(CArriveGrid* grid, i32 col, i32 row) {
    if ((u32)col < (u32)grid->m_c && (u32)row < (u32)grid->m_10) {
        return grid->m_8[row][col].m_0;
    }
    return 1;
}

// @early-stop
// RECONSTRUCTED 23.7%->54.8% (2026-07-05). The whole COMMON path is now complete + logic-
// correct (verified vs `sema disasm --diff`): Gate1a14(g) gate; the list-cached m_328 latch
// (esi=&g->m_31c, read [esi+0xc]); the double-GetTilePos dest cell grid[gy][bx] AND the own
// cell grid[fcy][fcx] (fcy = coord->y, was wrongly fcx twice) with the 0x01010101 OOB self-
// fill; maskFlags = own.m_0 & ~0x20000000; type = (m_170>0x16?m_19c:m_170); and ALL the
// flag/type handlers in retail order - door(0x400)/doorbody(0x4,FindChild==2)/0x8000-t3/
// 0x4000-t3(!=0x99)/0x200/0x8(Probe+Effect0x12)/0x20(t1 Move, t0x11 3x3 scan)/0x4000-tf/
// 0x8000-tf(FindByField0C+Impact/Move)/0x20-t5(SetCell/Impact/Move)/0x40(td Move / Effect0xd)/
// the neighbour-pick fallback (SelfImpact+Ready, rand()%3 origin scan + Trigger1640). Every
// callee owner corrected: this==g->m_10 (grid=this->m_c, Move via this->m_8, Find via this->
// m_14, Effect/Probe/Impact/Ready on this; origin g->m_10->m_5c/m_60). The CArriveMgr view had
// a missing char _00[8] (all this-> offsets were 8 low) - fixed.
//
// Residual is TWO walls, both proven with `sema disasm --base/--target`:
//   (1) REGALLOC SWAP (dominant, ~unclimbable): retail colours g->ebp and SPILLS this to
//       [esp+0x10]; our MSVC5 colours this->ebp and g->edi. Identical instruction stream +
//       logic, but every g-member/this-member ref uses the opposite base register (ebp<->edi),
//       so the modrm bytes differ throughout. Same allocator wall documented for
//       CGruntMover::Step (GruntMoveStep.cpp) - "no source spelling reassigns the callee-saved
//       register". Routing the board through g->m_10 DOES flip g->ebp but adds an [ebp+0x10]
//       reload per cluster (base grows), netting -0.8% - not worth it.
//   (2) DOOR-OPEN transform (off the common path, own recheck-DCE wall): retail's 0x1ad..0x46e
//       block is 3x GetTilePos + QuadIntRecord + IntersectRect + a per-cell CString-EH nested
//       loop (SearchEdge 0x20f4 / RemoveHead / g_freeList push) + a 0x2d31b cleanup tail. It
//       contains a dead stack-address null-recheck (`lea edx,[esp+0x38]; test edx,edx; je`) our
//       stronger MSVC5 DCE eliminates (cf. docs/patterns/dead-unreachable-recheck-block-dce.md),
//       so it can't reach byte-exact regardless. Reconstructed as a structural CString-EH loop
//       (forces the /GX frame); the exact 700-B transform + tail are a dedicated final-sweep job.
RVA(0x0002c690, 0xdb4)
i32 CArriveMgr::Resolve2c690(CArriveGrunt* g) {
    CArriveList* list = &g->m_31c;
    if (Gate1a14(g)) {
        return 1;
    }
    if (list->m_328 == 0) {
        return 0;
    }

    CArriveCoord* fc = (CArriveCoord*)list->m_320->m_8;
    i32 fcx = fc->x; // grunt head coord x (long-lived)
    i32 fcy = fc->y; // grunt head coord y

    CArriveCoord a;
    g->GetTilePos36c0(&a);
    i32 gy = a.y >> 5;
    i32 gx = a.x >> 5;
    CArriveCoord b;
    g->GetTilePos36c0(&b);
    i32 bx = b.x >> 5;

    // destination cell = grid[gy][bx]; OOB fills the dest buffer itself (self-copy).
    CArriveCell dest;
    CArriveCell* dsrc;
    if ((u32)bx < (u32)m_c->m_c && (u32)gy < (u32)m_c->m_10) {
        dsrc = &m_c->m_8[gy][bx];
    } else {
        memset(&dest, 1, 0x1c);
        dsrc = &dest;
    }
    dest = *dsrc;
    (void)gx;

    // own cell = grid[fcy][fcx], the grunt's head pending coordinate.
    CArriveCell own;
    CArriveCell* osrc;
    if ((u32)fcx < (u32)m_c->m_c && (u32)fcy < (u32)m_c->m_10) {
        osrc = &m_c->m_8[fcy][fcx];
    } else {
        memset(&own, 1, 0x1c);
        osrc = &own;
    }
    own = *osrc;

    i32 maskFlags = own.m_0 & 0xdfffffff;
    i32 type = (g->m_170 > 0x16) ? g->m_19c : g->m_170;

    // ---- door (dest.flags & 0x400, m_2d4==3, type!=8) ----
    if ((dest.m_0 & 0x400) && g->m_2d4 == 3 && type != 8) {
        if (own.m_0 & 0x4000) {
            // door-open transform: build a search rect from the grunt pos, then a per-cell
            // CString-EH loop recycling edge nodes onto g_freeList. (Byte-walled: retail
            // emits a dead stack-address null-recheck our MSVC5 DCEs - see @early-stop.)
            CArriveCoord da;
            g->GetTilePos36c0(&da);
            for (i32 drow = m_c->m_60.top; drow < m_c->m_60.bottom; drow++) {
                for (i32 dcol = m_c->m_60.left; dcol < m_c->m_60.right; dcol++) {
                    CArriveStr cs(0xa);
                    if (!(m_c->m_8[drow][dcol].m_0 & 0x20000000)) {
                        void* h = cs.Head1b4a03();
                        if (h != 0) {
                            void** node = (void**)((char*)h - g_freeListNodeBias);
                            *node = g_freeList;
                            g_freeList = node;
                        }
                    }
                }
            }
        }
        // recompute the grid clip region (IntersectRect with the full-grid rect)
        CArriveQuad full, corners;
        full.Set34a4(0, 0, m_c->m_c, m_c->m_10);
        CArriveQuad* cr = corners.Set34a4(0, 0, m_c->m_c, m_c->m_10);
        CArriveQuad tmp;
        tmp.left = cr->left;
        tmp.top = cr->top;
        tmp.right = cr->right;
        tmp.bottom = cr->bottom;
        if (!IntersectRect((RECT*)&m_c->m_60, (RECT*)&tmp, (RECT*)&corners)) {
            m_c->m_60 = tmp;
        }
        m_c->m_70 = m_c->m_60.right - m_c->m_60.left;
        m_c->m_74 = m_c->m_60.bottom - m_c->m_60.top;
    }

    // ---- door body flag (dest.flags & 4) ----
    if ((dest.m_0 & 4) && g->m_2d8 != 0xb) {
        CArriveCoord tp;
        i32 keyHi = g->m_10->m_5c >> 5;
        g->GetTilePos36c0(&tp);
        i32 key = (keyHi << 8) + (tp.y >> 5);
        (void)(tp.x >> 5);
        CArriveFind2* r = m_14->Find1c21(key, 0);
        if (r->m_4 == 2) {
            g->m_2d4 = 0;
            ARR_RECYCLE(list);
            g->m_2d8 = 0xb;
            g->m_2ec = 0;
            return 0;
        }
    }

    // ---- 0x8000 gate, type 3 ----
    if ((maskFlags & 0x8000) && type == 3 && g->m_2d8 == 0xa) {
        m_8->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (fcy << 5) + 0x10);
        ARR_RECYCLE(list);
        return 0;
    }

    // ---- 0x4000 gate, type 3 ----
    if ((maskFlags & 0x4000) && type == 3 && g->m_2d8 == 0xa) {
        if (m_c->m_8[fcy][fcx].m_10 != 0x99) {
            m_8->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (fcy << 5) + 0x10);
        }
        ARR_RECYCLE(list);
        return 0;
    }

    // ---- 0x200 -> done ----
    if (maskFlags & 0x200) {
        return 1;
    }

    // ---- 0x8 -> probe/effect ----
    if (maskFlags & 0x8) {
        if (Probe1a4b(g, fcx, fcy) != 0) {
            return 1;
        }
        Effect374c(g, 0x12);
    }

    // ---- 0x20 -> type dispatch ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 1 || t == 0x11) {
            if (t == 1) {
                m_8->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (fcy << 5) + 0x10);
                return 1;
            }
            // t == 0x11: scan the 3x3 neighbourhood for the first in-bounds cell
            for (i32 row = fcy - 1; row < fcy + 2; row++) {
                for (i32 col = fcx - 1; col < fcx + 2; col++) {
                    if ((u32)col < (u32)m_c->m_c && (u32)row < (u32)m_c->m_10) {
                        i32 cf = arrCell(m_c, col, row);
                        if (cf & 0x939) {
                            return 1;
                        }
                        if (g->Check3c4c((col << 5) + 0x10, (row << 5) + 0x10) != 0) {
                            m_8->Move14bf(g->m_1ec, g->m_1f0, (col << 5) + 0x10, (row << 5) + 0x10);
                        }
                        return 1;
                    }
                }
            }
        }
    }

    // ---- 0x4000 path, type 0xf (teleport) ----
    if (maskFlags & 0x4000) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 0xf) {
            CArriveFind* r = m_14->FindByField0C2838((fcx << 8) + fcy);
            if (r != 0) {
                if (r->m_arr[m_18] != 0) {
                    ARR_RECYCLE(list);
                    Impact25e5(g, fcx, fcy, 1);
                    return 1;
                }
                m_8->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (fcy << 5) + 0x10);
                return 1;
            }
        }
    }

    // ---- 0x8000 path, type 0xf ----
    if (maskFlags & 0x8000) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 0xf) {
            ARR_RECYCLE(list);
            Impact25e5(g, fcx, fcy, 1);
            return 1;
        }
    }

    // ---- 0x20 path, type 5 ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 5) {
            if (maskFlags & 0x4000) {
                CArriveFind* r = m_14->FindByField0C2838((fcx << 8) + fcy);
                if (r != 0) {
                    i32 k = r->m_0;
                    if (r->m_arr[m_18] != 0) {
                        if (k == 0x13e || k == 0x140 || k == 0x143) {
                            Impact25e5(g, fcx, fcy, 0);
                        }
                    } else {
                        if (k == 0x13e || k == 0x140 || k == 0x143) {
                            m_10->m_2e4->SetCell2f45(fcx, fcy, m_18);
                        }
                    }
                }
            }
            m_8->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (fcy << 5) + 0x10);
            return 0;
        }
        if (t == 0x11 || t == 1) {
            return 1;
        }
        i32 flag = 1;
        if (t == 3 && (maskFlags & 0x4000)) {
            flag = 0;
        }
        if (t == 0xf && (maskFlags & 0x4000)) {
            flag = 0;
        }
        if (flag == 0) {
            return 1;
        }
        Effect374c(g, 5);
        return 0;
    }

    // ---- 0x40 path ----
    if (maskFlags & 0x40) {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t != 0x16) {
            i32 t2 = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
            if (t2 == 0xd) {
                m_8->Move14bf(g->m_1ec, g->m_1f0, (fcx << 5) + 0x10, (fcy << 5) + 0x10);
                return 0;
            }
            Effect374c(g, 0xd);
            return 0;
        }
    }

    // ---- neighbour pick fallback ----
    SelfImpact2b58(g, 0, 0, 0);
    if (Ready27ed(g) != 0) {
        return 1;
    }
    {
        i32 t = (g->m_170 > 0x16) ? g->m_19c : g->m_170;
        if (t == 0x16) {
            return 1;
        }
    }
    {
        i32 oy = g->m_10->m_60 >> 5;
        i32 ox = g->m_10->m_5c >> 5;
        i32 row = rand() % 3 + oy - 1;
        i32 col = rand() % 3 + ox - 1;
        if ((u32)col >= (u32)m_c->m_c || (u32)row >= (u32)m_c->m_10) {
            return 1;
        }
        i32 c0 = arrCell(m_c, col, row);
        i32 c1 = arrCell(m_c, col, row);
        if ((c1 & 0x987) & 0x20000000) {
            return 1;
        }
        if (c1 & 0x987) {
            return 1;
        }
        if (c0 & 0x20000000) {
            return 1;
        }
        g->Trigger1640(col, row, 0x987, 0, 1, 0);
    }
    return 1;
}

SIZE_UNKNOWN(CArriveCell);
SIZE_UNKNOWN(CArriveCoord);
SIZE_UNKNOWN(CArriveCellSetter);
SIZE_UNKNOWN(CArriveFind);
SIZE_UNKNOWN(CArriveFind2);
SIZE_UNKNOWN(CArriveFinder);
SIZE_UNKNOWN(CArriveGrid);
SIZE_UNKNOWN(CArriveGrunt);
SIZE_UNKNOWN(CArriveList);
SIZE_UNKNOWN(CArriveMgr);
SIZE_UNKNOWN(CArriveMover);
SIZE_UNKNOWN(CArriveNode);
SIZE_UNKNOWN(CArriveQuad);
SIZE_UNKNOWN(CArriveStr);
SIZE_UNKNOWN(CArriveSub10b);
