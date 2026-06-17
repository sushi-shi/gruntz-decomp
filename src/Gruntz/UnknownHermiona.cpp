// UnknownHermiona.cpp - two leaf broadcast methods of the tomalla-named ddrawmgr
// sub-manager UnknownHermiona (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// Both methods share ONE shape: walk an intrusive singly-linked list anchored at
// UnknownHermiona+0x14 (each node's first dword is the next-node pointer, and
// node+0x8 holds a child object), dispatching one of the child's sibling virtuals
// with the method's three forwarded args; after the walk, dispatch the object's
// OWN +0x2c virtual with two of those args. The two methods differ only in which
// child virtual the loop calls: +0x34 (Unknown30) vs +0x38 (Unknown34).
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
// virtuals land at +0x34 / +0x38. Declarations only - never defined.
class HermionaChild {
public:
    virtual void Slot00();                              // +0x00
    virtual void Slot04();                              // +0x04
    virtual void Slot08();                              // +0x08
    virtual void Slot0C();                              // +0x0c
    virtual void Slot10();                              // +0x10
    virtual void Slot14();                              // +0x14
    virtual void Slot18();                              // +0x18
    virtual void Slot1C();                              // +0x1c
    virtual void Slot20();                              // +0x20
    virtual void Slot24();                              // +0x24
    virtual void Slot28();                              // +0x28
    virtual void Slot2C();                              // +0x2c
    virtual void Slot30();                              // +0x30
    virtual void Vfunc34(int a1, int a2, int a3);       // +0x34
    virtual void Vfunc38(int a1, int a2, int a3);       // +0x38
};

// One node of the intrusive list at +0x14: next pointer @0, child object @8.
struct HermionaNode {
    HermionaNode  *m_next;          // +0x00
    int            m_04;            // +0x04
    HermionaChild *m_obj;           // +0x08
};

// ---------------------------------------------------------------------------
// UnknownHermiona - only the load-bearing offsets are modeled: the +0x14 list
// anchor and the object's own +0x2c "post-broadcast" virtual. The matched methods
// occupy lower vtable slots (slot numbers not load-bearing, only bodies), so they
// are placed first; the +0x2c slot is padded to land exactly.
// ---------------------------------------------------------------------------
class UnknownHermiona {
public:
    int  VirtualMethodUnknown14();
    void VirtualMethodUnknown30(int a1, int a2, int a3);
    void VirtualMethodUnknown34(int a1, int a2, int a3);

    // --- vtable padding so the post-broadcast self-virtual lands at +0x2c ---
    virtual void Slot00();              // +0x00
    virtual void Slot04();              // +0x04
    virtual void Slot08();              // +0x08
    virtual void Slot0C();              // +0x0c
    virtual void Slot10();              // +0x10
    virtual void Slot14();              // +0x14
    virtual void Slot18();              // +0x18
    virtual void Slot1C();              // +0x1c
    virtual void Slot20();              // +0x20
    virtual void Slot24();              // +0x24
    virtual void Slot28();              // +0x28
    virtual void Vfunc2C(int a1, int a2);   // +0x2c  post-broadcast self virtual

    int           m_04;                 // +0x04  initialized to -1 when inactive
    char          m_pad08[0x0c - 0x08]; // +0x08..0x0b
    int           m_0c;                 // +0x0c  parent/root handle
    char          m_pad10[0x14 - 0x10]; // +0x10..0x13
    HermionaNode *m_14;                 // +0x14  intrusive-list head
};

// ---------------------------------------------------------------------------
// UnknownHermiona::VirtualMethodUnknown14  @0x1575e0  (__thiscall, ret 0)
// Same base readiness predicate used by several Lucius-derived managers.
// ---------------------------------------------------------------------------
// @address: 0x1575e0
// @size:    0x16
int UnknownHermiona::VirtualMethodUnknown14()
{
    if (m_0c == 0)
        goto fail;
    if (m_04 != -1)
        return 1;

fail:
    return 0;
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
// UnknownHermiona::VirtualMethodUnknown30  @0x159cf0  (__thiscall, ret 0xc)
// For each node in the +0x14 list, dispatch child +0x34 with (a1,a2,a3); then
// dispatch this->+0x2c with (a2,a3).
// ---------------------------------------------------------------------------
// @address: 0x159cf0
// @size:    0x42
void UnknownHermiona::VirtualMethodUnknown30(int a1, int a2, int a3)
{
    HermionaNode *n = m_14;
    if (n != 0) {
        do {
            n->m_obj->Vfunc34(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    Vfunc2C(a2, a3);
}

// ---------------------------------------------------------------------------
// UnknownHermiona::VirtualMethodUnknown34  @0x159d40  (__thiscall, ret 0xc)
// As Unknown30 but the loop dispatches child +0x38.
// ---------------------------------------------------------------------------
// @address: 0x159d40
// @size:    0x42
void UnknownHermiona::VirtualMethodUnknown34(int a1, int a2, int a3)
{
    HermionaNode *n = m_14;
    if (n != 0) {
        do {
            n->m_obj->Vfunc38(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    Vfunc2C(a2, a3);
}
