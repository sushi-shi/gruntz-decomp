#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/Object.h> // CObject - the 5-slot engine grand-base (vtbl 0x5e8cb4)

// VTBL_ABSENT: the engine grand-base is never standalone-constructed - its default
// bodies (IsLoaded @0x13b6-thunked, IsReady @0x1c08-thunked) are dispatched only
// through derived vtables; no ??_7CWapObj exists in the image (every analysed
// vtable is otherwise covered).
VTBL_ABSENT(CWapObj);
class CDDrawSurfaceMgr; // the owning world manager parked at +0x0c

class CWapObj : public CObject {
public:
    // slot 5 (@+0x14) default @0x0013b6: `return m_10 > 0`. Derived classes
    // (CLoadable::IsLoaded, CGameLevel::IsLoaded, ...) override; CImage keeps it.
    virtual i32 IsLoaded();
    // slot 6 (@+0x18) default @0x001c08: `return 1`. CImageSet1/2/3 override; the
    // rest of the family (CImage, CResolveNode, the workers, ...) keep it.
    virtual i32 IsReady();

    // +0x04/+0x08/+0x0c: the shared object header. It lives HERE, not in the two
    // direct children, on two independent proofs:
    //   1. BOTH direct children (CLoadable and CImage) used to declare the identical
    //      triple at the identical offsets with the identical roles (index/id, flag
    //      word, owning CDDrawSurfaceMgr) - one inherited header modelled twice.
    //   2. Retail's leaf ctors stamp the leaf vptr FOURTH, after these three stores and
    //      before the leaf's own +0x10 (CDDrawWorker::CreateFrame24/28/30 @0x151fb0/
    //      152060/152110, InsertFrame @0x151f00, CDDrawSubMgrLeafScan::CreateEntry/2
    //      @0x157d70/157e00). MSVC5 emits the derived vptr stamp between the BASE ctor
    //      and the DERIVED member inits, so a store that precedes the stamp can only
    //      come from a base ctor. Spelled in the leaf's own body they are derived
    //      member inits and the stamp moves to 1st (that was the "vptr-scheduler wall").
    i32 m_id;                           // +0x04  index/id; -1 = inactive (the liveness latch)
    i32 m_flags;                        // +0x08  flag word
    class CDDrawSurfaceMgr* m_ownerCtx; // +0x0c  the owning world manager

    CWapObj() {}
    // The seeding base ctor every leaf with a (index, owner) construction delegates to.
    CWapObj(i32 id, class CDDrawSurfaceMgr* owner) {
        m_id = id;
        m_flags = 0;
        m_ownerCtx = owner;
    }
    class CDDrawSurfaceMgr* OwnerMgr() const {
        return m_ownerCtx;
    }
};
SIZE(0x10);

#endif // WAP32_CWAPOBJ_H
