#include <Dsndmgr/SoundVoiceList.h>
#include <rva.h>

RVA(0x001390e0, 0x25)
void DSoundList::InsertHead(DSoundLink* node) {
    node->m_next = m_head;
    node->m_prev = 0;
    if (m_head) {
        m_head->m_prev = node;
    } else {
        m_tail = node;
    }
    m_head = node;
}

RVA(0x00139110, 0x27)
void DSoundList::InsertTail(DSoundLink* node) {
    node->m_next = 0;
    node->m_prev = m_tail;
    if (m_tail) {
        m_tail->m_next = node;
    } else {
        m_head = node;
    }
    m_tail = node;
}

RVA(0x00139140, 0x41)
void DSoundList::InsertAfter(DSoundLink* after, DSoundLink* node) {
    if (after == 0) {
        InsertHead(node);
    }
    if (after->m_next) {
        after->m_next->m_prev = node;
    } else {
        m_tail = node;
    }
    node->m_prev = after;
    node->m_next = after->m_next;
    after->m_next = node;
}

RVA(0x00139190, 0x44)
void DSoundList::InsertBefore(DSoundLink* before, DSoundLink* node) {
    if (before == 0) {
        InsertTail(node);
    }
    if (before->m_prev) {
        before->m_prev->m_next = node;
    } else {
        m_head = node;
    }
    node->m_next = before;
    node->m_prev = before->m_prev;
    before->m_prev = node;
}

RVA(0x001391e0, 0x30)
void DSoundList::Unlink(DSoundLink* node) {
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
