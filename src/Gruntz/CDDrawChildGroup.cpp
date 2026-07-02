#include <rva.h>
// CDDrawChildGroup.cpp - six leaf methods of the tomalla-named ddrawmgr
// sub-manager CDDrawChildGroup (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see src/Stub/types/ddrawmgr_surface_family.h).
//
// All six share ONE shape: walk an intrusive singly-linked list anchored at
// CDDrawChildGroup+0x14 (each node's first dword is the next-node pointer, and
// node+0x8 holds a child object), dispatching one of the child's sibling virtuals
// with a varying number of forwarded args. Some methods also dispatch the
// object's OWN +0x2c virtual after the walk.
//
// The children are:
//   +0x1c -> tail-call thunk to the object's own +0x3c virtual (no list walk)
//   +0x28 -> per-node child Slot2C(a1)                            [1 arg]
//   +0x2c -> per-node child Slot30(a1,a2)                          [2 args]
//   +0x30 -> per-node child Vfunc34(a1,a2,a3), then this->Vfunc2C  [3 args]
//   +0x34 -> per-node child Vfunc38(a1,a2,a3), then this->Vfunc2C  [3 args]
//   +0x38 -> per-node child field_0xd8 = -1 (no vtable dispatch)   [0 args]
//
// Plain /O2 /MT leaves: NO SEH frame, NO relocations - they touch only the +0x14
// list anchor, the node next/object offsets, and sibling/child vtables. Field
// names are placeholders; only OFFSETS + emitted code bytes are load-bearing.
//
// The child and "self" virtuals are modeled as a polymorphic stub (virtuals at
// the right slots) ONLY so each `p->Vfunc(...)` lowers to the exact
// `mov eax,[p]; call [eax+slot]` __thiscall dispatch; the stubs' virtuals are
// never defined, so no vtable is emitted in this TU.
// ---------------------------------------------------------------------------

// The child object dispatched per list node. Slots laid out so the broadcast
// virtuals land at +0x34 / +0x38, with +0x2c and +0x30 used by other methods.
// Declarations only - never defined.
#include <Gruntz/CObList.h>
#include <Gruntz/CMapStringToOb.h>

// The object reached via m_parent->+0x24->+0x5c; its 0x1628d0 method is run by the
// ClearAll cleanup (0x1591f0).
class HermRootObj {
public:
    void Method_1628d0(); // 0x1628d0 (__thiscall)
};

class HermionaChild {
public:
    virtual void Slot00();                        // +0x00
    virtual i32 ScalarDtor(i32 flag);             // +0x04  scalar-deleting destructor
    virtual void Slot08();                        // +0x08
    virtual void Slot0C();                        // +0x0c
    virtual void Slot10();                        // +0x10
    virtual void Slot14();                        // +0x14
    virtual void Slot18();                        // +0x18
    virtual void Slot1C();                        // +0x1c
    virtual void Slot20();                        // +0x20
    virtual void Slot24();                        // +0x24
    virtual void Slot28();                        // +0x28
    virtual void Slot2C(i32 a1);                  // +0x2c
    virtual void Slot30(i32 a1, i32 a2);          // +0x30
    virtual void Vfunc34(i32 a1, i32 a2, i32 a3); // +0x34
    virtual void Vfunc38(i32 a1, i32 a2, i32 a3); // +0x38

    // Data member used by VirtualMethodUnknown38 (write to +0xd8).
    // vtable pointer at +0x00 (4 B); pad from +0x04 to +0xd7.
    char m_pad04[0xd8 - 4];
    i32 m_d8; // +0xd8
};

// One node of the intrusive list at +0x14: next pointer @0, child object @8.
struct HermionaNode {
    HermionaNode* m_next; // +0x00
    i32 m_04;             // +0x04
    HermionaChild* m_obj; // +0x08
};

// ---------------------------------------------------------------------------
// CDDrawChildGroup - only the load-bearing offsets are modeled: the +0x14 list
// anchor and the vtable slots used by leaf methods.  The matched methods
// occupy lower vtable slots (slot numbers not load-bearing, only bodies), so
// they are placed first; the slot sequence from +0x1c through +0x3c is
// padded around the real virtuals so each lands at the correct offset.
// ---------------------------------------------------------------------------
class CDDrawChildGroup {
public:
    i32 VirtualMethodUnknown14();
    void VirtualMethodUnknown30(i32 a1, i32 a2, i32 a3);
    void VirtualMethodUnknown34(i32 a1, i32 a2, i32 a3);

    // --- vtable padding so the leaf virtuals land at their target slots ---
    virtual void Slot00();                               // +0x00
    virtual void Slot04();                               // +0x04
    virtual void Slot08();                               // +0x08
    virtual void Slot0C();                               // +0x0c
    virtual void Slot10();                               // +0x10
    virtual void Slot14();                               // +0x14
    virtual void Slot18();                               // +0x18
    virtual void VirtualMethodUnknown1C();               // +0x1c  thunk -> +0x3c
    virtual void Slot20();                               // +0x20
    virtual void Slot24();                               // +0x24
    virtual void VirtualMethodUnknown28(i32 a1);         // +0x28
    virtual void VirtualMethodUnknown2C(i32 a1, i32 a2); // +0x2c
    virtual void Slot30();                               // +0x30
    virtual void Slot34();                               // +0x34
    virtual void VirtualMethodUnknown38();               // +0x38
    virtual void Slot3C();                               // +0x3c  (referenced by +0x1c thunk)

    i32 m_status;              // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_parent;              // +0x0c  parent/root handle
    char m_pad10[0x14 - 0x10]; // +0x10..0x13
    HermionaNode* m_head;      // +0x14  intrusive-list head

    // Engine-label backlog stubs.
    void Stub_1591f0();
    void Stub_159a70();
};

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several Lucius-derived managers.
RVA(0x001575e0, 0x16)
i32 CDDrawChildGroup::VirtualMethodUnknown14() {
    if (m_parent == 0) {
        goto fail;
    }
    if (m_status != -1) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// Thunk: tail-calls the object's own virtual at vtable slot +0x3c.
RVA(0x001591e0, 0x5)
void CDDrawChildGroup::VirtualMethodUnknown1C() {
    this->Slot3C();
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list dispatching child->Slot2C(a1) per node. No post-loop
// dispatch.
//
// RESIDUE: same loop-advance scheduling plateau as Unknown30/34 — see comment
// below for details.
RVA(0x00159c90, 0x23)
void CDDrawChildGroup::VirtualMethodUnknown28(i32 a1) {
    HermionaNode* n = m_head;
    if (n != 0) {
        do {
            HermionaNode* cur = n;
            n = n->m_next;
            cur->m_obj->Slot2C(a1);
        } while (n != 0);
    }
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list dispatching child->Slot30(a1,a2) per node. No post-loop
// dispatch.
RVA(0x00159cc0, 0x2a)
void CDDrawChildGroup::VirtualMethodUnknown2C(i32 a1, i32 a2) {
    HermionaNode* n = m_head;
    if (n != 0) {
        do {
            HermionaNode* cur = n;
            n = n->m_next;
            cur->m_obj->Slot30(a1, a2);
        } while (n != 0);
    }
}

// Walks the +0x14 list, calling child->Vfunc34(a1,a2,a3) per node, then the
// object's own +0x2c virtual with (a2,a3). Written out in full below rather than
// sharing a helper (a function-pointer slot would not reproduce the direct
// `call [eax+0x34]`).
//
// RESIDUE (~89%, NOT a logic/offset/type/CFG error - documented store/load-
// scheduling entropy, see docs/matching-patterns.md "optimizer reorders field
// stores" and match-learnings.md loop-advance plateaus): in the per-node loop the
// target keeps the live node in ESI across the virtual call and advances it at the
// BOTTOM (`mov ecx,[esi+8]` obj-load; ...; call; `mov esi,[esi]` advance-after).
// MSVC5/c2.dll on this source instead floats the next-pointer load ABOVE the call
// via a node copy (`mov eax,esi; mov esi,[esi]; ...; mov ecx,[eax+8]`). Every loop
// form tried (for / while / do-while, with and without an explicit obj temp, and
// advancing before vs after the dispatch) yielded the same 4-instruction slip; the
// register set, offsets, arg order, and CFG are byte-exact otherwise. Left as the
// plateau - both methods share it identically.

// ---------------------------------------------------------------------------
// For each node in the +0x14 list, dispatch child +0x34 with (a1,a2,a3); then
// dispatch this->+0x2c with (a2,a3).
RVA(0x00159cf0, 0x42)
void CDDrawChildGroup::VirtualMethodUnknown30(i32 a1, i32 a2, i32 a3) {
    HermionaNode* n = m_head;
    if (n != 0) {
        do {
            n->m_obj->Vfunc34(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    VirtualMethodUnknown2C(a2, a3);
}

// ---------------------------------------------------------------------------
// As Unknown30 but the loop dispatches child +0x38.
RVA(0x00159d40, 0x42)
void CDDrawChildGroup::VirtualMethodUnknown34(i32 a1, i32 a2, i32 a3) {
    HermionaNode* n = m_head;
    if (n != 0) {
        do {
            n->m_obj->Vfunc38(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    VirtualMethodUnknown2C(a2, a3);
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list setting each child's field at +0xd8 to -1. No vtable
// dispatch, no stack args.
RVA(0x00159d90, 0x1c)
void CDDrawChildGroup::VirtualMethodUnknown38() {
    HermionaNode* n = m_head;
    if (n != 0) {
        do {
            HermionaNode* cur = n;
            n = n->m_next;
            cur->m_obj->m_d8 = -1;
        } while (n != 0);
    }
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// 0x1591f0: ClearAll cleanup - run m_parent->+0x24->+0x5c->0x1628d0 (when present),
// walk the +0x10 CObList destroying each node's child via its scalar-deleting
// destructor, then RemoveAll the +0x10 list and the +0x2c / +0x48 collections.
RVA(0x001591f0, 0x54)
void CDDrawChildGroup::Stub_1591f0() {
    void* p = *(void**)((char*)m_parent + 0x24);
    if (p != 0) {
        HermRootObj* q = *(HermRootObj**)((char*)p + 0x5c);
        if (q != 0) {
            q->Method_1628d0();
        }
    }
    HermionaNode* n = m_head;
    while (n != 0) {
        HermionaNode* cur = n;
        n = n->m_next;
        HermionaChild* obj = cur->m_obj;
        if (obj != 0) {
            obj->ScalarDtor(1);
        }
    }
    ((CObList*)((char*)this + 0x10))->RemoveAll();
    ((CMapStringToOb*)((char*)this + 0x2c))->RemoveAll();
    ((CMapStringToOb*)((char*)this + 0x48))->RemoveAll();
}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00159a70, 0x200)
void CDDrawChildGroup::Stub_159a70() {}

SIZE_UNKNOWN(CDDrawChildGroup);
SIZE_UNKNOWN(HermionaChild);
SIZE_UNKNOWN(HermionaNode);
SIZE_UNKNOWN(HermRootObj);
