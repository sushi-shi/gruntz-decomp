#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

// DDrawWorkerList.h - CDDrawWorkerList, the CObList-backed worker factory of the
// DDraw surface-manager family (14-slot vtable ??_7CDDrawWorkerList @0x1efd88).
// Hoisted from DDrawWorkerList.cpp (wave4-L): the IsReady/GetStateId/factory
// quartet lives in the G obj (DDrawSubMgr.cpp), the dtor/Clear/Prune teardown in
// the T obj (DDrawSurfacePair.cpp).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <rva.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Gruntz/ObList.h>
#include <DDrawMgr/DDrawWorkerNode.h> // CDDrawWorkerBase/A/B (the spawned elements)

struct CDDrawFrameSource; // the frame table view (def rides the workers G section)

// The child type used in the work-node linked-list at +0x14. Virtual slot +0x28
// (Vfunc28) is dispatched in PruneWorkers; +0x74 is a reference count.
class CDDrawWorkerItem {
public:
    virtual void Slot00();
    virtual ~CDDrawWorkerItem(); // slot 1 (deleting dtor -> cl-emitted ??_G)
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
SIZE_UNKNOWN(CDDrawWorkerItem);

// Work nodes (CObList CNode-shaped: +0x00 = next, +0x08 = child pointer).
struct WorkNode {
    WorkNode* m_next;          // +0x00
    i32 m_discard;             // +0x04  (not accessed; prev ptr or padding)
    CDDrawWorkerItem* m_child; // +0x08
};
SIZE_UNKNOWN(WorkNode);

// 14-slot vtable 0x5efd88: slots 0-4 are the shared CObject base thunks, slot 1
// the scalar-deleting dtor, slots 5-13 the list's own virtuals.
class CDDrawWorkerList : public CObject {
public:
    virtual ~CDDrawWorkerList() OVERRIDE; // slot 1 (dtor 0x163bc0, T obj / ??_G 0x156f30)
    virtual i32 IsReady();                // slot 5  0x156f00 (G obj)
    virtual i32 IsReadyPredicate();       // slot 6  0x156fc0 (G obj)
    virtual void VDtor7();                // slot 7  @0x163bc0
    virtual StateId GetStateId();         // slot 8  0x156f20 (G obj; 0x11)
    virtual void VCreateA();              // slot 9  @0x156fd0
    virtual void VCreateB28();            // slot 10 @0x1573e0
    virtual void VCreateB2C();            // slot 11 @0x157330
    virtual void VCreateB30();            // slot 12 @0x157150
    virtual void VPrune();                // slot 13 @0x163bf0

    // Non-virtual helpers (direct-called). Factories in the G obj; teardown in T.
    void ClearWorkers(); // 0x163c60
    void* CreateWorkerA(i32 a1, i32 a2, i32 a3);
    void* CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead);
    void* CreateWorkerB2C(i32 a1, i32 a2, CDDrawFrameSource* a3, i32 a4, i32 addHead);
    void* CreateWorkerB30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead);
    void PruneWorkers(i32 a1, i32 a2); // 0x163bf0

    i32 m_status;                   // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08];      // +0x08..0x0b
    CDDrawWorkerCtx* m_pSurfaceMgr; // +0x0c  (CDDrawSubMgr+0xc; copied into worker m_ctx)
    CObList m_workers;              // +0x10  worker list (CObList)
};
SIZE_UNKNOWN(CDDrawWorkerList);
VTBL(CDDrawWorkerList, 0x001efd88); // ??_7CDDrawWorkerList@@6B@ (14-slot vtable)

// The sibling manager (vtable 0x5efd88) whose real member-teardown destructor
// (0x156f50, G obj) calls CDDrawWorkerList's 0x163bc0 teardown helper.
class WorkerListSibBase {
public:
    virtual void s0();
    virtual ~WorkerListSibBase(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void s2();
    virtual void s3();
    virtual void s4();
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
SIZE_UNKNOWN(WorkerListSibBase);

class CDDrawWorkerListSib : public WorkerListSibBase {
public:
    ~CDDrawWorkerListSib(); // 0x156f50 (G obj)
    CObList m_10;           // +0x10
};
SIZE_UNKNOWN(CDDrawWorkerListSib);

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
