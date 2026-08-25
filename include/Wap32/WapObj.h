#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <stddef.h>

GZ_ENUM_BEGIN(LoadableClassId)
    CLASSID_NONE = 0,
    CLASSID_SUBWORKER = 1,
    CLASSID_SURFACECHILDA = 2,
    CLASSID_SURFACEPAIR = 3,
    CLASSID_WWDOBJA = 5,
    CLASSID_WWDOBJC = 6,
    CLASSID_LOGICRECORD = 9,
    CLASSID_WWDOBJF = 0x16,
    CLASSID_WWDOBJB = 0x1b,
    CLASSID_WORKERNODE = 8,
    CLASSID_WORKERPIXEL = 11,
    CLASSID_IMAGE = 10,
    CLASSID_WORKER = 14,

    CLASSID_SUBMGRPAGES = 0xf,
    CLASSID_CHILDGROUP = 0x10,
    CLASSID_WORKERLIST = 0x11,
    CLASSID_WORKERREGISTRY = 0x12,
    CLASSID_LOGICRECORDREGISTRY = 0x13,
    CLASSID_WORKERMAPSMALL = 0x14,
    CLASSID_ANIRECORDBASE2 = 0x15,
    CLASSID_GAMELEVEL = 0x19,
    CLASSID_WORKERHOST = 0x1a,

    CLASSID_SERIALREF = 5,

    CLASSID_WWDOBJ_C = 6,
    CLASSID_WWDOBJ_F = 0x16,
    CLASSID_WWDOBJ_B = 0x1b,

    CLASSID_CALLBACKOBJ = 0x1c
GZ_ENUM_END(LoadableClassId)

GZ_ENUM_CONST_BEGIN(WapObjId)
    WAPOBJ_ID_NONE = -1
GZ_ENUM_CONST_END(WapObjId)

class CDDrawSurfaceMgr;

// ONE class, not two.  `??_7CLoadable@@6B@` was our name for the vtable at
// 0x1efc30, whose nine slots are CObject's five plus IsLoaded / IsReady /
// Unload / GetClassId - i.e. exactly the union of what we had split across an
// abstract "CWapObj" and a concrete "CLoadable".  Three independent readings
// agree there is no class between them:
//   * `??_GCLoadable@@UAEPAXI@Z` (0x155720) calls 0xd5d70 DIRECTLY as its own
//     destructor - no intermediate teardown exists to call.
//   * 0xd5d70 is also what retail's `??1CImage` unwind funclet calls, and
//     CImage's RTTI base array is CImage -> CWapObj -> CObject.
//   * the 31 `??1CLoadable -> ??1CWapObj` rows in `eh_band --census`: retail
//     destroys every derived base sub-object as CWapObj.
// RTTI names it CWapObj (`.?AVCWapObj@@` is present, `.?AVCLoadable@@` is not),
// so CWapObj is the surviving name and 0x1efc30 is `??_7CWapObj@@6B@`.
class CWapObj : public CObject {
public:
    // Slots 5..8 of 0x1efc30, in vtable order.
    virtual i32 IsLoaded();
    virtual i32 IsReady();
    virtual void Unload();
    virtual LoadableClassId GetClassId();

    i32 m_id;
    i32 m_flags;
    CDDrawSurfaceMgr* m_ownerCtx;

    // 0xd5d70 (RVA_COMPGEN pin at the emitting keeper, Play.cpp - an RVA() here
    // would annotate BOTH cl dtor variants and collide with ??_GCWapObj@0x155720).
    // Retail's ??1CImage EH funclet CALLS it and ??_GCLoadable (=
    // ??_GCWapObj) tail-calls it, so the three-field reset is this class's own.
    virtual ~CWapObj() OVERRIDE {
        m_id = WAPOBJ_ID_NONE;
        m_flags = 0;
        m_ownerCtx = NULL;
    }

    CWapObj() {
        m_ownerCtx = NULL;
    }

    // Two entities for the two shapes retail shows (docs/patterns/
    // two-shapes-need-two-entities.md).  `sema xref 0x156cb0` lists exactly four
    // retail `call` sites - CDDrawSurfaceMgr::Init, CDDrawSubMgrPages::
    // CreateChildren, CDDrawChildGroup::CreateContainerObject and
    // CDDrawWorkerHost::ReadPlaneObjects - while every other derived ctor
    // expands the three stores.  The pinned body (0x156cb0, DDrawSubMgr.cpp)
    // serves the callers; the tagged sibling serves the expansions.  The tag
    // must stay on the SIBLING: on the pinned body it would turn `ret 0xc`
    // into `ret 0x10`.
    CWapObj(CDDrawSurfaceMgr* owner, i32 id, i32 flags);

    enum ENoSeed {
        NO_SEED
    };
    CWapObj(CDDrawSurfaceMgr* owner, i32 id, i32 flags, ENoSeed) {
        m_id = id;
        m_flags = flags;
        m_ownerCtx = owner;
    }

    CWapObj(i32 id, CDDrawSurfaceMgr* owner) {
        m_id = id;
        m_flags = 0;
        m_ownerCtx = owner;
    }

    CDDrawSurfaceMgr* OwnerMgr() const {
        return m_ownerCtx;
    }
};

RVA(0x000d5da0, 0x6)
inline i32 CWapObj::IsReady() {
    return 1;
}

#endif // WAP32_CWAPOBJ_H
