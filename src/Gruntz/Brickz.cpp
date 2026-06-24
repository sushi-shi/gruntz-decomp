// Brickz.cpp - reconstruction of the trace-discovered "CBrickz" cluster.
//
// Placeholder class name (see <Gruntz/Brickz.h>): these are __thiscall pointer-
// shuffle ops over a self-contained graph/grid container's intrusive node lists.
// They match by shape; field names are placeholders, offsets are load-bearing.
#include <rva.h>

#include <Gruntz/Brickz.h>

// abs() is a /Oi intrinsic under /O2: the heuristic's |goal - cur| terms lower
// branchlessly to `cdq; xor; sub` (matching retail), not a `jns` branch.
extern "C" i32 __cdecl abs(i32);

// A recycled result record off the shared free-list: m_0 = next-free link,
// m_4/m_8 = the path cell (col,row) handed to the result list.
struct BrickzFreeRec {
    i32 m_0; // +0x00  next-free link
    i32 m_4; // +0x04  path col
    i32 m_8; // +0x08  path row
};

// The intrusive free-list the search nodes recycle into when a goal node is
// reached (shared global pool; modeled extern + DATA() so the load reloc-masks).
// ?g_freeList@@3PAXA at VA 0x645544.
DATA(0x00245544)
extern BrickzFreeRec* g_brickzFreeList;

// CPtrList::AddHead (0x1b4967, __thiscall) - the list the matched search records
// are handed off to as the result path; modeled no-body so the call reloc-masks.
struct BrickzResultList {
    void* AddHead(void* node); // 0x1b4967
};

// ---------------------------------------------------------------------------
// CBrickz::Search (0x09eca0) - the A*-style grid search driver. Bounds-check the
// start (x1,y1) and goal (x2,y2) against the grid origin (m_60,m_64) / size
// (m_70,m_74); reject if the start cell fails the passability masks; reset the
// per-cell open counts; seed the open list with the start record and expand
// neighbours via Expand until the open list empties or the goal is popped. On
// success the result path is recycled to g_brickzFreeList + handed to `list`.
// @early-stop
// regalloc wall, ~91%: logic byte-correct (BFS loop + path-walk + free-list pop
// all match). Residual is the opening bounds-gate's callee-saved assignment
// (retail pins x1 in ebx for the whole fn, reused at m_20=x1) and the cell-clear
// loop's reg/zero choice; no source spelling flips MSVC5's allocator here.
RVA(0x0009eca0, 0x2bd)
i32 CBrickz::Search(i32 x1, i32 y1, i32 x2, i32 y2, void* list, i32 maskA, i32 maskB, i32 maskC) {
    i32 ox = m_60;
    if ((u32)(x1 - ox) >= (u32)m_70) {
        return 0;
    }
    i32 oy = m_64;
    i32 hgt = m_74;
    if ((u32)(y1 - oy) >= (u32)hgt) {
        return 0;
    }
    if ((u32)(x2 - ox) >= (u32)m_70) {
        return 0;
    }
    if ((u32)(y2 - oy) >= (u32)hgt) {
        return 0;
    }
    m_54 = maskC;
    m_58 = maskB;
    m_50 = maskA;
    i32 flags = *(i32*)&m_8[y2][x2];
    if ((maskA & flags) != 0 && (maskC & flags) != 0) {
        return 0;
    }
    // Reset the per-cell open counts across all m_14 cells.
    if (m_14 != 0) {
        u32 i = 0;
        i32 off = 0;
        do {
            *(i32*)((char*)m_4 + off + 0x14) = 0;
            i++;
            off += 0x1c;
        } while (i < m_14);
    }
    if (x1 == x2 && y1 == y2) {
        return 1;
    }
    m_28 = x2;
    m_20 = x1;
    m_2c = y2;
    m_24 = y1;
    // Pop the seed record off the closed (m_30) list head.
    BrickzNode* seed = m_30;
    BrickzNode* slot = seed->m_14;
    if (slot == 0) {
        m_30 = 0;
    } else {
        m_30 = slot;
        slot->m_18 = 0;
    }
    if (seed == 0) {
        return 0;
    }
    seed->m_0 = x1;
    seed->m_4 = y1;
    seed->m_8 = 0;
    i32 dx = abs(m_2c - y1);
    i32 dxx = abs(m_28 - x1);
    i32 h = (dx + dxx) * 2;
    seed->m_c = h;
    seed->m_10 = h;
    seed->m_14 = 0;
    seed->m_18 = 0;
    seed->m_1c = 0;
    Insert(seed);
    (&m_8[y1][x1])->m_count++;
    BrickzNode* node = 0;
    while (m_18 != 0) {
        node = PopFront();
        (&m_8[node->m_4][node->m_0])->m_count--;
        if (node->m_0 == m_28 && node->m_4 == m_2c) {
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
    if (m_48 != 0) {
        m_48();
    }
    return 0;

reached:
    BrickzNode* p = node;
    do {
        BrickzFreeRec* rec = (BrickzFreeRec*)g_brickzFreeList;
        i32* slot = 0;
        if (rec->m_0 != 0) {
            slot = &rec->m_4;
            rec->m_4 = p->m_0;
            rec->m_8 = p->m_4;
            g_brickzFreeList = (BrickzFreeRec*)rec->m_0;
        }
        ((BrickzResultList*)list)->AddHead(slot);
        p = (BrickzNode*)p->m_1c;
    } while (p != 0);
    if (m_48 != 0) {
        m_48();
    }
    node->m_18 = 0;
    node->m_14 = m_30;
    m_30->m_18 = node;
    m_30 = node;
    Drain();
    Reset();
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickz::Expand (0x09f010) - relax one neighbour of `node` in the (dx,dy)
// direction. Bounds-check the target cell, reject it on the passability masks
// (m_4c / m_50&m_54), and for a diagonal step (diag) reject corner-cutting via
// the two orthogonal cells (m_58). If the cell is unvisited / improvable, take a
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
i32 CBrickz::Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag) {
    i32 ng = (i32)node->m_8 + cost;
    i32 ncol = node->m_0 + dx;
    i32 nrow = node->m_4 + dy;
    BrickzNode* found0 = 0;
    if ((u32)(ncol - m_60) >= (u32)m_70) {
        return 1;
    }
    if ((u32)(nrow - m_64) >= (u32)m_74) {
        return 1;
    }
    i32* ncell = (i32*)&m_8[nrow][ncol];
    i32 nflags = *ncell;
    i32* cell = (i32*)&m_8[node->m_4][node->m_0];
    if ((m_4c & nflags) != 0) {
        return 1;
    }
    if ((m_50 & nflags) != 0 && (m_54 & nflags) == 0) {
        return 1;
    }
    if (diag != 0 && m_58 != 0) {
        i32 *cellA, *cellB;
        if (dx > 0 && dy > 0) {
            cellB = cell + (m_c * 7);
            cellA = cell + 7;
        } else if (dx < 0 && dy > 0) {
            cellB = cell + (m_c * 7);
            cellA = cell - 7;
        } else if (dx > 0 && dy < 0) {
            cellB = cell - (m_c * 7);
            cellA = cell + 7;
        } else if (dx < 0 && dy < 0) {
            cellB = cell - (m_c * 7);
            cellA = cell - 7;
        } else {
            goto relax;
        }
        if ((m_58 & *cellA) != 0 || (m_58 & *cellB) != 0) {
            return 1;
        }
    }
relax:
    BrickzNode* closed = 0;
    BrickzNode* head = ((BrickzCell*)ncell)->m_head;
    if (head != 0) {
        closed = (BrickzNode*)head->m_0;
    }
    if (closed != 0) {
        if (ng >= (i32)closed->m_8) {
            return 1;
        }
    }
    BrickzNode* open;
    if (((BrickzCell*)ncell)->m_count != 0) {
        open = Find(ncol, nrow);
    } else {
        open = found0;
    }
    if (open != 0) {
        i32 og = (i32)open->m_8;
        if (ng >= og) {
            return 1;
        }
        if (open != 0 && ng < og) {
            if (closed != 0) {
                CellPop(closed, 1);
            }
            Unlink(open);
            open->m_10 = ng + open->m_c;
            open->m_1c = (i32)node;
            open->m_8 = (BrickzNode*)ng;
            Insert(open);
            return 1;
        }
    }
    if (closed != 0) {
        if (ng < (i32)closed->m_8) {
            CellPop(closed, 0);
            closed->m_1c = (i32)node;
            closed->m_8 = (BrickzNode*)ng;
            closed->m_10 = closed->m_c + ng;
            Insert(closed);
            ((BrickzCell*)ncell)->m_count++;
            return 1;
        }
        CellPop(closed, 1);
    }
    if (open != 0) {
        return 1;
    }
    BrickzNode* rec = m_30;
    BrickzNode* nx = rec->m_14;
    if (nx == 0) {
        rec = 0;
    } else {
        m_30 = nx;
        nx->m_18 = 0;
    }
    if (rec == 0) {
        return 0;
    }
    rec->m_0 = ncol;
    rec->m_4 = nrow;
    rec->m_8 = (BrickzNode*)ng;
    i32 hy = abs(m_2c - nrow);
    i32 hx = abs(m_28 - ncol);
    i32 h = (hy + hx) * 2;
    rec->m_1c = (i32)node;
    rec->m_c = h;
    rec->m_10 = ng + h;
    rec->m_14 = 0;
    rec->m_18 = 0;
    rec->m_20 = 0;
    Insert(rec);
    ((BrickzCell*)ncell)->m_count++;
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickz::Insert - insert node into the m_18-headed list, kept ascending by
// m_10. Links: m_14 = forward (next), m_18 = backward (prev). Always returns 1.
RVA(0x0009f370, 0x8a)
i32 CBrickz::Insert(BrickzNode* node) {
    BrickzNode* cur = m_18;
    node->m_18 = 0;
    node->m_14 = 0;
    if (cur == 0) {
        m_18 = node;
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
                m_18 = node;
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
// CBrickz::PopFront - detach the head of the m_18 list; promote its m_14
// successor (clearing the successor's back-link) and clear the popped links.
// Returns the popped head (eax, consumed by Search). The return type does not
// affect the callee's own bytes - head is already materialized in eax.
// @early-stop
// regalloc wall: only residual is a head<->next register swap (retail pins head
// in eax, recompile lands it in edx); logic byte-correct, 97% (no source steer).
RVA(0x0009f430, 0x2a)
BrickzNode* CBrickz::PopFront() {
    BrickzNode* head = m_18;
    if (head != 0) {
        BrickzNode* next = head->m_14;
        if (next != 0) {
            m_18 = next;
            next->m_18 = 0;
        } else {
            m_18 = 0;
        }
        head->m_14 = 0;
        head->m_18 = 0;
    }
    return head;
}

// ---------------------------------------------------------------------------
// CBrickz::Unlink - remove node from the m_18-headed doubly-linked list
// (m_14 = next, m_18 = prev), repairing the neighbours and the head, then
// clearing the node's links.
// @early-stop
// sibling-guard-retest + regalloc wall: retail keeps redundant `cmp prev,0`
// re-tests (4-way dispatch from a sequential-if source) and uses 2 callee-saved
// regs; with no calls to pin the flag MSVC5 folds the re-tests + uses 1 reg.
// Logic byte-correct, ~75%.
RVA(0x0009f690, 0x5d)
void CBrickz::Unlink(BrickzNode* node) {
    if (node->m_18 != 0) {
        if (node->m_14 != 0) {
            node->m_18->m_14 = node->m_14;
            node->m_14->m_18 = node->m_18;
        }
    } else if (node->m_14 == 0) {
        m_18 = 0;
    } else if (node->m_18 == 0) {
        BrickzNode* next = node->m_14;
        if (next != 0) {
            m_18 = next;
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
// CBrickz::CellPush - allocate a bucket node from the m_40 free list and link it
// into the grid cell m_8[node->m_4][node->m_0]; record the slot in node->m_20.
// @early-stop
// regalloc/scheduling wall: branch shape + free-list pop byte-match; only the
// arg-pointer register (retail defers the `node` load past the 3 pushes -> edi;
// recompile loads it pre-push -> edx) and the dependent reg chain differ, ~86%.
RVA(0x0009f470, 0x62)
void CBrickz::CellPush(BrickzNode* node) {
    BrickzNode** head = &m_8[node->m_4][node->m_0].m_head;
    BrickzNode* slot = m_40;
    BrickzNode* nx = slot->m_8;
    if (nx == 0) {
        slot = 0;
    } else {
        m_40 = nx;
        nx->m_4 = 0;
    }
    BrickzNode* old = *head;
    if (old == 0) {
        *head = slot;
        slot->m_4 = 0;
        slot->m_8 = 0;
        slot->m_0 = (i32)node;
        node->m_20 = slot;
    } else {
        slot->m_4 = (i32)old;
        slot->m_8 = (*head)->m_8;
        *head = slot;
        node->m_20 = slot;
    }
}

// ---------------------------------------------------------------------------
// CBrickz::Reset - empty every grid cell: each bucket node's child (m_0) is
// pushed onto the m_30 active list and the bucket node itself onto the m_40
// free list; then the cell head is cleared.
// @early-stop
// regalloc/addressing wall (same family as Drain): retail materializes
// &node->m_8 as a callee-saved base ptr (lea + extra reg) and commutes the
// m_10*m_c imul operand order; recompile uses direct offsets. Logic
// byte-correct (loop structure + unsigned counter match), ~65%.
RVA(0x0009f5d0, 0x81)
void CBrickz::Reset() {
    BrickzCell* cell = m_4;
    for (u32 i = 0; i < m_10 * m_c; i++) {
        BrickzNode* node = cell->m_head;
        while (node != 0) {
            BrickzNode** link = &node->m_8;
            BrickzNode* next = *link;
            BrickzNode* child = (BrickzNode*)node->m_0;
            child->m_14 = m_30;
            child->m_18 = 0;
            m_30->m_18 = child;
            m_30 = child;
            node->m_4 = 0;
            *link = m_40;
            m_40->m_4 = (i32)node;
            m_40 = node;
            node = next;
        }
        cell->m_head = 0;
        cell++;
    }
}

// ---------------------------------------------------------------------------
// CBrickz::CellPop - remove node's bucket slot (node->m_20) from its grid cell's
// doubly-linked bucket list (m_4 = prev, m_8 = next), clear node's links, return
// the slot to the m_40 free list, and (if flag) push node onto the m_30 list.
// @early-stop
// sibling-guard-retest wall (same as Unlink): the 4-way prev/next dispatch keeps
// redundant `cmp prev,0` re-tests in retail that MSVC5 folds with no call to pin
// the flag. Logic byte-correct, container shape proven; parked for the sweep.
RVA(0x0009f710, 0xa7)
void CBrickz::CellPop(BrickzNode* node, i32 flag) {
    BrickzNode** head = &m_8[node->m_4][node->m_0].m_head;
    BrickzNode* slot = node->m_20;
    if ((BrickzNode*)slot->m_4 != 0) {
        if (slot->m_8 != 0) {
            ((BrickzNode*)slot->m_4)->m_8 = slot->m_8;
            slot->m_8->m_4 = slot->m_4;
        }
    } else if (slot->m_8 == 0) {
        *head = 0;
    } else if ((BrickzNode*)slot->m_4 == 0) {
        BrickzNode* next = slot->m_8;
        if (next != 0) {
            *head = next;
            next->m_4 = 0;
        }
    }
    if ((BrickzNode*)slot->m_4 != 0 && slot->m_8 == 0) {
        ((BrickzNode*)slot->m_4)->m_8 = 0;
    }
    node->m_18 = 0;
    node->m_14 = 0;
    node->m_20 = 0;
    slot->m_8 = m_40;
    slot->m_4 = 0;
    m_40->m_4 = (i32)slot;
    m_40 = slot;
    if (flag != 0) {
        node->m_18 = 0;
        node->m_14 = m_30;
        m_30->m_18 = node;
        m_30 = node;
    }
}

// ---------------------------------------------------------------------------
// CBrickz::Find - walk the m_18 lookup list (linked via m_14), return the node
// whose (m_0,m_4) pair equals (key1,key2); 0 if absent.
RVA(0x0009f500, 0x24)
BrickzNode* CBrickz::Find(i32 key1, i32 key2) {
    BrickzNode* p = m_18;
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

// ---------------------------------------------------------------------------
// CBrickz::Drain - move every node off the m_18 list onto the front of the m_30
// list (re-threaded via m_14/m_18), then clear the m_18 head.
// @early-stop
// regalloc wall: retail materializes &node->m_14 as a base ptr in a callee-saved
// reg (lea + 3 pushes); recompile uses a direct offset + 2 pushes. Logic
// byte-correct, ~67% (no source spelling forces the 3rd reg / lea base).
RVA(0x0009f590, 0x2f)
void CBrickz::Drain() {
    BrickzNode* p = m_18;
    if (p != 0) {
        do {
            BrickzNode* next = p->m_14;
            p->m_14 = m_30;
            p->m_18 = 0;
            m_30->m_18 = p;
            m_30 = p;
            p = next;
        } while (p != 0);
    }
    m_18 = 0;
}
