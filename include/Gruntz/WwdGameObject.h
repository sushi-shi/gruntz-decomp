#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // real MFC CObject (the object's grand-base) + CObList (m_subList @+0x1dc)
#include <Gruntz/UserLogic.h>        // CGameObject - the BASE (all data + the 17-slot vtable)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor - the real +0x1a0 anim/command cursor
#include <Gruntz/WwdGridIter.h>      // WwdRegion - the embedded +0x9c region node

class CDDrawSurfaceMgr;

struct WwdSnapshot {
    i32 m_00;          // +0x00  m_04
    i32 m_04;          // +0x04  m_188 (object id)
    i32 m_08;          // +0x08  this->GetTypeId()
    i32 m_0c;          // +0x0c  0, or this->GetSnapshotSubId() when GetTypeId()==0x1c
    i32 m_10;          // +0x10  0, or worker->m_logic->GetTypeTag()
    char m_name[0x80]; // +0x14  name string from the mgr
    i32 m_94;          // +0x94  m_posX
    i32 m_98;          // +0x98  m_posY
    i32 m_9c;          // +0x9c  m_sortKey
};
SIZE(0xa0); // WriteSnapshot emits ar->Write(&rec, 0xa0)

class CDDrawWorker; // CDDrawWorker IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>); the

class CImage;            // the cached frame element (<Image/CImage.h>; ex CGameObjLayer view)
class CDDrawSurfacePair; // slots 12-14 params (<DDrawMgr/DDrawSurfacePair.h>)
struct LeafCue;          // the leaf-scan cache value (<Gruntz/LeafCue.h>; ex LeafScanValue)

#include <DDrawMgr/AnimWorkerObj.h>

// VTBL_ABSENT: never-constructed dispatch facet, PROVEN retail-faithful - it spells
// the bare ??_G call (push 1; call [vt+4], no null guard: MSVC5 `delete` ALWAYS
// guards, so retail's caller TU itself used a slot-view declaration) and the
// shipped OOB slot-16 quirk (below).
VTBL_ABSENT(WwdRetailSlot16Facet);
struct WwdRetailSlot16Facet {
    virtual void S00();
    virtual void* Delete(i32 flag); // [1] the scalar-deleting dtor slot - ReadPlaneObjects
                                    //     calls it BARE (push 1; call [edx+4], no null guard;
                                    //     plain `delete` under MSVC5 emits the guard)
    virtual void S02();
    virtual void S03();
    virtual void S04();
    virtual void S05();
    virtual void S06();
    virtual void S07();
    virtual void S08();
    virtual void S09();
    virtual void S10();
    virtual void S11();
    virtual void S12();
    virtual void S13();
    virtual void S14();
    virtual void S15();
    virtual i32 GetSnapshotSubId(); // [16] +0x40 - lands on B[0] at runtime
};
SIZE_UNKNOWN(); // dispatch facet (never constructed)

#endif // GRUNTZ_WWDGAMEOBJECT_H
