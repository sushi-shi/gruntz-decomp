#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

// DDrawWorkerList.h - CDDrawWorkerList, the CObList-backed worker factory of the
// DDraw surface-manager family (14-slot vtable ??_7CDDrawWorkerList @0x1efd88).
// It is "renderer B": the object CDDrawSurfaceMgr::Init (0x155900) news (0x2c B,
// base ctor 0x156cb0(mgr,0,0), CObList ctor at +0x10, vptr stamp 0x5efd88) and
// stores at holder+0x0c (CSpriteFactoryHolder::m_rendererB). The per-frame
// "present" the play states dispatch on it IS slot 13, PruneWorkers.
//
// SLOT-BODY PROOF (xref, 2026-07-13): slots 9-13 (the factories + PruneWorkers)
// have ZERO direct rel32 callers in retail - every reference is the vtable slot
// itself - so they are REAL VIRTUALS, not non-virtual twins (the former
// VCreateA/VCreateB28/VCreateB2C/VCreateB30/VPrune placeholder slots shadowed
// their own bodies). ClearWorkers (0x163c60) has 3 direct game callers and is NOT
// in the vtable: genuinely non-virtual. Slot 7 (0x163bc0) is byte-IDENTICAL to
// ClearWorkers (no /OPT:ICF, so this is the same source body compiled twice - the
// virtual teardown broadcast); the real destructor is 0x156f50 (own-vtbl stamp,
// call 0x163bc0, ~CObList, base field resets, CObject restamp), with its ??_G at
// 0x156f30 (vtable slot 1).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <rva.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Gruntz/ObList.h>
#include <DDrawMgr/DDrawWorkerNode.h>  // CDDrawWorkerBase/A/B (the spawned elements)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair - PruneWorkers' two render targets

struct CDDrawFrameSource; // the frame table view (def rides the workers G section)

// The worker element as the LIST sees it - the dispatch view of the
// CDDrawWorkerBase family (<DDrawMgr/DDrawWorkerNode.h>): both concrete subtypes
// (CDDrawWorkerA vtbl 0x1efea0 / CDDrawWorkerB vtbl 0x1efed0) share slots 5-9
// (same body RVAs in both retail vtables - one base definition each) and agree on
// slot 10's (CDDrawSurfacePair*, CDDrawSurfacePair*) render signature, so the list
// dispatches them through this base-shaped view. It exists because the real
// CDDrawWorkerBase deliberately declares NO virtuals (an own-slot base would emit
// a ??_7CDDrawWorkerBase that retail does not have - see DDrawWorkerNode.h); slots
// below carry the family's proven names/RVAs. CObject supplies slots [0..4].
// Declarations only - never defined/instantiated, so no ??_7 is emitted here.
class CDDrawWorkerItem : public CObject {
public:
    virtual ~CDDrawWorkerItem();  // slot 1 (each worker's own cl-emitted ??_G)
    virtual void Slot05_157200(); // [5]  0x157200 (family-shared body)
    virtual void IsValidImage();  // [6]  0x001c08 (shared engine thunk)
    virtual void Slot07_157310(); // [7]  0x157310 (family-shared body)
    virtual void Slot08_157210(); // [8]  0x157210 (family-shared body)
    virtual void Slot09_157080(); // [9]  0x157080 (family-shared body)
    // [10] the per-frame render onto the two surface pairs (A: PlotMarker_165fa0,
    // B: Slot10_1660b0) - the slot PruneWorkers dispatches per element.
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b);

    char _pad04[0x74 - 0x04]; // +0x04..+0x73 (CDDrawWorkerBase's field block)
    i32 m_refCount;           // +0x74  frames-remaining count (== CDDrawWorkerBase::m_74)
};
SIZE_UNKNOWN(CDDrawWorkerItem);

// (The former `WorkNode` raw CNode view is gone: the walks use the real MFC
// CObList POSITION/GetHeadPosition/GetNext idiom, whose afxcoll.inl inline IS the
// {pNext @+0x00, data @+0x08} node walk retail shows.)

// The intermediate base: owns the +0x04/+0x08/+0x0c header fields and the inline
// dtor that resets them (byte-proven from ~CDDrawWorkerList @0x156f50: after
// ~CObList the retail dtor stores m_status=-1, m_08=0, m_pSurfaceMgr=0, then
// restamps ??_7CObject @0x5e8cb4 - an inlined intermediate-base dtor over the
// CObject grand-base; the intermediate's own stamp is dead-store-eliminated, no
// call/ctor intervenes before the CObject stamp). Its ctor is 0x156cb0 (stores
// parent/a2/a3 at +0xc/+4/+8, stamps 0x5efc30) - i.e. this IS the real
// CDDrawSubMgr-family base (vtable 0x1efc30); the fold onto
// <DDrawMgr/DDrawSubMgr.h>'s CDDrawSubMgr is DEFERRED to the flagged DDrawSubMgr
// identity pass (that header models the same vtable with no members yet).
class WorkerListSibBase : public CObject {
public:
    virtual ~WorkerListSibBase() OVERRIDE; // slot 1 (deleting dtor -> cl-emitted ??_G)
    i32 m_status;                          // +0x04  initialized to -1 when inactive
    i32 m_08;                              // +0x08
    CDDrawWorkerCtx* m_pSurfaceMgr; // +0x0c  the owning surface mgr (copied into worker m_ctx)
    WorkerListSibBase() {}
};
inline WorkerListSibBase::~WorkerListSibBase() {
    m_status = -1;
    m_08 = 0;
    m_pSurfaceMgr = 0;
}
SIZE(WorkerListSibBase, 0x10);

// 14-slot vtable 0x5efd88: slots 0-4 from the base scheme, slots 5-13 own.
class CDDrawWorkerList : public WorkerListSibBase {
public:
    // slot 1 - real dtor body @0x156f50 (G obj, DDrawSubMgr.cpp); ??_G @0x156f30.
    virtual ~CDDrawWorkerList() OVERRIDE;
    virtual i32 IsReady();          // slot 5  0x156f00 (G obj)
    virtual i32 IsReadyPredicate(); // slot 6  0x156fc0 (G obj)
    // slot 7 - the virtual teardown broadcast (0x163bc0, T obj): byte-identical
    // twin of the non-virtual ClearWorkers (same source body, compiled twice).
    virtual void DestroyWorkers();
    virtual StateId GetStateId(); // slot 8  0x156f20 (G obj; 0x11)
    // slots 9-12 - the worker factories (G obj bodies; NO direct retail callers:
    // reached only through these slots).
    virtual void* CreateWorkerA(i32 a1, i32 a2, i32 a3);                // slot 9  0x156fd0
    virtual void* CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead); // slot 10 0x1573e0
    virtual void* CreateWorkerB2C(
        i32 a1,
        i32 a2,
        CDDrawFrameSource* a3,
        i32 a4,
        i32 addHead
    ); // slot 11 0x157330
    virtual void* CreateWorkerB30(i32 a1, i32 a2, i32 a3, i32 a4,
                                  i32 addHead); // slot 12 0x157150
    // slot 13 - the per-frame worker pump the play states "present" through
    // (0x163bf0, T obj): dispatch each worker's RenderFrame(a, b) onto the two
    // surface pairs, decrement its m_refCount, prune the expired.
    virtual void PruneWorkers(CDDrawSurfacePair* a, CDDrawSurfacePair* b);

    // Non-virtual teardown (direct-called by CMenuState::ReleaseResources /
    // CPlay::FreeListTeardown / CPlay::ModeCleanup). 0x163c60 (T obj).
    void ClearWorkers();

    // +0x10  worker list. A PLAIN CObList, byte-proven - NOT a CTypedPtrList:
    // ~CDDrawWorkerList's member teardown is a bare `call ??1CObList` (0x1b5a2b)
    // with no derived vptr stamp before it, and Init's ctor is CObList::CObList(10)
    // (0x1b59cc) - a typed-wrapper member would stamp its own ??_7 first. Hence the
    // walks' `(CDDrawWorkerItem*)GetNext(pos)` element downcast is the AUTHENTIC
    // dev idiom (MFC CObject* container), not a removable view cast.
    CObList m_workers;
};
SIZE(CDDrawWorkerList, 0x2c);       // new(0x2c) in CDDrawSurfaceMgr::Init @0x1559d1
VTBL(CDDrawWorkerList, 0x001efd88); // ??_7CDDrawWorkerList@@6B@ (14-slot vtable)

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
