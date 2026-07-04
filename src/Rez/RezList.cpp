// RezList.cpp - the Rez subsystem's intrusive doubly-linked list (C:\Proj\...\Rez).
// AddHead / AddTail link a node by its +4/+8 intrusive links. Both re-read the
// head/tail member after writing the node's links (the node may alias the header, so
// MSVC cannot cache the field across the store). Leaf pointer-shuffles, no callees.
#include <rva.h>

#include <Rez/RezList.h>

// Insert node at the front (node->next = head; node->prev = 0).
RVA(0x001851e0, 0x2a)
void CRezList::AddHead(CRezListNode* node) {
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

// Insert node at the back (node->next = 0; node->prev = tail).
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
