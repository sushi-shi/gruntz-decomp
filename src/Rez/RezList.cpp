#include <rva.h>

#include <Rez/RezMgr.h> // CRezItmBase - the list's node type (m_next/m_prev at +4/+8)

RVA(0x001851e0, 0x2a)
void CObjList::AddHead(CRezItmBase* node) {
    node->m_next = m_head;
    node->m_prev = 0;
    if (m_head) {
        m_head->m_prev = node;
        m_head = node;
    } else {
        m_tail = node;
        m_head = node;
    }
}

RVA(0x00185210, 0x2a)
void CRezList::AddTail(CRezItmBase* node) {
    node->m_next = 0;
    node->m_prev = m_tail;
    if (m_tail) {
        m_tail->m_next = node;
        m_tail = node;
    } else {
        m_head = node;
        m_tail = node;
    }
}

// Splice `node` in after `pos` (null pos -> AddHead, then fall through). When `pos`
// is the tail the list's m_tail is retargeted; otherwise pos->m_next's back-link is
// fixed. pos/node/this stay live across the AddHead call (no early return), so MSVC
// pins them in ebx/esi/edi.
// The three-store link tail is written ONCE, factored out of both arms - cl duplicates
// it into each arm itself (two epilogues, retail's layout). Hand-duplicating it in the
// source was the old 99.68% "regalloc wall": in the copy cl wrote for the m_tail arm it
// re-read pos->m_next into ecx (eax was pinned to the known-0 tested value) where retail
// re-reads into eax. The factored form emits retail's eax in both copies. Now EXACT.
RVA(0x00185240, 0x48)
void CRezList::InsertAfter(CRezItmBase* pos, CRezItmBase* node) {
    if (pos == 0) {
        AddHead(node);
    }
    if (pos->m_next != 0) {
        pos->m_next->m_prev = node;
    } else {
        m_tail = node;
    }
    node->m_prev = pos;
    node->m_next = pos->m_next;
    pos->m_next = node;
}

// Splice `node` in before `pos` (null pos -> AddTail, then fall through). When `pos`
// is the head the list's m_head is retargeted; otherwise pos->m_prev's forward-link
// is fixed. Same live-across-call shape as InsertAfter, and the same factored link
// tail (see the note above - hand-duplicating it costs retail's eax scratch). Now EXACT.
RVA(0x00185290, 0x48)
void CRezList::InsertBefore(CRezItmBase* pos, CRezItmBase* node) {
    if (pos == 0) {
        AddTail(node);
    }
    if (pos->m_prev != 0) {
        pos->m_prev->m_next = node;
    } else {
        m_head = node;
    }
    node->m_next = pos;
    node->m_prev = pos->m_prev;
    pos->m_prev = node;
}

RVA(0x001852e0, 0x35)
void CObjList::Remove(CRezItmBase* node) {
    if (node->m_prev) {
        node->m_prev->m_next = node->m_next;
    } else {
        m_head = node->m_next;
    }
    if (node->m_next) {
        node->m_next->m_prev = node->m_prev;
    } else {
        m_tail = node->m_prev;
    }
}
