#include <rva.h>
// CDDrawSubMgrDraco.cpp - one leaf cleanup method of the tomalla-named ddrawmgr
// sub-manager CDDrawSubMgrDraco (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see src/Stub/types/ddrawmgr_surface_family.h).
//
// CDDrawSubMgrDraco carries three owned-child pointers at +0x10/+0x14/+0x18 (the three
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
    virtual void FUN_005bef01();      // [0] 0x1bef01 (shared thunk, declared-only)
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// ---------------------------------------------------------------------------
// CDDrawSubMgrDraco - real polymorphic now (own 10-slot vtable ??_7CDDrawSubMgrDraco
// @0x5efe08, was Vtbl_1efe08 / ClassWithUnknownVTable35). Slots 0/2/3/4/6 are the
// shared CObject thunks, slot 1 the virtual dtor (0x1574b0), slot 5 =
// VirtualMethodUnknown14 (0x157480), slot 7 = VirtualMethodUnknown1C (0x158ac0),
// slots 8/9 the backlog stubs. cl auto-emits the vtable; the implicit vptr-stamp
// replaces the explicit m_vptr. Three owned-child pointers at +0x10/+0x14/+0x18.
// ---------------------------------------------------------------------------
class CDDrawSubMgrDraco {
public:
    virtual void FUN_005bef01();           // [0] 0x1bef01 (shared thunk, declared-only)
    virtual ~CDDrawSubMgrDraco();          // [1] 0x1574b0 scalar-deleting dtor
    virtual void FUN_004028ec();           // [2] 0x0028ec (shared thunk, declared-only)
    virtual void FUN_0040106e();           // [3] 0x00106e (shared thunk, declared-only)
    virtual void FUN_00404034();           // [4] 0x004034 (shared thunk, declared-only)
    virtual i32 VirtualMethodUnknown14();  // [5] 0x157480
    virtual void FUN_00401c08();           // [6] 0x001c08 (shared thunk, declared-only)
    virtual void VirtualMethodUnknown1C(); // [7] 0x158ac0
    virtual void Stub_1574a0();            // [8] 0x1574a0 (backlog stub)
    virtual void Stub_1588f0();            // [9] 0x1588f0 (backlog stub)

    // vptr implicit @ +0x00
    char m_pad04[0x10 - 0x04];
    DracoChild* m_10; // +0x10
    DracoChild* m_14; // +0x14
    DracoChild* m_18; // +0x18
};

// ---------------------------------------------------------------------------
// Ready when all three owned child pointers are populated.
RVA(0x00157480, 0x1e)
i32 CDDrawSubMgrDraco::VirtualMethodUnknown14() {
    if (m_14 == 0) {
        goto fail;
    }
    if (m_18 == 0) {
        goto fail;
    }
    if (m_10 != 0) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// For each owned child at +0x10/+0x14/+0x18: if non-null, run its scalar-deleting
// destructor (vtbl +0x4, arg 1) and null the slot.
RVA(0x00158ac0, 0x44)
void CDDrawSubMgrDraco::VirtualMethodUnknown1C() {
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
// @stub
RVA(0x001574a0, 0x6)
void CDDrawSubMgrDraco::Stub_1574a0() {}

// @confidence: high
// @source: tomalla
// @stub
// slot[1] scalar-deleting dtor (0x1574b0); cl auto-stamps the vptr at entry.
RVA(0x001574b0, 0x1e)
CDDrawSubMgrDraco::~CDDrawSubMgrDraco() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x001588f0, 0x1c5)
void CDDrawSubMgrDraco::Stub_1588f0() {}

SIZE_UNKNOWN(CDDrawSubMgrDraco);
SIZE_UNKNOWN(DracoChild);
// ??_7CDDrawSubMgrDraco (was Vtbl_1efe08 / ClassWithUnknownVTable35; 10 slots). cl
// auto-emits it from the real-polymorphic class; retail datum reloc-masked ->
// matching-neutral catalog tracking.
VTBL(CDDrawSubMgrDraco, 0x001efe08);
