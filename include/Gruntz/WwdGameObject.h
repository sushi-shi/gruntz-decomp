#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // real MFC CObject (the object's grand-base) + CObList (m_subList @+0x1dc)
#include <Gruntz/UserLogic.h> // CGameObject - the BASE (all data + the 16-slot vtable @0x1f0020)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor - the real +0x1a0 anim/command cursor
#include <Gruntz/WwdGridIter.h>      // WwdRegion - the embedded +0x9c region node
#include <DDrawMgr/AnimWorkerObj.h>

class CDDrawSurfaceMgr;

// The 0xa0 level-object record. This is the WRITE side of the very same record
// CDDrawChildGroup::LoadObjects @0x15ad30 reads back (it spells it `WwdObjDesc`
// in <DDrawMgr/DDrawChildGroup.h>, field-for-field identical) - the two are one
// record under two names; folding them is a separate, still-open change.
struct WwdSnapshot {
    i32 m_00; // +0x00  m_id            (LoadObjects: -> the new object's m_id)
    i32 m_04; // +0x04  m_188           (LoadObjects: the +0x48 dedup key)
    i32 m_08; // +0x08  this->GetClassId()  (LoadObjects: the kind selector)
    // +0x0c  the SERIAL FACTORY TAG, and only for the CLASSID_CALLBACKOBJ kind:
    // LoadObjects' 0x1c arm hands this word straight to the game's registered
    // factory (InvokeCallback mode 0xa) as SerialObjectFactory's `typeId`. Every
    // other kind writes 0. See CWwdGameObjectSerial below.
    i32 m_serialTypeId;
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

// ---------------------------------------------------------------------------
// The CLASSID_CALLBACKOBJ (0x1c) game object - the one kind in the family the
// ENGINE never mints itself. CDDrawChildGroup::LoadObjects @0x15ad30 routes a
// 0x1c descriptor to the GAME's registered factory (CDDrawSurfaceMgr::
// InvokeCallback @0x156a90, mode 0xa, typeId = the descriptor's serial tag), and
// WriteSnapshot below writes that same tag back out through this class's slot-16
// virtual - the two halves of the save/load round trip.
//
// SHAPE - byte-proven at the single call site, WriteSnapshot @0x151c53:
//     mov edx,DWORD PTR [esi] / mov ecx,esi / call DWORD PTR [edx+0x40]
//     mov ebx,eax
// i.e. a NEW __thiscall virtual at slot 16 taking no arguments and returning i32.
// CGameObject's own vtable @0x1f0020 is exactly 16 words (CWwdGameObjectF's table
// starts at 0x1f0060, 0x40 bytes later), so slot 16 is this subclass's FIRST new
// slot - which is what fixes the base at CGameObject and makes the call site's
// downcast a plain checked static_cast.
//
// @identity-TODO - the class NAME is unrecoverable. The full chase, so nobody
// repeats it:
//   * no ??_7: exhaustive .rdata scan for a CObject-family table (slots 2/3/4 ==
//     0x4028ec/0x40106e/0x404034) with >= 17 slots yields only CWwdGameObjectC
//     (slot 16 = SetupFlagged @0x15c1d0, `ret 0x14` - five args) and
//     CWwdGameObjectF (slot 16 = SetupDeferred @0x15bc30, `ret 8` - two args).
//     Neither is the zero-arg getter this call site needs.
//   * no slot-8 body returns 0x1c: exhaustive `b8 1c 00 00 00` scan of .text gives
//     six hits, none a GetClassId (the family's are 0x15b760=5, 0x15c020=6,
//     0x15ba60=0x16, 0x15bce0=0x1b). Repeating the scan on the DEMO build
//     (GruntDem.exe) is also empty, so this is not an EN-v1.0 accident.
//   * no operator-new site, no RTTI COL: never constructed, so cl emitted neither
//     a vtable nor a type descriptor.
//   * the minting path is itself dead in the shipped build: SerialObjectFactory
//     @0xd2a0's mode table (index bytes @0xeb04, jump table @0xeaf0) sends mode
//     0xa to 0xeadb == `xor eax,eax; ret`, so InvokeCallback(...,0xa,...) always
//     fails and LoadObjects' 0x1c arm always bails. Consequently WriteSnapshot's
//     `cmp eax,0x1c` @0x151c51 - the ONLY such compare in the image - never fires.
// So the SHAPE is proven and the NAME is not; the class is declared here rather
// than papered over with a `.cpp`-local slot facet.
VTBL_ABSENT(CWwdGameObjectSerial); // never constructed -> retail emitted no ??_7
class CWwdGameObjectSerial : public CGameObject {
public:
    // [16] the serial factory tag LoadObjects feeds back to SerialObjectFactory
    // mode 0xa as `typeId`. Declared-only: retail emitted no body (see above).
    virtual i32 GetSerialTypeId();
};
SIZE_UNKNOWN(); // no `new`-site in either build - the size is not recoverable

#endif // GRUNTZ_WWDGAMEOBJECT_H
