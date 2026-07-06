#include <rva.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
// CDDrawWorkerList.cpp - four leaf factory methods of the tomalla-named class
// CDDrawWorkerList (a CDirectDrawMgr surface/page sub-manager in the ddrawmgr
// "DDraw surface manager" family; see docs/ddraw-family-names.md).
//
// All four share ONE shape: allocate a 0x7c-byte "worker" object with the global
// operator new, inline-construct it (zero/seed its
// fields + stamp its vftable), then call one of the worker's own sibling virtuals
// forwarding the caller's args. On a 0 result the worker is destroyed via its
// scalar-deleting destructor (vtable +0x4, arg 1) and the method returns 0; on a
// nonzero result the worker is appended to CDDrawWorkerList's CObList (+0x10) - either
// AddHead or AddTail, selected by a trailing bool arg - and the worker is returned.
//
// Two distinct worker vftables appear: one used by CreateWorkerA, whose +0x78
// flag is a BYTE, and one used by CreateWorkerB28/2C/30, whose +0x78 flag is an
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
// RemoveAll / RemoveAt are called by ClearWorkers and PruneWorkers.
#include <Gruntz/ObList.h>

// The worker hierarchy (CDDrawWorkerBase + CDDrawWorkerA/B) - the elements this
// list allocates/dispatches. `new CDDrawWorkerB/A` makes cl auto-emit
// ??_7CDDrawWorkerB/A and stamp the vptr in the ctor (no manual stamp); the
// slot-11/12/13 relocs name the real Vfunc overrides defined in CDDrawWorkers.cpp.
#include <DDrawMgr/DDrawWorkerNode.h>

// The child type used in the work-node linked-list at +0x14.
// Virtual slot +0x28 (Vfunc28) is dispatched in PruneWorkers; +0x74 is a
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
    i32 IsReady();
    void ClearWorkers();
    StateId GetStateId();
    void* CreateWorkerA(i32 a1, i32 a2, i32 a3);
    void* CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead);
    void* CreateWorkerB2C(i32 a1, i32 a2, CDDrawFrameSource* a3, i32 a4, i32 addHead);
    void* CreateWorkerB30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead);
    void PruneWorkers(i32 a1, i32 a2);

    virtual void
    VSlot0(); // +0x00 (vptr; not stamped by these methods)  // real polymorphic vptr @+0x00 (was m_vptr)
    i32 m_status;                   // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08];      // +0x08..0x0b
    CDDrawWorkerCtx* m_pSurfaceMgr; // +0x0c  (CDDrawSubMgr+0xc; copied into worker m_ctx)
    CObList m_workers;              // +0x10  worker list (CObList)

    ~CDDrawWorkerList(); // 0x163bc0 (walk+destroy children, then ~CObList(m_workers))

    // Engine-label backlog stub (state predicate).
    i32 IsReadyPredicate();
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
    ~CDDrawWorkerListSib(); // 0x156f50
    CObList m_10;           // +0x10
};

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several CDDrawSubMgr-derived managers.
RVA(0x00156f00, 0x16)
i32 CDDrawWorkerList::IsReady() {
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
        CDDrawWorkerCtx* surfaceMgr = parent->m_pSurfaceMgr;
        w->m_04 = 0;
        w->m_ctx = surfaceMgr;
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
        CDDrawWorkerCtx* surfaceMgr = parent->m_pSurfaceMgr;
        w->m_04 = 0;
        w->m_ctx = surfaceMgr;
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
void* CDDrawWorkerList::CreateWorkerA(i32 a1, i32 a2, i32 a3) {
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
// As CreateWorkerA but the int-flag worker; on success the trailing
// bool selects AddHead vs AddTail.
RVA(0x001573e0, 0xa0)
void* CDDrawWorkerList::CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead) {
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
void* CDDrawWorkerList::CreateWorkerB2C(
    i32 a1,
    i32 a2,
    CDDrawFrameSource* a3,
    i32 a4,
    i32 addHead
) {
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
// As CreateWorkerB2C but dispatches the worker's +0x34 virtual.
RVA(0x00157150, 0xa5)
void* CDDrawWorkerList::CreateWorkerB30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead) {
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
void CDDrawWorkerList::ClearWorkers() {
    WorkNode* pNode = (WorkNode*)m_workers.GetHeadPosition();
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
StateId CDDrawWorkerList::GetStateId() {
    return STATE_WORKERLIST; // 0x11
}

// ---------------------------------------------------------------------------
// ~CDDrawWorkerList: walk the work-node list (head @ this+0x14), destroy every
// node's child via its scalar-deleting destructor, then the embedded CObList
// member (m_workers) auto-destructs (the trailing `lea ecx,[this+0x10]; call ~CObList`).
// Built /MT (base, no /GX) so the member teardown carries no EH frame.
RVA(0x00163bc0, 0x2c)
CDDrawWorkerList::~CDDrawWorkerList() {
    WorkNode* pNode = (WorkNode*)m_workers.GetHeadPosition();
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
void CDDrawWorkerList::PruneWorkers(i32 a1, i32 a2) {
    WorkNode* pNode = (WorkNode*)m_workers.GetHeadPosition();
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
    ((CDDrawWorkerList*)this)->~CDDrawWorkerList();
    // implicit: ~m_10 (CObList) then ~WorkerListSibBase (field resets + base restamp).
}

// ---------------------------------------------------------------------------
// Constant state predicate returning 1.
RVA(0x00156fc0, 0x6)
i32 CDDrawWorkerList::IsReadyPredicate() {
    return 1;
}

SIZE_UNKNOWN(CDDrawWorkerList);
SIZE_UNKNOWN(CDDrawWorkerListSib);
SIZE_UNKNOWN(WorkerListSibBase);
SIZE_UNKNOWN(CDDrawWorkerItem);
SIZE_UNKNOWN(WorkNode);
// CDDrawWorkerBase/A/B SIZE + VTBL(CDDrawWorkerB) now live in
// <DDrawMgr/DDrawWorkerNode.h>.

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(WorkerListSibBase, 0x001efd88);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
VTBL(CDDrawWorkerItem, 0x001efea0);
VTBL(CDDrawWorkerList, 0x001efea0);
