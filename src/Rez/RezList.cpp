#include <rva.h>

#include <Rez/RezList.h>

RVA(0x001851e0, 0x2a)
void CObjList::AddHead(CObjNode* node) {
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
void CRezList::AddTail(CRezListNode* node) {
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
// pins them in ebx/esi/edi and duplicates the link tail into both arms.
// @early-stop
// regalloc wall (99.68%): byte-exact except the else (m_tail) arm re-reads
// pos->m_next into ecx where retail reuses eax (the tested register stays hot in the
// non-tail arm but goes cold after the m_tail store). Permuter confirms no steer.
RVA(0x00185240, 0x48)
void CRezList::InsertAfter(CRezListNode* pos, CRezListNode* node) {
    if (pos == 0) {
        AddHead(node);
    }
    if (pos->m_next != 0) {
        pos->m_next->m_prev = node;
        node->m_prev = pos;
        node->m_next = pos->m_next;
        pos->m_next = node;
    } else {
        m_tail = node;
        node->m_prev = pos;
        node->m_next = pos->m_next;
        pos->m_next = node;
    }
}

// Splice `node` in before `pos` (null pos -> AddTail, then fall through). When `pos`
// is the head the list's m_head is retargeted; otherwise pos->m_prev's forward-link
// is fixed. Same live-across-call shape as InsertAfter.
// @early-stop
// regalloc wall (99.68%): byte-exact except the else (m_head) arm re-reads
// pos->m_prev into ecx where retail reuses eax (mirror of InsertAfter). Permuter
// confirms no steer.
RVA(0x00185290, 0x48)
void CRezList::InsertBefore(CRezListNode* pos, CRezListNode* node) {
    if (pos == 0) {
        AddTail(node);
    }
    if (pos->m_prev != 0) {
        pos->m_prev->m_next = node;
        node->m_next = pos;
        node->m_prev = pos->m_prev;
        pos->m_prev = node;
    } else {
        m_head = node;
        node->m_next = pos;
        node->m_prev = pos->m_prev;
        pos->m_prev = node;
    }
}

RVA(0x001852e0, 0x35)
void CObjList::Remove(CObjNode* node) {
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
