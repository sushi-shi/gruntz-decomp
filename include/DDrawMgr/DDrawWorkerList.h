#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h> // CLoadable - the real base (ctor 0x156cb0, vtbl 0x1efc30)
#include <Gruntz/ObList.h>
#include <DDrawMgr/DDrawWorkerNode.h>  // CDDrawWorkerBase/A/B (the spawned elements)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair - PruneWorkers' two render targets

class
    CDDrawWorker; // the frame-source (ex CDDrawFrameSource view) // the frame table view (def rides the workers G section)

// (The "WorkerListSibBase" intermediate shell is DISSOLVED - it WAS CLoadable:
// CDDrawSurfaceMgr::Init constructs the list with the CLoadable 3-arg base ctor
// 0x156cb0 (stores parent/a2/a3 at +0xc/+4/+8, stamps ??_7CLoadable 0x5efc30)
// before the derived 0x5efd88 stamp, and ~CDDrawWorkerList @0x156f50 ends with
// the same m_04=-1/m_flags=0/m_0c=0 reset trio ~CLoadable inlines. The slot
// quartet [5..8] below is the CLoadable scheme.)
class CDDrawWorkerList : public CLoadable {
public:
    // slot 1 - real dtor body @0x156f50 (G obj, DDrawSubMgr.cpp); ??_G @0x156f30.
    virtual ~CDDrawWorkerList() OVERRIDE;
    // [5] 0x156f00 (G obj) - loaded iff the +0x0c owner is bound and the +0x04
    // status latch isn't -1 (ex "IsReady"; the scheme's slot-5 predicate).
    virtual i32 IsLoaded() OVERRIDE; // [5] 0x156f00
    // [6] 0x156fc0 (G obj) - the scheme's IsReady (ex "IsReadyPredicate").
    virtual i32 IsReady() OVERRIDE; // [6] 0x156fc0
    // [7] 0x163bc0 (T obj) - the virtual teardown broadcast (ex "DestroyWorkers"):
    // byte-identical twin of the non-virtual ClearWorkers (same source body,
    // compiled twice); overrides CLoadable's reset/unload hook.
    virtual void Unload() OVERRIDE; // [7] 0x163bc0
    virtual i32 GetClassId() OVERRIDE; // [8] 0x156f20 -> CLASSID_WORKERLIST (0x11)
    // slots 9-12 - the worker factories (G obj bodies; NO direct retail callers:
    // reached only through these slots).
    virtual void* CreateWorkerA(i32 a1, i32 a2, i32 a3);                // slot 9  0x156fd0
    virtual void* CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead); // slot 10 0x1573e0
    virtual void* CreateWorkerB2C(
        i32 a1,
        i32 a2,
        CDDrawWorker* a3,
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
SIZE(0x2c); // new(0x2c) in CDDrawSurfaceMgr::Init @0x1559d1

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
