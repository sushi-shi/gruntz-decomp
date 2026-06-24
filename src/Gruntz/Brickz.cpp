// Brickz.cpp - reconstruction of the trace-discovered "CBrickz" cluster.
//
// Placeholder class name (see <Gruntz/Brickz.h>): these are __thiscall pointer-
// shuffle ops over a self-contained graph/grid container's intrusive node lists.
// They match by shape; field names are placeholders, offsets are load-bearing.
#include <rva.h>

#include <Gruntz/Brickz.h>

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
// @early-stop
// regalloc wall: only residual is a head<->next register swap (retail pins head
// in eax, recompile lands it in edx); logic byte-correct, 97% (no source steer).
RVA(0x0009f430, 0x2a)
void CBrickz::PopFront() {
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
