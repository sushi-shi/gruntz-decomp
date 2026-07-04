// RezList.h - the Rez subsystem's intrusive doubly-linked list (C:\Proj\...\Rez).
// The list HEADER is polymorphic (implicit vptr @+0) with head/tail at +4/+8; the
// NODES carry their own next/prev links at +4/+8 (and an element base at +0).
//
// Real hierarchy (folds the placeholder list symbols): CRezList : public CObjList.
// The base CObjList owns Remove (0x1852e0, unlink); CRezList adds the front/back
// inserts AddHead (0x1851e0) / AddTail (0x185210). All are non-virtual __thiscall
// (ret 4) and never touch +0. Offsets + code bytes are load-bearing.
#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>
#include <rva.h>

// An intrusive list NODE: next at +4, prev at +8. +0 is the element's own base (a
// vptr for a polymorphic element, else data) - never touched by the list ops.
// Named CObjNode to match the base list's Remove(CObjNode*) parameter (0x1852e0).
// Non-polymorphic here (a plain {base,next,prev} view; Remove only touches +4/+8).
class CObjNode {
public:
    void* m_base;     // +0x00  element base (vptr for a polymorphic element, else data)
    CObjNode* m_next; // +0x04
    CObjNode* m_prev; // +0x08
};
SIZE_UNKNOWN(CObjNode); // a view of the (variably-sized) list elements

// AddHead/AddTail's node parameter (0x1851e0 / 0x185210). The SAME physical node,
// but declared `struct` (mangles PAU where CObjNode mangles PAV) - the intrusive-list
// ops were typed for two node views that ICF-folded to these addresses. A trivial
// alias so an element deriving CRezListNode also converts to CObjNode* (Remove) with
// no cross-cast.
struct CRezListNode : public CObjNode {};
SIZE_UNKNOWN(CRezListNode);

// The base intrusive list header: polymorphic (implicit vptr @+0), head at +4, tail
// at +8. Remove(CObjNode*) @0x1852e0 unlinks a node from the {head,tail} pair
// (defined in Bute/SymParser.cpp; external no-body here so its call reloc-masks).
struct CObjList {
    virtual ~CObjList();         // +0x00  vptr (external no-body dtor)
    CObjNode* m_head;            // +0x04
    CObjNode* m_tail;            // +0x08
    void Remove(CObjNode* node); // 0x1852e0
};
SIZE_UNKNOWN(CObjList); // {vptr,head,tail}=0xc header; full engine size unproven

// CRezList : public CObjList - adds the front/back inserts. AddHead re-reads the
// head/tail member after writing the node's links (the node may alias the header, so
// MSVC cannot cache the field across the store). Leaf pointer-shuffles, no callees.
struct CRezList : public CObjList {
    void AddHead(CRezListNode* node); // 0x1851e0
    void AddTail(CRezListNode* node); // 0x185210
};
SIZE_UNKNOWN(CRezList); // {vptr,head,tail}=0xc header; full engine size unproven

#endif // REZ_REZLIST_H
