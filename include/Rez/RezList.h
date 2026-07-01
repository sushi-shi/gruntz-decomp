// RezList.h - the Rez subsystem's intrusive doubly-linked list (C:\Proj\...\Rez).
// A header object whose head/tail live at +4/+8 and whose elements carry their own
// next/prev links at +4/+8. AddHead / AddTail / Remove are non-virtual __thiscall
// (ret 4). Offsets + code bytes are load-bearing; names are descriptive placeholders.
#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>
#include <rva.h>

// An element's intrusive links: next at +4, prev at +8.
struct CRezListNode {
    void* m_0;
    CRezListNode* m_next; // +0x04
    CRezListNode* m_prev; // +0x08
};
SIZE_UNKNOWN(CRezListNode);

// The list header: head at +4, tail at +8.
struct CRezList {
    void* m_0;
    CRezListNode* m_head; // +0x04
    CRezListNode* m_tail; // +0x08

    void AddHead(CRezListNode* node); // 0x1851e0
    void AddTail(CRezListNode* node); // 0x185210
};
SIZE_UNKNOWN(CRezList);

#endif // REZ_REZLIST_H
