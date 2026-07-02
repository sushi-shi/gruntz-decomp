#include <rva.h>
// CDDrawWorkerList.cpp - four leaf factory methods of the tomalla-named class
// CDDrawWorkerList (a CDirectDrawMgr surface/page sub-manager in the ddrawmgr
// "DDraw surface manager" family; see src/Stub/types/ddrawmgr_surface_family.h).
//
// All four share ONE shape: allocate a 0x7c-byte "worker" object with the global
// operator new, inline-construct it (zero/seed its
// fields + stamp its vftable), then call one of the worker's own sibling virtuals
// forwarding the caller's args. On a 0 result the worker is destroyed via its
// scalar-deleting destructor (vtable +0x4, arg 1) and the method returns 0; on a
// nonzero result the worker is appended to CDDrawWorkerList's CObList (+0x10) - either
// AddHead or AddTail, selected by a trailing bool arg - and the worker is returned.
//
// Two distinct worker vftables appear: one used by Unknown24, whose +0x78
// flag is a BYTE, and one used by Unknown28/2C/30, whose +0x78 flag is an
// int. Both worker layouts are otherwise identical and both carry a scalar-
// deleting destructor at vtable slot +0x4 (the delinker tags slot +0x4 of each as
// the MFC delete-this thunk). The vftables are foreign
// engine data, referenced here as named `DATA(...)` DIR32 externs and stamped
// manually into the raw heap block; the worker classes are modeled as polymorphic
// (virtuals at the right slots) ONLY so `worker->Vfunc(...)` lowers to the exact
// `mov eax,[obj]; call [eax+slot]` __thiscall dispatch - their virtuals are never
// defined, so no vtable is emitted in this TU.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine). These are plain /O2 /MT leaves:
// NO SEH frame; the only relocations are the two reloc-masked rel32 library calls
// (operator new / CObList::AddHead/AddTail) and the DIR32 worker-vftable stores.
// ---------------------------------------------------------------------------

// --- MFC placeholders (only the call symbols / the 0x10 CObList offset matter) --
struct __POSITION;
class CObject;

// CObList lives at CDDrawWorkerList+0x10. AddHead/AddTail are out-of-line NAFXCW
// thunks (reloc-masked rel32 calls); declared with the exact MFC signatures so
// clang mangles them to the MFC-canonical names.
// RemoveAll / RemoveAt are called by VirtualMethodUnknown1C and Unknown34.
#include <Gruntz/CObList.h>

// The worker virtual interface. Slots are laid out so the dispatched methods land
// at the byte offsets the target uses:
//   +0x04 (slot 1)  scalar-deleting destructor (delete-flag arg)
//   +0x2c (slot 11) Vfunc2C(3 args)   +0x30 (slot 12) Vfunc30(4 args)
//   +0x34 (slot 13) Vfunc34(4 args)
// Only declarations - never defined, so no ??_7 vtable is emitted here; the real
// vtable is the foreign engine datum stamped into the object below.
class CDDrawWorkerBase {
public:
    // Slot RVAs named per CDDrawWorkerB (0x1efed0, 14 slots); CDDrawWorkerA
    // (0x1efea0, 12 slots) reuses this base but its slots 5/7/8/10/11 differ.
    virtual void FUN_005bef01();                         // [0] 0x1bef01
    virtual i32 ScalarDtor(i32 flag);                    // [1] scalar-deleting destructor
    virtual void FUN_004028ec();                         // [2] 0x0028ec
    virtual void FUN_0040106e();                         // [3] 0x00106e
    virtual void FUN_00404034();                         // [4] 0x004034
    virtual void FUN_00557200();                         // [5] 0x157200 (B)
    virtual void FUN_00401c08();                         // [6] 0x001c08
    virtual void FUN_00557310();                         // [7] 0x157310 (B)
    virtual void FUN_00557210();                         // [8] 0x157210 (B)
    virtual void FUN_00557080();                         // [9] 0x157080
    virtual void FUN_005660b0();                         // [10] 0x1660b0 (B)
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3);         // [11] 0x1572f0
    virtual i32 Vfunc30(i32 a1, i32 a2, i32 a3, i32 a4); // [12] 0x1572b0
    virtual i32 Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4); // [13] 0x157280
};

// The 0x7c-byte worker layouts. Only the seeded offsets are load-bearing; m_78 is
// the one field whose store width differs between the two workers (BYTE vs int).
// Real polymorphic: `new CDDrawWorkerB/A` makes cl auto-emit ??_7CDDrawWorkerB/A
// (masks the retail vtables 0x5efed0 / 0x5efea0) and stamp the vptr in the ctor -
// no manual `*(void**)w = &g_ddrawWorkerVtbl*` store (ALL-VTABLES mandate).
struct CDDrawWorkerB : public CDDrawWorkerBase {
    CDDrawWorkerB() {}
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_parent; // +0x0c  = parent CDDrawWorkerList::m_pSurfaceMgr (+0xc)
    char _pad10[0x20 - 0x10];
    i32 m_20; // +0x20  = 0x80000000
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38  = -1
    i32 m_3c; // +0x3c  = 0
    i32 m_40; // +0x40  = 0
    char _pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  = 0x80000000
    char _pad60[0x64 - 0x60];
    i32 m_64; // +0x64  = 0x80000000
    char _pad68[0x78 - 0x68];
    i32 m_78; // +0x78  = 0 (int flag for the int-flag workers)
}; // 0x7c

struct CDDrawWorkerA : public CDDrawWorkerBase {
    CDDrawWorkerA() {}
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_parent; // +0x0c
    char _pad10[0x20 - 0x10];
    i32 m_20; // +0x20  = 0x80000000
    char _pad24[0x38 - 0x24];
    i32 m_38; // +0x38  = -1
    i32 m_3c; // +0x3c  = 0
    i32 m_40; // +0x40  = 0
    char _pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  = 0x80000000
    char _pad60[0x64 - 0x60];
    i32 m_64; // +0x64  = 0x80000000
    char _pad68[0x78 - 0x68];
    char m_78; // +0x78  = 0 (BYTE flag for the BYTE-flag worker)
    char _pad79[0x7c - 0x79];
}; // 0x7c

// The child type used in the work-node linked-list at +0x14.
// Virtual slot +0x28 (Vfunc28) is dispatched in Unknown34; +0x74 is a
// reference count. Slot layout matches the child class in the engine.
class CDDrawWorkerItem {
public:
    virtual void Slot00();
    virtual i32 ScalarDtor(i32 flag); // +0x04
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual i32 Vfunc28(i32 a1, i32 a2); // +0x28
    char _pad04[0x74 - 0x04];            // +0x04..+0x73
    i32 m_refCount;                      // +0x74  reference count
};

// Work nodes (CObList CNode-shaped: +0x00 = next, +0x08 = child pointer).
struct WorkNode {
    WorkNode* m_next;          // +0x00
    i32 m_discard;             // +0x04  (not accessed; prev ptr or padding)
    CDDrawWorkerItem* m_child; // +0x08
};

// ---------------------------------------------------------------------------
// CDDrawWorkerList - only the load-bearing offsets are modeled: m_pSurfaceMgr at
// +0x0c (copied into the worker) and the CObList at +0x10. The four factory
// methods occupy lower vtable slots (their slot numbers are not load-bearing;
// only their bodies are matched), so they are placed last.
// ---------------------------------------------------------------------------
class CDDrawWorkerList {
public:
    i32 VirtualMethodUnknown14();
    void VirtualMethodUnknown1C();
    i32 VirtualMethodUnknown20();
    void* VirtualMethodUnknown24(i32 a1, i32 a2, i32 a3);
    void* VirtualMethodUnknown28(i32 a1, i32 a2, i32 a3, i32 addHead);
    void* VirtualMethodUnknown2C(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead);
    void* VirtualMethodUnknown30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead);
    void VirtualMethodUnknown34(i32 a1, i32 a2);

    void* m_vptr;              // +0x00 (vptr; not stamped by these methods)
    i32 m_status;              // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_pSurfaceMgr;         // +0x0c  (CDDrawSubMgr+0xc)
    CObList m_workers;         // +0x10  worker list (CObList)

    ~CDDrawWorkerList(); // 0x163bc0 (walk+destroy children, then ~CObList(m_workers))

    // Engine-label backlog stub (state predicate).
    i32 Stub_156fc0();
};

// operator delete + the sibling manager (vtable 0x5efd88) whose real member-teardown
// destructor (0x156f50) landed in this TU. Its teardown helper is CDDrawWorkerList's
// 0x163bc0 (reloc-masked cross-ref); it destructs a CObList member at +0x10 and resets
// the CObject-like header fields via the grand-base.
void operator delete(void*);

class WorkerListSibBase {
public:
    virtual void s0();
    virtual void* ScalarDtor(i32 flag);
    virtual void s2();
    virtual void s3();
    virtual void s4();
    ~WorkerListSibBase();
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    WorkerListSibBase() {}
};
inline WorkerListSibBase::~WorkerListSibBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

class CDDrawWorkerListSib : public WorkerListSibBase {
public:
    void Cleanup_163bc0();  // declared-only teardown helper (0x163bc0)
    ~CDDrawWorkerListSib(); // 0x156f50
    CObList m_10;           // +0x10
};

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several Lucius-derived managers.
RVA(0x00156f00, 0x16)
i32 CDDrawWorkerList::VirtualMethodUnknown14() {
    if (m_pSurfaceMgr == 0) {
        goto fail;
    }
    if (m_status != -1) {
        return 1;
    }

fail:
    return 0;
}

// Inline worker constructors. Each new's the raw block, and on success seeds the
// fields THROUGH the allocation register and returns it; the null path returns 0.
// Defined inline so they fold into each factory, reproducing the target's "init
// via eax, commit to esi only at the merge" register schedule. The parent's
// m_pSurfaceMgr is read INSIDE the init (after the null check), not passed as a
// pre-evaluated argument, so its load is not hoisted above the new call.
static inline CDDrawWorkerB* MakeWorkerB(const CDDrawWorkerList* parent) {
    CDDrawWorkerB* w = new CDDrawWorkerB;
    if (w != 0) {
        i32 surfaceMgr = parent->m_pSurfaceMgr;
        w->m_04 = 0;
        w->m_parent = surfaceMgr;
        w->m_08 = 0;
        w->m_20 = (i32)0x80000000;
        w->m_38 = -1;
        w->m_5c = (i32)0x80000000;
        w->m_64 = (i32)0x80000000;
        w->m_3c = 0;
        w->m_40 = 0;
        w->m_78 = 0;
    }
    return w;
}

static inline CDDrawWorkerA* MakeWorkerA(const CDDrawWorkerList* parent) {
    CDDrawWorkerA* w = new CDDrawWorkerA;
    if (w != 0) {
        i32 surfaceMgr = parent->m_pSurfaceMgr;
        w->m_04 = 0;
        w->m_parent = surfaceMgr;
        w->m_08 = 0;
        w->m_20 = (i32)0x80000000;
        w->m_38 = -1;
        w->m_5c = (i32)0x80000000;
        w->m_64 = (i32)0x80000000;
        w->m_3c = 0;
        w->m_40 = 0;
        w->m_78 = 0;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Allocates a BYTE-flag worker, constructs it, calls its +0x2c
// virtual with (a1,a2,a3). On success appends it to the list (AddTail) and returns
// it; on failure destroys it and returns 0.
RVA(0x00156fd0, 0x8b)
void* CDDrawWorkerList::VirtualMethodUnknown24(i32 a1, i32 a2, i32 a3) {
    CDDrawWorkerA* w = MakeWorkerA(this);
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    m_workers.AddTail((CObject*)w);
    return w;
}

// ---------------------------------------------------------------------------
// As Unknown24 but the int-flag worker; on success the trailing
// bool selects AddHead vs AddTail.
RVA(0x001573e0, 0xa0)
void* CDDrawWorkerList::VirtualMethodUnknown28(i32 a1, i32 a2, i32 a3, i32 addHead) {
    CDDrawWorkerB* w = MakeWorkerB(this);
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead((CObject*)w);
    } else {
        m_workers.AddTail((CObject*)w);
    }
    return w;
}

// ---------------------------------------------------------------------------
// Int-flag worker; calls the worker's +0x30 virtual with (a1,a2,a3,a4); trailing
// bool selects AddHead vs AddTail.
RVA(0x00157330, 0xa5)
void* CDDrawWorkerList::VirtualMethodUnknown2C(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead) {
    CDDrawWorkerB* w = MakeWorkerB(this);
    if (w->Vfunc30(a1, a2, a3, a4) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead((CObject*)w);
    } else {
        m_workers.AddTail((CObject*)w);
    }
    return w;
}

// ---------------------------------------------------------------------------
// As Unknown2C but dispatches the worker's +0x34 virtual.
RVA(0x00157150, 0xa5)
void* CDDrawWorkerList::VirtualMethodUnknown30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead) {
    CDDrawWorkerB* w = MakeWorkerB(this);
    if (w->Vfunc34(a1, a2, a3, a4) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead((CObject*)w);
    } else {
        m_workers.AddTail((CObject*)w);
    }
    return w;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Walks the work-node list at m_pSurfaceMgr+0x04 (= this+0x14 in the MFC
// CObList's internal head pointer, offset +0x04 from the CObList base due to
// the inherited vptr).  Destroys each node's child if present, then clears
// the list.
RVA(0x00163c60, 0x2c)
void CDDrawWorkerList::VirtualMethodUnknown1C() {
    struct HLayout {
        char _pad[0x14];
        WorkNode* m_head;
    };
    WorkNode* pNode = ((HLayout*)this)->m_head;
    while (pNode) {
        WorkNode* pCurrent = pNode;
        pNode = pNode->m_next;
        if (pCurrent->m_child) {
            pCurrent->m_child->ScalarDtor(1);
        }
    }
    m_workers.RemoveAll();
}

// ---------------------------------------------------------------------------
// Returns constant 0x11 (17).
RVA(0x00156f20, 0x6)
i32 CDDrawWorkerList::VirtualMethodUnknown20() {
    return 0x11;
}

// ---------------------------------------------------------------------------
// ~CDDrawWorkerList: walk the work-node list (head @ this+0x14), destroy every
// node's child via its scalar-deleting destructor, then the embedded CObList
// member (m_workers) auto-destructs (the trailing `lea ecx,[this+0x10]; call ~CObList`).
// Built /MT (base, no /GX) so the member teardown carries no EH frame.
RVA(0x00163bc0, 0x2c)
CDDrawWorkerList::~CDDrawWorkerList() {
    struct HLayout {
        char _pad[0x14];
        WorkNode* m_head;
    };
    WorkNode* pNode = ((HLayout*)this)->m_head;
    while (pNode) {
        WorkNode* pCurrent = pNode;
        pNode = pNode->m_next;
        if (pCurrent->m_child) {
            pCurrent->m_child->ScalarDtor(1);
        }
    }
}

// ---------------------------------------------------------------------------
// Walks the work-node list at this+0x14. For each node: calls the child's
// +0x28 virtual (passing a1, a2), decrements child->m_refCount, and conditionally
// removes the node from the CObList + destroys the child.
//
// The remove-or-skip decision:
//    (a2 != 0 && (a2->field08 & 0x20000) == 0)  → always free
//    OR child->m_refCount <= 0                          → free
//    Otherwise                                     → skip, keep node
RVA(0x00163bf0, 0x6d)
void CDDrawWorkerList::VirtualMethodUnknown34(i32 a1, i32 a2) {
    struct HLayout {
        char _pad[0x14];
        WorkNode* m_head;
    };
    WorkNode* pNode = ((HLayout*)this)->m_head;
    while (pNode) {
        WorkNode* pCurrent = pNode;
        CDDrawWorkerItem* child = pCurrent->m_child;
        pNode = pNode->m_next;
        child->Vfunc28(a1, a2);
        child->m_refCount--;
        if ((*(i32*)((char*)a2 + 0x2c) != 0 && (*(i32*)((char*)a2 + 0x08) & 0x20000) == 0)
            || child->m_refCount <= 0) {
            m_workers.RemoveAt((__POSITION*)pCurrent);
            if (child) {
                child->ScalarDtor(1);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 0x156f50: real member-teardown ~ of the sibling manager (vtable 0x5efd88): stamp the
// class vtable, run the cleanup helper (0x163bc0), destruct the CObList member @+0x10,
// reset the header fields + restamp the grand-base vtable (0x5e8cb4).
// @early-stop
// eh-unit-rule wall: retail compiled this dtor's TU /GX (it carries a push-1/handler/
// fs:0 SEH frame around the member teardown); CDDrawWorkerList.cpp is /MT (base) to
// keep its frameless factory neighbours matched, so cl emits no EH frame here. The
// vtable stamps / cleanup call / CObList destruct / field resets are reproduced; the
// SEH prologue/epilogue + reloc-masked names are the residual. Re-home in the sweep.
RVA(0x00156f50, 0x68)
CDDrawWorkerListSib::~CDDrawWorkerListSib() {
    Cleanup_163bc0();
    // implicit: ~m_10 (CObList) then ~WorkerListSibBase (field resets + base restamp).
}

// ---------------------------------------------------------------------------
// Constant state predicate returning 1.
RVA(0x00156fc0, 0x6)
i32 CDDrawWorkerList::Stub_156fc0() {
    return 1;
}

SIZE_UNKNOWN(CDDrawWorkerList);
SIZE_UNKNOWN(CDDrawWorkerListSib);
SIZE_UNKNOWN(WorkerListSibBase);
SIZE_UNKNOWN(CDDrawWorkerItem);
SIZE_UNKNOWN(CDDrawWorkerBase);
SIZE(CDDrawWorkerA, 0x7c);
SIZE(CDDrawWorkerB, 0x7c);
SIZE_UNKNOWN(WorkNode);
// ??_7CDDrawWorkerA/B (was g_ddrawWorkerAVtbl/B). A's emitted vtable carries 2
// extra slots (shared 14-slot base); only B (14 slots) matches retail size.
VTBL(CDDrawWorkerB, 0x001efed0);
