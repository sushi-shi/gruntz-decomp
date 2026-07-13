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

// The base intrusive list header: derives the abstract list-interface grand-base
// CObjListBase (vtable 0x1ef760, one __purecall slot - the DESTRUCTION vtable the
// inlined dtor chains of every derived list restamp); head at +4, tail at +8.
// Remove(CObjNode*) @0x1852e0 unlinks a node from the {head,tail} pair. The dtor
// is NON-virtual (no retail list vtable carries a dtor slot) and inline+empty so
// an embedding class's dtor inlines the chain down to the single ??_7CObjListBase
// stamp (/O2 dead-store-eliminates the intermediates - the retail shape).
#include <Bute/ObjListBase.h>
struct CObjList : public CObjListBase {
    // V0 (slot 0) stays pure here - CObjList is only ever a base in the Rez model.
    ~CObjList() {}
    CObjNode* m_head;            // +0x04
    CObjNode* m_tail;            // +0x08
    void Remove(CObjNode* node); // 0x1852e0
};
SIZE_UNKNOWN(CObjList); // {vptr,head,tail}=0xc header; full engine size unproven

// CRezList : public CObjList - THE concrete Rez list (own 1-slot vtable
// ??_7CRezList @0x1ef7c8: [0] = the empty V0 override 0x13c4d0). Adds the
// front/back inserts. AddHead re-reads the head/tail member after writing the
// node's links (the node may alias the header, so MSVC cannot cache the field
// across the store). Leaf pointer-shuffles, no callees. (The former RezMgr.h
// "CRezDirList" - CRezDir's two embedded child lists - was THIS class under a
// second name: the dir ctor stamps 0x5ef7c8 into both members.)
VTBL(CRezList, 0x001ef7c8);
struct CRezList : public CObjList {
    CRezList() {
        m_head = 0;
        m_tail = 0;
    }
    virtual void V0() OVERRIDE; // [0] 0x13c4d0 (empty body; RezFile.cpp)
    // NON-virtual inline dtor: embedding dtors (~CRezDir) inline the chain; the
    // EH-funclet-referenced standalone COMDAT copy is retail 0x13ca30 (bound by
    // RezFile.cpp's @rva-symbol).
    ~CRezList() {}
    void AddHead(CRezListNode* node); // 0x1851e0
    void AddTail(CRezListNode* node); // 0x185210
    // Positional inserts: splice `node` after/before `pos` (null pos -> AddHead /
    // AddTail respectively). Each branch re-reads pos->m_next/m_prev after the
    // aliasing store, and MSVC duplicates the common link tail into both arms.
    void InsertAfter(CRezListNode* pos, CRezListNode* node);  // 0x185240
    void InsertBefore(CRezListNode* pos, CRezListNode* node); // 0x185290
};
SIZE(CRezList, 0xc); // {vptr,head,tail}

#endif // REZ_REZLIST_H
