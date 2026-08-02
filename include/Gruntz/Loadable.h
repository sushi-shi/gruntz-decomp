#ifndef GRUNTZ_CLOADABLE_H
#define GRUNTZ_CLOADABLE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

enum LoadableClassId {
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
    CLASSID_IMAGE = 10,
    CLASSID_WORKER = 14,

    CLASSID_SUBMGRPAGES = 0xf,
    CLASSID_CHILDGROUP = 0x10,
    CLASSID_WORKERLIST = 0x11,
    CLASSID_WORKERREGISTRY = 0x12,
    CLASSID_WORKERCACHE = 0x13,
    CLASSID_WORKERMAPSMALL = 0x14,
    CLASSID_GAMELEVEL = 0x19,
    CLASSID_WORKERHOST = 0x1a,

    CLASSID_SERIALREF = 5,

    CLASSID_WWDOBJ_C = 6,
    CLASSID_WWDOBJ_F = 0x16,
    CLASSID_WWDOBJ_B = 0x1b,

    CLASSID_CALLBACKOBJ = 0x1c,
};

class CLoadable : public CWapObj {
public:
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload();

    virtual i32 GetClassId();

    CLoadable() {}

    CLoadable(class CDDrawSurfaceMgr* owner, i32 field04, i32 field08);

    CLoadable(class CDDrawSurfaceMgr* owner) {
        m_id = 0;
        m_flags = 0;
        m_ownerCtx = owner;
    }

    CLoadable(i32 id, class CDDrawSurfaceMgr* owner) {
        m_id = id;
        m_flags = 0;
        m_ownerCtx = owner;
    }

    CLoadable(class CDDrawSurfaceMgr* owner, i32 id) {
        m_id = id;
        m_flags = 0;
        m_ownerCtx = owner;
    }

    CLoadable(i32 id, i32 flags, class CDDrawSurfaceMgr* owner) {
        m_id = id;
        m_flags = flags;
        m_ownerCtx = owner;
    }

    class CDDrawSurfaceMgr* OwnerMgr() const {
        return m_ownerCtx;
    }

    virtual ~CLoadable() OVERRIDE {
        m_id = -1;
        m_flags = 0;
        m_ownerCtx = 0;
    }
};
SIZE(0x10);

#endif // GRUNTZ_CLOADABLE_H
