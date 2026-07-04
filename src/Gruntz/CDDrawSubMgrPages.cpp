#include <rva.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
// CDDrawSubMgrPages.cpp - one leaf cleanup method of the tomalla-named ddrawmgr
// sub-manager CDDrawSubMgrPages (a CDirectDrawMgr surface/page sub-manager in the
// "DDraw surface manager" family; see docs/ddraw-family-names.md).
//
// CDDrawSubMgrPages carries three owned-child pointers at +0x10/+0x14/+0x18 (the three
// int fields m_10/14/18 of the layout). DestroyChildren is a
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
class CDDrawSurfaceChild {
public:
    virtual void FUN_005bef01();      // [0] 0x1bef01 (shared thunk, declared-only)
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// ---------------------------------------------------------------------------
// CDDrawSubMgrPages - real polymorphic now (own 10-slot vtable ??_7CDDrawSubMgrPages
// @0x5efe08, was Vtbl_1efe08 / the CDDrawSubMgrPages vtable). Slots 0/2/3/4/6 are the
// shared CObject thunks, slot 1 the virtual dtor (0x1574b0), slot 5 =
// IsReady (0x157480), slot 7 = DestroyChildren (0x158ac0),
// slots 8/9 the backlog stubs. cl auto-emits the vtable; the implicit vptr-stamp
// replaces the explicit m_vptr. Three owned-child pointers at +0x10/+0x14/+0x18.
// ---------------------------------------------------------------------------
// operator delete (called by the scalar-deleting dtor under the delete flag).
void operator delete(void*);
inline void* operator new(u32, void* p) {
    return p;
}

// The two spawned-worker vtables, stamped manually into the heap block AFTER the
// external base ctor runs (transitional workaround: the workers' virtuals live in
// other TUs, so cl can't emit these vtables - referenced as reloc-masked DATA externs).
DATA(0x001eff70)
extern i32 g_ddrawSurfaceChildAVtbl; // 0x5eff70
DATA(0x001eff30)
extern i32 g_ddrawSurfacePairVtbl; // 0x5eff30

// The grand-base teardown vftable restored at ~CDDrawSubMgrPages exit (0x5e8cb4;
// the delinked retail dtor names its restamp DIR32 ?g_wapObjectDtorVtbl@@3PAXA, so
// this manual DATA-extern stamp - not a cl-auto ~CObject fold - is what matches).
DATA(0x001e8cb4)
extern void* g_wapObjectDtorVtbl;

// CDDrawSubMgrPagesBase - the CObject-like family grand-base (5-slot vtable masks
// 0x5e8cb4). Slot 1 is a REGULAR virtual (not a C++ dtor) so CDDrawSubMgrPages can
// override it with its RVA-pinned ??_G ScalarDtor (0x1574b0) WITHOUT cl auto-
// generating a clashing ??_G - the (B)-form CLoadable leaf shape (same as
// CDDrawSubMgrGrandBase in CDDrawSubMgrLeaf.cpp). The subobject dtor does ONLY the
// grand-base vptr restamp (the m_04/m_08/m_0c field resets live in the derived
// ~CDDrawSubMgrPages body, so they land after DestroyChildren, matching retail);
// the destructible base is what gives ~CDDrawSubMgrPages its /GX EH frame.
struct CDDrawSubMgrPagesBase {
    virtual void FUN_005bef01();        // [0] 0x1bef01 (shared thunk, declared-only)
    virtual void* ScalarDtor(i32 flag); // [1] scalar-deleting dtor (regular virtual)
    virtual void FUN_004028ec();        // [2] 0x0028ec (shared thunk, declared-only)
    virtual void FUN_0040106e();        // [3] 0x00106e (shared thunk, declared-only)
    virtual void FUN_00404034();        // [4] 0x004034 (shared thunk, declared-only)
    ~CDDrawSubMgrPagesBase();
    CDDrawSubMgrPagesBase() {}
};
inline CDDrawSubMgrPagesBase::~CDDrawSubMgrPagesBase() {
    *(void**)this = &g_wapObjectDtorVtbl;
}

// The two spawned worker types built by CreateChildren (0x1588f0): a 0x30-byte
// "A" child (ctor 0x158f30, vtable 0x5eff70, dispatch +0x24) and two 0x34-byte "B"
// children (ctor 0x156cb0 = CDDrawSubMgr::CDDrawSubMgr, vtable 0x5eff30, dispatch +0x30).
class CDDrawSurfaceChildA {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual i32 Vfunc24(i32 a1, i32 a2, i32 a3);     // slot 9 (@0x24)
    CDDrawSurfaceChildA(i32 handle, i32 a2, i32 a3); // 0x158f30
    char m_pad04[0x2c - 0x04];
    i32 m_2c; // +0x2c
}; // 0x30

class CDDrawSurfacePair {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual i32 Vfunc30(i32 a1, i32 a2, i32 a3, i32 a4); // slot 12 (@0x30)
    CDDrawSurfacePair(i32 handle, i32 a2, i32 a3);       // 0x156cb0
    char m_pad04[0x10 - 0x04];
    i32 m_10; // +0x10
    char m_pad14[0x2c - 0x14];
    i32 m_2c; // +0x2c
    i32 m_30; // +0x30
}; // 0x34

// The parent/root handle object at CDDrawSubMgrPages+0x0c; the factory records an
// error code into its +0x38 field when a child fails.
class CDDrawSurfaceMgr {
public:
    i32 m_pad00[0x0e]; // +0x00..0x37
    i32 m_38;          // +0x38  error code slot
};

// ---------------------------------------------------------------------------
// CDDrawSubMgrPages (retail RTTI ??_7CDDrawSubMgrDraco @0x5efe08 - proven by the
// vtable dump: slot 1 = 0x1574b0 = this ScalarDtor, slot 7 = DestroyChildren
// 0x158ac0). A (B)-form CLoadable leaf: slots 5..9 are its own, slot 1 overrides
// the grand-base's regular-virtual ScalarDtor with the RVA-pinned ??_G.
class CDDrawSubMgrPages : public CDDrawSubMgrPagesBase {
public:
    void* ScalarDtor(i32 flag) OVERRIDE; // [1] ??_G scalar-deleting dtor (0x1574b0)
    virtual i32 IsReady();               // [5] 0x157480
    virtual void FUN_00401c08();         // [6] 0x001c08 (shared thunk, declared-only)
    virtual void DestroyChildren();      // [7] 0x158ac0
    virtual StateId GetStateId();        // [8] 0x1574a0 (state id)
    virtual i32 CreateChildren(i32 a1, i32 a2, i32 a3, i32 a4); // [9] 0x1588f0
    ~CDDrawSubMgrPages();                                       // 0x1574d0 member-teardown

    // vptr @+0x00 (grand-base); the three-word header at +0x04..+0x0c.
    i32 m_04;               // +0x04  (reset to -1 on teardown)
    i32 m_08;               // +0x08  (reset to 0)
    CDDrawSurfaceMgr* m_0c; // +0x0c  parent/root handle (reset to 0)
    CDDrawSurfaceChild*
        m_10; // +0x10  (CDDrawSurfaceChildA object, viewed as CDDrawSurfaceChild by VM1C)
    CDDrawSurfaceChild* m_14; // +0x14  (CDDrawSurfacePair object)
    CDDrawSurfaceChild* m_18; // +0x18  (CDDrawSurfacePair object)
};

// ---------------------------------------------------------------------------
// Ready when all three owned child pointers are populated.
RVA(0x00157480, 0x1e)
i32 CDDrawSubMgrPages::IsReady() {
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
void CDDrawSubMgrPages::DestroyChildren() {
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

// ---------------------------------------------------------------------------
// Constant state id.
RVA(0x001574a0, 0x6)
StateId CDDrawSubMgrPages::GetStateId() {
    return STATE_SUBMGRPAGES; // 0xf
}

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (??_G at 0x1574b0): run the real member-teardown
// ~CDDrawSubMgrPages (0x1574d0, below), then operator delete this if the low flag
// bit is set. Slot 1 override.
SYMBOL(??_GCDDrawSubMgrPages @@UAEPAXI@Z)
RVA(0x001574b0, 0x1e)
void* CDDrawSubMgrPages::ScalarDtor(i32 flag) {
    this->CDDrawSubMgrPages::~CDDrawSubMgrPages();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// Member-teardown destructor (0x1574d0; retail ??1CDDrawSubMgr / ~CDDrawSubMgrDraco).
// cl stamps the derived vftable ??_7CDDrawSubMgrPages (masks 0x5efe08) at entry,
// devirtualizes DestroyChildren (slot 7 -> 0x158ac0) to a direct call, resets the
// three header words, then the grand-base subobject dtor restamps g_wapObjectDtorVtbl
// (0x5e8cb4). The destructible grand-base gives the /GX EH frame. This is the DERIVED
// half of the former mis-modeled "CDDrawSubMgr" (whose ctor 0x156cb0 is the distinct
// CLoadable base-vtable ctor in CDDrawSubMgr.cpp); DestroyChildren is now the correct
// devirtualized callee (retail 0x158ac0), not the old ?OnDestroy@CDDrawSubMgr.
RVA(0x001574d0, 0x5b)
CDDrawSubMgrPages::~CDDrawSubMgrPages() {
    DestroyChildren();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    // ~CDDrawSubMgrPagesBase: *(void**)this = &g_wapObjectDtorVtbl (restamp, last).
}

// ---------------------------------------------------------------------------
// 0x1588f0: build the three owned children then run their per-stage init. Alloc
// child A (0x30, ctor 0x158f30(handle,0,0), vtable 0x5eff70) -> m_10; child B and C
// (0x34, ctor 0x156cb0 = CDDrawSubMgr(handle,N,0), vtable 0x5eff30, m_10=0/m_2c=0/
// m_30=1) -> m_14/m_18. Then A->Vfunc24, B->Vfunc30, and (unless arg4&1) C->Vfunc30,
// each with (a1,a2,a3[,0]); on any failure stamp the root's +0x38 error code
// (0x7d1/0x7d2/0x7d3 if not already set) and return 0. All-ok -> 1. /GX EH frame
// tracks the partially-built children during construction.
// @early-stop
// vptr-position / worker-ctor-shape wall: retail stamps each child's derived vtable
// (0x5eff70 / 0x5eff30) AFTER the base ctor + field seeds (vptr-last); the polymorphic
// `new` model stamps vptr-first, and the two child ctors/vtables are foreign engine
// data (reloc-masked). Logic/CFG/offsets/error-codes reproduced.
RVA(0x001588f0, 0x1c5)
i32 CDDrawSubMgrPages::CreateChildren(i32 a1, i32 a2, i32 a3, i32 a4) {
    CDDrawSurfaceChildA* a = (CDDrawSurfaceChildA*)operator new(0x30);
    if (a != 0) {
        new (a) CDDrawSurfaceChildA((i32)m_0c, 0, 0);
        *(i32**)a = &g_ddrawSurfaceChildAVtbl;
        a->m_2c = 0;
    }
    m_10 = (CDDrawSurfaceChild*)a;

    CDDrawSurfacePair* b = (CDDrawSurfacePair*)operator new(0x34);
    if (b != 0) {
        new (b) CDDrawSurfacePair((i32)m_0c, 1, 0);
        b->m_10 = 0;
        *(i32**)b = &g_ddrawSurfacePairVtbl;
        b->m_2c = 0;
        b->m_30 = 1;
    }
    m_14 = (CDDrawSurfaceChild*)b;

    CDDrawSurfacePair* c = (CDDrawSurfacePair*)operator new(0x34);
    if (c != 0) {
        new (c) CDDrawSurfacePair((i32)m_0c, 2, 0);
        c->m_10 = 0;
        *(i32**)c = &g_ddrawSurfacePairVtbl;
        c->m_2c = 0;
        c->m_30 = 1;
    }
    m_18 = (CDDrawSurfaceChild*)c;

    if (a->Vfunc24(a1, a2, a3) == 0) {
        if (m_0c->m_38 == 0) {
            m_0c->m_38 = 0x7d1;
        }
        return 0;
    }
    if (b->Vfunc30(a1, a2, a3, 0) == 0) {
        if (m_0c->m_38 == 0) {
            m_0c->m_38 = 0x7d2;
        }
        return 0;
    }
    if (!(a4 & 1)) {
        if (c->Vfunc30(a1, a2, a3, 0) == 0) {
            if (m_0c->m_38 == 0) {
                m_0c->m_38 = 0x7d3;
            }
            return 0;
        }
    }
    return 1;
}

SIZE_UNKNOWN(CDDrawSubMgrPages);
SIZE_UNKNOWN(CDDrawSubMgrPagesBase);
SIZE_UNKNOWN(CDDrawSurfaceChild);
SIZE_UNKNOWN(CDDrawSurfaceMgr);
SIZE(CDDrawSurfaceChildA, 0x30);
SIZE(CDDrawSurfacePair, 0x34);
// ??_7CDDrawSubMgrPages (was Vtbl_1efe08 / the CDDrawSubMgrPages vtable; 10 slots). cl
// auto-emits it from the real-polymorphic class; retail datum reloc-masked ->
// matching-neutral catalog tracking.
VTBL(CDDrawSubMgrPages, 0x001efe08);
