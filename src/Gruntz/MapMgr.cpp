#include <Mfc.h>          // afx-first umbrella (windows.h: RECT + IntersectRect for AllocGrid)
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Io/FileMem.h>   // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/MapMgr.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (Read @+0x2c / Write @+0x30)
#include <Gruntz/Brickz.h>        // CMapMgr (the pathfinding core homed here)
#include <Gruntz/GameMode.h> // canonical CGMVerRect g_versionRect (SetVersionRect's version RECT)
#include <Rez/RezList.h>     // CRezList::AddHead (Search's result hand-off)
#include <rva.h>
#include <stdlib.h> // abs (/Oi intrinsic: |goal-cur| lowers to cdq/xor/sub, not jns)
#include <string.h> // memset (/Oi intrinsic: shr/rep stosd/and/rep stosb)

#include <Gruntz/FreeNodePool.h>

VTBL(CMapMgr, 0x001ea3b4); // vtable_names -> code (RTTI game class)
RVA(0x0009e700, 0xd)
CMapArrayA::CMapArrayA() {
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

// CMapArrayA::Allocate(count): allocate count*0x24
// bytes, then carve the block into a doubly-linked free list (next @elem+0x14,
// prev @elem+0x18). Returns 0 on alloc failure, else 1.
// @early-stop
// ~71% loop strength-reduction / regalloc wall (the loop-induction family, cf.
// docs/patterns/loop-invariant-multiply-strength-reduce-vs-memreread.md): the
// alloc (count*0x24), the three header stores (m_0/m_block/m_count), the pre-loop
// block->m_prev=0, and the per-element prev/next link writes are all byte-faithful,
// but retail strength-reduces `e+1` into a second running pointer (next=cur+0x24)
// and addresses the link fields as next-relative (next-0x10/next-0xc); cl keeps a
// single `cur` and derefs cur+0x14/cur+0x18. A whole-loop induction-variable pick,
// not source-steerable. Logic 100% correct; deferred to the final sweep.
RVA(0x0009e740, 0x76)
i32 CMapArrayA::Allocate(u32 count) {
    BrickzNode* block = static_cast<BrickzNode*>(::operator new(count * sizeof(BrickzNode)));
    m_0 = block;
    if (!block) {
        return reinterpret_cast<i32>(block);
    }

    m_block = block;
    m_count = count;
    block->m_18 = 0;

    BrickzNode* e = block;
    for (u32 i = 0; i < m_count; ++i) {
        if (e == m_block) {
            e->m_18 = 0;
        } else {
            e->m_18 = e - 1;
        }
        e->m_14 = e + 1;
        ++e;
    }
    m_block[m_count - 1].m_14 = 0;
    return 1;
}

RVA(0x0009e7e0, 0x29)
CMapArrayA::~CMapArrayA() {
    if (m_0) {
        ::operator delete(m_0);
    }
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

RVA(0x0009e820, 0xd)
CMapArrayB::CMapArrayB() {
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

// CMapArrayB::Allocate(count): allocate count*0x0c
// bytes, then carve the block into a doubly-linked free list (next @elem+0x08,
// prev @elem+0x04). Returns 0 on alloc failure, else 1.
// @early-stop
// ~62% loop strength-reduction / regalloc wall (same family as CMapArrayA::Allocate
// above): the alloc (count*0xc), header stores, pre-loop block->m_prev=0, and the
// per-element m_prev/m_0/m_next writes are byte-faithful, but retail carries a
// second running pointer (next=cur+0xc) and writes the fields as next-0x8/next-0x4
// while cl keeps a single `cur`. A whole-loop induction-variable pick, not
// source-steerable. Logic 100% correct; deferred to the final sweep.
RVA(0x0009e860, 0x7a)
i32 CMapArrayB::Allocate(u32 count) {
    BrickzNode* block = static_cast<BrickzNode*>(::operator new(count * sizeof(BrickzNode)));
    m_0 = block;
    if (!block) {
        return 0;
    }

    m_block = block;
    m_count = count;
    block->m_prev = 0;

    BrickzNode* e = block;
    for (u32 i = 0; i < m_count; ++i) {
        if (e == m_block) {
            e->m_prev = 0;
        } else {
            e->m_prev = e - 1;
        }
        e->m_child = 0;
        e->m_next = e + 1;
        ++e;
    }
    m_block[m_count - 1].m_next = 0;
    return 1;
}

RVA(0x0009e900, 0x28)
CMapArrayB::~CMapArrayB() {
    if (m_0) {
        ::operator delete(m_0);
    }
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

RVA(0x0009e940, 0x73)
CMapMgr::CMapMgr() {
    m_cellPool = 0;
    m_rows = 0;
    m_width = 0;
    m_height = 0;
    m_openList = 0;
    m_1c = 0;
    m_edgeMask = 0;
    m_maskB = 0;
    m_maskA = -1;
    m_dirty = 1;
}

RVA(0x0009e9e0, 0x5d)
CMapMgr::~CMapMgr() {
    Reset();
}

// ---------------------------------------------------------------------------
// CMapMgr::AllocGrid (0x09ea60) - allocate + initialize the width x height grid:
// new the flat cell pool (0x1c bytes/cell) + the per-row column table, zero the
// pool, thread each row pointer, seed the two intrusive node pools (count*5
// nodes each), record the per-step callback, and compute the grid bounding rect
// (m_originX) via the Win32 IntersectRect (of the {0,0,width,height} box with itself),
// from which m_gridW/m_gridH = the rect width/height. Returns 1, or 0 on any alloc fail.
// @early-stop
// alloc/loop spill wall: logic byte-correct (the two new's + null gates, the
// inline memset, the row-pointer accumulate loop, the two pool inits, the
// IntersectRect rect build + the m_gridW/m_gridH size compute), but the count*0x1c temp
// + the rect stack slots spill against retail's slot schedule. Parked for sweep.
RVA(0x0009ea60, 0x168)
i32 CMapMgr::AllocGrid(i32 width, i32 height, void (*callback)()) {
    i32 count = height * width;
    m_width = width;
    m_height = height;
    m_cellCount = count;
    m_cellPool = static_cast<BrickzCell*>(RezAlloc(count * 0x1c));
    if (m_cellPool == 0) {
        return 0;
    }
    m_rows = static_cast<BrickzCell**>(RezAlloc(height * 4));
    if (m_rows == 0) {
        return 0;
    }
    memset(m_cellPool, 0, count * 0x1c);
    i32 stride = width * 0x1c;
    i32 off = 0;
    for (i32 i = 0; i < height; i++) {
        m_rows[i] = reinterpret_cast<BrickzCell*>((reinterpret_cast<char*>(m_cellPool) + off));
        off += stride;
    }
    if ((reinterpret_cast<CMapArrayA*>(&m_colA.m_block))->Allocate(count * 5) == 0) {
        return 0;
    }
    if (m_colB.Allocate(count * 5) == 0) {
        return 0;
    }
    m_stepCb = callback;
    // Build the grid bounding rect: intersect the {0,0,width,height} box with
    // itself into m_originX (the {left,top,right,bottom} at +0x60); on an empty result
    // fall back to the box. m_gridW/m_gridH = the resulting width/height.
    RECT a;
    RECT b;
    a.left = 0;
    a.top = 0;
    a.right = width;
    a.bottom = height;
    b.left = 0;
    b.top = 0;
    b.right = width;
    b.bottom = height;
    RECT* out = &m_bounds;
    if (!IntersectRect(out, &a, &b)) {
        *out = a;
    }
    m_gridW = out->right - out->left;
    m_gridH = out->bottom - out->top;
    return 1;
}

RVA(0x0009ec30, 0x4b)
void CMapMgr::Reset() {
    if (m_cellPool) {
        ::operator delete(m_cellPool);
    }
    if (m_rows) {
        ::operator delete(m_rows);
    }

    m_colA.~CMapArrayA();
    m_colB.~CMapArrayB();

    m_cellPool = 0;
    m_rows = 0;
    m_width = 0;
    m_height = 0;
    m_openList = 0;
    m_1c = 0;
}

// ---------------------------------------------------------------------------
// CMapMgr::Search (0x09eca0) - the A*-style grid search driver. Bounds-check the
// start (x1,y1) and goal (x2,y2) against the grid origin (m_originX,m_originY) / size
// (m_gridW,m_gridH); reject if the start cell fails the passability masks; reset the
// per-cell open counts; seed the open list with the start record and expand
// neighbours via Expand until the open list empties or the goal is popped. On
// success the result path is recycled to g_brickzFreeList + handed to `list`.
// @early-stop
// regalloc wall, ~91%: logic byte-correct (BFS loop + path-walk + free-list pop
// all match). Residual is the opening bounds-gate's callee-saved assignment
// (retail pins x1 in ebx for the whole fn, reused at m_startX=x1) and the cell-clear
// loop's reg/zero choice; no source spelling flips MSVC5's allocator here.
RVA(0x0009eca0, 0x2bd)
i32 CMapMgr::Search(
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    void* list,
    i32 maskA,
    i32 maskB,
    i32 maskC
) {
    i32 ox = m_bounds.left;
    if (static_cast<u32>((x1 - ox)) >= static_cast<u32>(m_gridW)) {
        return 0;
    }
    i32 oy = m_bounds.top;
    i32 hgt = m_gridH;
    if (static_cast<u32>((y1 - oy)) >= static_cast<u32>(hgt)) {
        return 0;
    }
    if (static_cast<u32>((x2 - ox)) >= static_cast<u32>(m_gridW)) {
        return 0;
    }
    if (static_cast<u32>((y2 - oy)) >= static_cast<u32>(hgt)) {
        return 0;
    }
    m_maskC = maskC;
    m_maskB = maskB;
    m_maskA = maskA;
    i32 flags = *reinterpret_cast<i32*>(&m_rows[y2][x2]);
    if ((maskA & flags) != 0 && (maskC & flags) != 0) {
        return 0;
    }
    // Reset the per-cell open counts across all m_cellCount cells.
    if (m_cellCount != 0) {
        u32 i = 0;
        i32 off = 0;
        do {
            *reinterpret_cast<i32*>((reinterpret_cast<char*>(m_cellPool) + off + 0x14)) = 0;
            i++;
            off += 0x1c;
        } while (i < m_cellCount);
    }
    if (x1 == x2 && y1 == y2) {
        return 1;
    }
    m_goalX = x2;
    m_startX = x1;
    m_goalY = y2;
    m_startY = y1;
    // Pop the seed record off the closed (m_30) list head.
    BrickzNode* seed = m_colA.m_block;
    BrickzNode* slot = seed->m_14;
    if (slot == 0) {
        m_colA.m_block = 0;
    } else {
        m_colA.m_block = slot;
        slot->m_18 = 0;
    }
    if (seed == 0) {
        return 0;
    }
    seed->m_0 = x1;
    seed->m_4 = y1;
    seed->m_8 = 0;
    i32 dx = abs(m_goalY - y1);
    i32 dxx = abs(m_goalX - x1);
    i32 h = (dx + dxx) * 2;
    seed->m_c = h;
    seed->m_10 = h;
    seed->m_14 = 0;
    seed->m_18 = 0;
    seed->m_1c = 0;
    Insert(seed);
    (&m_rows[y1][x1])->m_count++;
    BrickzNode* node = 0;
    while (m_openList != 0) {
        node = PopFront();
        (&m_rows[node->m_4][node->m_0])->m_count--;
        if (node->m_0 == m_goalX && node->m_4 == m_goalY) {
            goto reached;
        }
        Expand(node, 0, 1, 2, 0);
        Expand(node, 1, 0, 2, 0);
        Expand(node, 0, -1, 2, 0);
        Expand(node, -1, 0, 2, 0);
        Expand(node, 1, 1, 3, 1);
        Expand(node, 1, -1, 3, 1);
        Expand(node, -1, -1, 3, 1);
        Expand(node, -1, 1, 3, 1);
        CellPush(node);
    }
    node = 0;
    Drain();
    Reset();
    if (m_stepCb != 0) {
        m_stepCb();
    }
    return 0;

reached:
    BrickzNode* p = node;
    do {
        BrickzFreeRec* rec = reinterpret_cast<BrickzFreeRec*>(g_coordPool.m_freeHead);
        i32* slot = 0;
        if (rec->m_0 != 0) {
            slot = &rec->m_4;
            rec->m_4 = p->m_0;
            rec->m_8 = p->m_4;
            g_coordPool.m_freeHead = reinterpret_cast<CoordPoolNode*>(rec->m_0);
        }
        (static_cast<CRezList*>(list))->AddHead(reinterpret_cast<CRezListNode*>(slot));
        p = reinterpret_cast<BrickzNode*>(p->m_1c);
    } while (p != 0);
    if (m_stepCb != 0) {
        m_stepCb();
    }
    node->m_18 = 0;
    node->m_14 = m_colA.m_block;
    m_colA.m_block->m_18 = node;
    m_colA.m_block = node;
    Drain();
    Reset();
    return 1;
}

// ---------------------------------------------------------------------------
// CMapMgr::Expand (0x09f010) - relax one neighbour of `node` in the (dx,dy)
// direction. Bounds-check the target cell, reject it on the passability masks
// (m_edgeMask / m_maskA&m_maskC), and for a diagonal step (diag) reject corner-cutting via
// the two orthogonal cells (m_maskB). If the cell is unvisited / improvable, take a
// search record (reusing an open Find()ed record, a closed CellPop'd record, or
// a fresh one off the m_30 pool), set its g = node->g + cost and f = g + h
// (h = 2*(|gx-ncol| + |gy-nrow|)), parent = node, and Insert() it. Returns 1
// (the open-list is unchanged only when out of records => 0).
// @early-stop
// spill-scheduling + sibling-retest wall, ~79%: logic byte-correct (bounds gate,
// 4-quadrant corner check, open/closed-record relax, heuristic abs() all match).
// Residual is the prologue's local-spill count (retail spills ng/ncol/nrow to 4
// stack slots; MSVC5 rematerializes into 2) + the redundant open-node re-tests;
// neither flips with a local-pin here. Parked for the final sweep.
RVA(0x0009f010, 0x2a1)
i32 CMapMgr::Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag) {
    i32 ng = reinterpret_cast<i32>(node->m_8) + cost;
    i32 ncol = node->m_0 + dx;
    i32 nrow = node->m_4 + dy;
    BrickzNode* found0 = 0;
    if (static_cast<u32>((ncol - m_bounds.left)) >= static_cast<u32>(m_gridW)) {
        return 1;
    }
    if (static_cast<u32>((nrow - m_bounds.top)) >= static_cast<u32>(m_gridH)) {
        return 1;
    }
    i32* ncell = reinterpret_cast<i32*>(&m_rows[nrow][ncol]);
    i32 nflags = *ncell;
    i32* cell = reinterpret_cast<i32*>(&m_rows[node->m_4][node->m_0]);
    if ((m_edgeMask & nflags) != 0) {
        return 1;
    }
    if ((m_maskA & nflags) != 0 && (m_maskC & nflags) == 0) {
        return 1;
    }
    if (diag != 0 && m_maskB != 0) {
        i32 *cellA, *cellB;
        if (dx > 0 && dy > 0) {
            cellB = cell + (m_width * 7);
            cellA = cell + 7;
        } else if (dx < 0 && dy > 0) {
            cellB = cell + (m_width * 7);
            cellA = cell - 7;
        } else if (dx > 0 && dy < 0) {
            cellB = cell - (m_width * 7);
            cellA = cell + 7;
        } else if (dx < 0 && dy < 0) {
            cellB = cell - (m_width * 7);
            cellA = cell - 7;
        } else {
            goto relax;
        }
        if ((m_maskB & *cellA) != 0 || (m_maskB & *cellB) != 0) {
            return 1;
        }
    }
relax:
    BrickzNode* closed = 0;
    BrickzNode* head = (reinterpret_cast<BrickzCell*>(ncell))->m_head;
    if (head != 0) {
        closed = reinterpret_cast<BrickzNode*>(head->m_0);
    }
    if (closed != 0) {
        if (ng >= reinterpret_cast<i32>(closed->m_8)) {
            return 1;
        }
    }
    BrickzNode* open;
    if ((reinterpret_cast<BrickzCell*>(ncell))->m_count != 0) {
        open = Find(ncol, nrow);
    } else {
        open = found0;
    }
    if (open != 0) {
        i32 og = reinterpret_cast<i32>(open->m_8);
        if (ng >= og) {
            return 1;
        }
        if (open != 0 && ng < og) {
            if (closed != 0) {
                CellPop(closed, 1);
            }
            Unlink(open);
            open->m_10 = ng + open->m_c;
            open->m_1c = reinterpret_cast<i32>(node);
            open->m_8 = reinterpret_cast<BrickzNode*>(ng);
            Insert(open);
            return 1;
        }
    }
    if (closed != 0) {
        if (ng < reinterpret_cast<i32>(closed->m_8)) {
            CellPop(closed, 0);
            closed->m_1c = reinterpret_cast<i32>(node);
            closed->m_8 = reinterpret_cast<BrickzNode*>(ng);
            closed->m_10 = closed->m_c + ng;
            Insert(closed);
            (reinterpret_cast<BrickzCell*>(ncell))->m_count++;
            return 1;
        }
        CellPop(closed, 1);
    }
    if (open != 0) {
        return 1;
    }
    BrickzNode* rec = m_colA.m_block;
    BrickzNode* nx = rec->m_14;
    if (nx == 0) {
        rec = 0;
    } else {
        m_colA.m_block = nx;
        nx->m_18 = 0;
    }
    if (rec == 0) {
        return 0;
    }
    rec->m_0 = ncol;
    rec->m_4 = nrow;
    rec->m_8 = reinterpret_cast<BrickzNode*>(ng);
    i32 hy = abs(m_goalY - nrow);
    i32 hx = abs(m_goalX - ncol);
    i32 h = (hy + hx) * 2;
    rec->m_1c = reinterpret_cast<i32>(node);
    rec->m_c = h;
    rec->m_10 = ng + h;
    rec->m_14 = 0;
    rec->m_18 = 0;
    rec->m_20 = 0;
    Insert(rec);
    (reinterpret_cast<BrickzCell*>(ncell))->m_count++;
    return 1;
}

RVA(0x0009f370, 0x8a)
i32 CMapMgr::Insert(BrickzNode* node) {
    BrickzNode* cur = m_openList;
    node->m_18 = 0;
    node->m_14 = 0;
    if (cur == 0) {
        m_openList = node;
        return 1;
    }
    i32 key = node->m_10;
    while (cur != 0) {
        if (key < cur->m_10) {
            if (cur->m_18 != 0) {
                node->m_18 = cur->m_18;
                node->m_14 = cur;
                cur->m_18->m_14 = node;
                cur->m_18 = node;
            } else {
                m_openList = node;
                node->m_14 = cur;
                cur->m_18 = node;
            }
            return 1;
        }
        if (cur->m_14 == 0) {
            cur->m_14 = node;
            node->m_18 = cur;
            return 1;
        }
        cur = cur->m_14;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CMapMgr::PopFront - detach the head of the m_openList list; promote its m_cellCount
// successor (clearing the successor's back-link) and clear the popped links.
// Returns the popped head (eax, consumed by Search). The return type does not
// affect the callee's own bytes - head is already materialized in eax.
// @early-stop
// regalloc wall: only residual is a head<->next register swap (retail pins head
// in eax, recompile lands it in edx); logic byte-correct, 97% (no source steer).
RVA(0x0009f430, 0x2a)
BrickzNode* CMapMgr::PopFront() {
    BrickzNode* head = m_openList;
    if (head != 0) {
        BrickzNode* next = head->m_14;
        if (next != 0) {
            m_openList = next;
            next->m_18 = 0;
        } else {
            m_openList = 0;
        }
        head->m_14 = 0;
        head->m_18 = 0;
    }
    return head;
}

// ---------------------------------------------------------------------------
// CMapMgr::CellPush - allocate a bucket node from the m_40 free list and link it
// into the grid cell m_rows[node->m_4][node->m_0]; record the slot in node->m_20.
// @early-stop
// regalloc/scheduling wall: branch shape + free-list pop byte-match; only the
// arg-pointer register (retail defers the `node` load past the 3 pushes -> edi;
// recompile loads it pre-push -> edx) and the dependent reg chain differ, ~86%.
RVA(0x0009f470, 0x62)
void CMapMgr::CellPush(BrickzNode* node) {
    BrickzNode** head = &m_rows[node->m_4][node->m_0].m_head;
    BrickzNode* slot = m_colB.m_block;
    BrickzNode* nx = slot->m_8;
    if (nx == 0) {
        slot = 0;
    } else {
        m_colB.m_block = nx;
        nx->m_4 = 0;
    }
    BrickzNode* old = *head;
    if (old == 0) {
        *head = slot;
        slot->m_4 = 0;
        slot->m_8 = 0;
        slot->m_0 = reinterpret_cast<i32>(node);
        node->m_20 = slot;
    } else {
        slot->m_4 = reinterpret_cast<i32>(old);
        slot->m_8 = (*head)->m_8;
        *head = slot;
        node->m_20 = slot;
    }
}

RVA(0x0009f500, 0x24)
BrickzNode* CMapMgr::Find(i32 key1, i32 key2) {
    BrickzNode* p = m_openList;
    if (p == 0) {
        return 0;
    }
    do {
        if (p->m_0 == key1 && p->m_4 == key2) {
            return p;
        }
        p = p->m_14;
    } while (p != 0);
    return 0;
}

RVA(0x0009f540, 0x40)
BrickzNode* CMapMgr::FindCellNode(i32 col, i32 row) {
    BrickzNode* n = m_rows[row][col].m_head;
    while (n != 0) {
        BrickzNode* child = reinterpret_cast<BrickzNode*>(n->m_0);
        if (child->m_0 == col && child->m_4 == row) {
            return reinterpret_cast<BrickzNode*>(n->m_0);
        }
        n = n->m_8;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CMapMgr::Drain - move every node off the m_openList list onto the front of the m_30
// list (re-threaded via m_cellCount/m_openList), then clear the m_openList head.
// @early-stop
// regalloc wall: retail materializes &node->m_14 as a base ptr in a callee-saved
// reg (lea + 3 pushes); recompile uses a direct offset + 2 pushes. Logic
// byte-correct, ~67% (no source spelling forces the 3rd reg / lea base).
RVA(0x0009f590, 0x2f)
void CMapMgr::Drain() {
    BrickzNode* p = m_openList;
    if (p != 0) {
        do {
            BrickzNode* next = p->m_14;
            p->m_14 = m_colA.m_block;
            p->m_18 = 0;
            m_colA.m_block->m_18 = p;
            m_colA.m_block = p;
            p = next;
        } while (p != 0);
    }
    m_openList = 0;
}

// ---------------------------------------------------------------------------
// CMapMgr::Reset - empty every grid cell: each bucket node's child (m_0) is
// pushed onto the m_30 active list and the bucket node itself onto the m_40
// free list; then the cell head is cleared.
// @early-stop
// regalloc/addressing wall (same family as Drain): retail materializes
// &node->m_8 as a callee-saved base ptr (lea + extra reg) and commutes the
// m_height*m_width imul operand order; recompile uses direct offsets. Logic
// byte-correct (loop structure + unsigned counter match), ~65%.
RVA(0x0009f5d0, 0x81)
void CMapMgr::ResetCells() {
    BrickzCell* cell = m_cellPool;
    for (u32 i = 0; i < m_height * m_width; i++) {
        BrickzNode* node = cell->m_head;
        while (node != 0) {
            BrickzNode** link = &node->m_8;
            BrickzNode* next = *link;
            BrickzNode* child = reinterpret_cast<BrickzNode*>(node->m_0);
            child->m_14 = m_colA.m_block;
            child->m_18 = 0;
            m_colA.m_block->m_18 = child;
            m_colA.m_block = child;
            node->m_4 = 0;
            *link = m_colB.m_block;
            m_colB.m_block->m_4 = reinterpret_cast<i32>(node);
            m_colB.m_block = node;
            node = next;
        }
        cell->m_head = 0;
        cell++;
    }
}

// ---------------------------------------------------------------------------
// CMapMgr::Unlink - remove node from the m_openList-headed doubly-linked list
// (m_cellCount = next, m_openList = prev), repairing the neighbours and the head, then
// clearing the node's links.
// @early-stop
// sibling-guard-retest + regalloc wall: retail keeps redundant `cmp prev,0`
// re-tests (4-way dispatch from a sequential-if source) and uses 2 callee-saved
// regs; with no calls to pin the flag MSVC5 folds the re-tests + uses 1 reg.
// Logic byte-correct, ~75%.
RVA(0x0009f690, 0x5d)
void CMapMgr::Unlink(BrickzNode* node) {
    if (node->m_18 != 0) {
        if (node->m_14 != 0) {
            node->m_18->m_14 = node->m_14;
            node->m_14->m_18 = node->m_18;
        }
    } else if (node->m_14 == 0) {
        m_openList = 0;
    } else if (node->m_18 == 0) {
        BrickzNode* next = node->m_14;
        if (next != 0) {
            m_openList = next;
            next->m_18 = 0;
        }
    }
    if (node->m_18 != 0 && node->m_14 == 0) {
        node->m_18->m_14 = 0;
    }
    node->m_18 = 0;
    node->m_14 = 0;
}

// ---------------------------------------------------------------------------
// CMapMgr::CellPop - remove node's bucket slot (node->m_20) from its grid cell's
// doubly-linked bucket list (m_cellPool = prev, m_rows = next), clear node's links, return
// the slot to the m_40 free list, and (if flag) push node onto the m_30 list.
// @early-stop
// sibling-guard-retest wall (same as Unlink): the 4-way prev/next dispatch keeps
// redundant `cmp prev,0` re-tests in retail that MSVC5 folds with no call to pin
// the flag. Logic byte-correct, container shape proven; parked for the sweep.
RVA(0x0009f710, 0xa7)
void CMapMgr::CellPop(BrickzNode* node, i32 flag) {
    BrickzNode** head = &m_rows[node->m_4][node->m_0].m_head;
    BrickzNode* slot = node->m_20;
    if (reinterpret_cast<BrickzNode*>(slot->m_4) != 0) {
        if (slot->m_8 != 0) {
            (reinterpret_cast<BrickzNode*>(slot->m_4))->m_8 = slot->m_8;
            slot->m_8->m_4 = slot->m_4;
        }
    } else if (slot->m_8 == 0) {
        *head = 0;
    } else if (reinterpret_cast<BrickzNode*>(slot->m_4) == 0) {
        BrickzNode* next = slot->m_8;
        if (next != 0) {
            *head = next;
            next->m_4 = 0;
        }
    }
    if (reinterpret_cast<BrickzNode*>(slot->m_4) != 0 && slot->m_8 == 0) {
        (reinterpret_cast<BrickzNode*>(slot->m_4))->m_8 = 0;
    }
    node->m_18 = 0;
    node->m_14 = 0;
    node->m_20 = 0;
    slot->m_8 = m_colB.m_block;
    slot->m_4 = 0;
    m_colB.m_block->m_4 = reinterpret_cast<i32>(slot);
    m_colB.m_block = slot;
    if (flag != 0) {
        node->m_18 = 0;
        node->m_14 = m_colA.m_block;
        m_colA.m_block->m_18 = node;
        m_colA.m_block = node;
    }
}

RVA(0x0009f7f0, 0x3b)
i32 CMapMgr::Visit(CFileMemBase* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (Save(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Load(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// CMapMgr::Save (slot 2, 0x09f840): stream the scalar bookkeeping members out, then
// the whole cell grid (m_cellPool[j*m_width + i], 0x1c bytes each) row-major over m_width x m_height.
// @early-stop
// commutative-imul operand-materialization coin-flip (99.92%, docs/patterns/
// commutative-imul-operand-in-eax.md): the whole body is byte-faithful (null gate,
// the 12 field Write()s, ebp=&m_width base-pointer, the doubly-nested grid loop). The
// sole residue is the inner index `j*m_width`: retail loads j (`mov eax,edi`) then
// `imul eax,[ebp](m_width)`; cl loads m_width (`mov eax,[ebp]`) then `imul eax,edi` - both
// re-read m_width from memory, same 6 bytes, only which operand lands in eax differs.
// MSVC5 canonicalizes the commutative multiply regardless of source form (tried
// `j*m_width`, `m_width*j`, `i+m_width*j`, compound `idx=j;idx*=m_width` + permute - all identical).
// Not source-steerable; parked for the final sweep.
RVA(0x0009f840, 0x110)
i32 CMapMgr::Save(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_width, 4);
    ar->Write(&m_height, 4);
    ar->Write(&m_cellCount, 4);
    ar->Write(&m_startX, 8);
    ar->Write(&m_goalX, 8);
    ar->Write(&m_maskA, 4);
    ar->Write(&m_maskC, 4);
    ar->Write(&m_maskB, 4);
    ar->Write(&m_dirty, 4);
    ar->Write(&m_bounds.left, 0x10);
    ar->Write(&m_gridW, 4);
    ar->Write(&m_gridH, 4);
    for (u32 i = 0; i < m_width; i++) {
        for (u32 j = 0; j < m_height; j++) {
            ar->Write(&m_cellPool[j * m_width + i], 0x1c);
        }
    }
    return 1;
}

// CMapMgr::Load (slot 3, 0x09f9a0): the read counterpart of Save; after reading each
// cell, zero its +0x18 runtime field (not a persisted value).
// @early-stop
// same commutative-imul operand pick as Save (99.85%, docs/patterns/
// commutative-imul-operand-in-eax.md) - two occurrences here (the read index and the
// zero-cell index), each a `mov eax,edi;imul eax,[ebp]` vs `mov eax,[ebp];imul eax,edi`
// byte-swap. Logic byte-faithful; not source-steerable. Parked for the final sweep.
RVA(0x0009f9a0, 0x12e)
i32 CMapMgr::Load(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_width, 4);
    ar->Read(&m_height, 4);
    ar->Read(&m_cellCount, 4);
    ar->Read(&m_startX, 8);
    ar->Read(&m_goalX, 8);
    ar->Read(&m_maskA, 4);
    ar->Read(&m_maskC, 4);
    ar->Read(&m_maskB, 4);
    ar->Read(&m_dirty, 4);
    ar->Read(&m_bounds.left, 0x10);
    ar->Read(&m_gridW, 4);
    ar->Read(&m_gridH, 4);
    for (u32 i = 0; i < m_width; i++) {
        for (u32 j = 0; j < m_height; j++) {
            ar->Read(&m_cellPool[j * m_width + i], 0x1c);
            m_cellPool[j * m_width + i].m_head = 0;
        }
    }
    return 1;
}

RVA(0x0009fe10, 0x29)
void SetVersionRect() {
    g_versionRect.a = 5;
    g_versionRect.b = 0x1c5;
    g_versionRect.c = 0x27b;
    g_versionRect.d = 0x1de;
}
