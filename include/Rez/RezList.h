// RezList.h - the Rez subsystem's intrusive doubly-linked list (C:\Proj\...\Rez).
// A polymorphic header (implicit vptr @+0) whose head/tail live at +4/+8; elements
// carry their own next/prev links at +4/+8. AddHead / AddTail / Remove are
// non-virtual __thiscall (ret 4) and never touch +0. This is the SAME list class
// as CObjList in RezFile.h (both AddHead/Append resolve to the engine 0x1851e0);
// CObjList is its CRezFile-typed view. Offsets + code bytes are load-bearing.
#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>
#include <rva.h>

// An element's intrusive links: next at +4, prev at +8. The +0 slot is the
// element's own base (a vptr for a polymorphic element, else data) - never touched
// by the list ops, so it stays a generic element-base pointer.
struct CRezListNode {
    void* m_base;         // +0x00  element base (vptr for a polymorphic element)
    CRezListNode* m_next; // +0x04
    CRezListNode* m_prev; // +0x08
};
SIZE_UNKNOWN(CRezListNode); // a view of the (variably-sized) list elements

// The list header: polymorphic (implicit vptr @+0), head at +4, tail at +8.
struct CRezList {
    virtual ~CRezList();  // +0x00  vptr (external no-body dtor; matches CObjList)
    CRezListNode* m_head; // +0x04
    CRezListNode* m_tail; // +0x08

    void AddHead(CRezListNode* node); // 0x1851e0
    void AddTail(CRezListNode* node); // 0x185210
};
SIZE_UNKNOWN(CRezList); // {vptr,head,tail}=0xc header; full engine size unproven

#endif // REZ_REZLIST_H
