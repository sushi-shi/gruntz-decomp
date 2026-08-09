#ifndef GRUNTZ_CLOADABLE_H
#define GRUNTZ_CLOADABLE_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

GZ_ENUM_BEGIN(LoadableClassId)
    CLASSID_NONE = 0,
    CLASSID_SUBWORKER = 1,
    CLASSID_SURFACECHILDA = 2,
    CLASSID_SURFACEPAIR = 3,
    CLASSID_WWDOBJA = 5,
    CLASSID_WWDOBJC = 6,
    CLASSID_ANIMWORKER = 9,
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
    CLASSID_WORKERCACHE = 0x13,
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

class CLoadable : public CWapObj {
public:
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload();

    virtual LoadableClassId GetClassId();

    CLoadable() {
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
    CLoadable(class CDDrawSurfaceMgr* owner, i32 field04, i32 field08);

    enum ENoSeed {
        NO_SEED
    };
    CLoadable(class CDDrawSurfaceMgr* owner, i32 field04, i32 field08, ENoSeed) {
        m_id = field04;
        m_flags = field08;
        m_ownerCtx = owner;
    }

    class CDDrawSurfaceMgr* OwnerMgr() const {
        return m_ownerCtx;
    }

    // No destructor here: the three-field reset is CWapObj's (retail 0xd5d70),
    // reached through the inherited ??1CWapObj@@UAE@XZ.
};
SIZE(0x10);

#endif // GRUNTZ_CLOADABLE_H
