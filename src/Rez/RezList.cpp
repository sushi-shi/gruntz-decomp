#include <rva.h>

#include <Rez/RezMgr.h>

#include <stddef.h>

RVA(0x001851e0, 0x2a)
void CObjList::AddHead(CRezItmBase* node) {
    node->m_next = m_head;
    node->m_prev = NULL;
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
    node->m_next = NULL;
    node->m_prev = m_tail;
    if (m_tail) {
        m_tail->m_next = node;
        m_tail = node;
    } else {
        m_head = node;
        m_tail = node;
    }
}

RVA(0x00185240, 0x48)
void CRezList::InsertAfter(CRezItmBase* pos, CRezItmBase* node) {
    if (pos == NULL) {
        AddHead(node);
    }
    if (pos->m_next != NULL) {
        pos->m_next->m_prev = node;
    } else {
        m_tail = node;
    }
    node->m_prev = pos;
    node->m_next = pos->m_next;
    pos->m_next = node;
}

RVA(0x00185290, 0x48)
void CRezList::InsertBefore(CRezItmBase* pos, CRezItmBase* node) {
    if (pos == NULL) {
        AddTail(node);
    }
    if (pos->m_prev != NULL) {
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
