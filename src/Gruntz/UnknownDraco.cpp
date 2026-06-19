// UnknownDraco.cpp - one leaf cleanup method of the tomalla-named ddrawmgr
// sub-manager UnknownDraco (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// UnknownDraco carries three owned-child pointers at +0x10/+0x14/+0x18 (the three
// int fields fieldUnknown10/14/18 of the layout). VirtualMethodUnknown1C is a
// destruct/reset hook: for each of the three pointers, if non-null it dispatches
// the child's scalar-deleting destructor (vtable slot +0x4, delete-flag arg 1)
// and then nulls the slot. Plain /O2 /MT leaf: NO SEH frame, NO relocations -
// it only touches its own member offsets and the children's own vtables.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine). The children are modeled as a polymorphic
// stub (virtuals at the right slots) ONLY so `child->ScalarDtor(1)` lowers to the
// exact `mov eax,[child]; push 1; call [eax+4]` __thiscall dispatch; the stub's
// virtuals are never defined, so no vtable is emitted in this TU.
// ---------------------------------------------------------------------------

// The owned-child interface: only the scalar-deleting destructor slot (+0x04)
// is load-bearing. Declarations only - never defined, so no ??_7 is emitted here.
class DracoChild {
public:
    virtual void Slot00();              // +0x00
    virtual int  ScalarDtor(int flag);  // +0x04  scalar-deleting destructor
};

// ---------------------------------------------------------------------------
// UnknownDraco - only the load-bearing offsets are modeled: three owned-child
// pointers at +0x10/+0x14/+0x18. The matched method occupies a lower vtable slot
// (its slot number is not load-bearing, only its body is matched), placed last.
// ---------------------------------------------------------------------------
class UnknownDraco {
public:
    int  VirtualMethodUnknown14();
    void VirtualMethodUnknown1C();

    void       *m_vptr;     // +0x00 (vptr; not touched here)
    char        m_pad04[0x10 - 0x04];
    DracoChild *m_10;       // +0x10
    DracoChild *m_14;       // +0x14
    DracoChild *m_18;       // +0x18

    // Engine-label backlog stubs.
    void Stub_1574a0();
    void Stub_1574b0();
    void Stub_1588f0();
};

// ---------------------------------------------------------------------------
// UnknownDraco::VirtualMethodUnknown14  @0x157480  (__thiscall, ret 0)
// Ready when all three owned child pointers are populated.
// ---------------------------------------------------------------------------
// @address: 0x157480
// @size:    0x1e
int UnknownDraco::VirtualMethodUnknown14()
{
    if (m_14 == 0)
        goto fail;
    if (m_18 == 0)
        goto fail;
    if (m_10 != 0)
        return 1;

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::VirtualMethodUnknown1C  @0x158ac0  (__thiscall, ret 0)
// For each owned child at +0x10/+0x14/+0x18: if non-null, run its scalar-deleting
// destructor (vtbl +0x4, arg 1) and null the slot.
// ---------------------------------------------------------------------------
// @address: 0x158ac0
// @size:    0x44
void UnknownDraco::VirtualMethodUnknown1C()
{
    if (m_10 != 0) {
        m_10->ScalarDtor(1);
        m_10 = 0;
    }
    if (m_14 != 0) {
        m_14->ScalarDtor(1);
        m_14 = 0;
    }
    if (m_18 != 0) {
        m_18->ScalarDtor(1);
        m_18 = 0;
    }
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: tomalla
// @address: 0x1574a0
// @size:    0x6
// @stub
void UnknownDraco::Stub_1574a0() {}

// @confidence: high
// @source: tomalla
// @address: 0x1574b0
// @size:    0x1e
// @stub
void UnknownDraco::Stub_1574b0() {}

// @confidence: high
// @source: tomalla
// @address: 0x1588f0
// @size:    0x1c5
// @stub
void UnknownDraco::Stub_1588f0() {}
