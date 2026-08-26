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
    CLASSID_FRONT_SURFACE = 2,
    CLASSID_SURFACEPAIR = 3,
    CLASSID_WWD_SPRITE_OBJECT = 5,
    CLASSID_WWD_DOT_OBJECT = 6,
    CLASSID_LOGICRECORD = 9,
    CLASSID_WWD_DEFERRED_OBJECT = 0x16,
    CLASSID_WWD_CONTAINER_OBJECT = 0x1b,
    CLASSID_PLACED_WORKER = 8,
    CLASSID_PIXEL_WORKER = 11,
    CLASSID_IMAGE = 10,
    CLASSID_WORKER = 14,

    CLASSID_SUBMGRPAGES = 0xf,
    CLASSID_CHILDGROUP = 0x10,
    CLASSID_WORKERLIST = 0x11,
    CLASSID_WORKERREGISTRY = 0x12,
    CLASSID_LOGICRECORDREGISTRY = 0x13,
    CLASSID_PALETTE_REGISTRY = 0x14,
    CLASSID_PALETTE_RESOURCE = 0x15,
    CLASSID_GAMELEVEL = 0x19,
    CLASSID_WORKERHOST = 0x1a,

    CLASSID_SERIALREF = 5,
    CLASSID_CALLBACKOBJ = 0x1c
GZ_ENUM_END(LoadableClassId)

GZ_ENUM_CONST_BEGIN(WapObjId)
    WAPOBJ_ID_NONE = -1
GZ_ENUM_CONST_END(WapObjId)

class CDDrawSurfaceMgr;

class CWapObj : public CObject {
public:
    virtual i32 IsLoaded();
    virtual i32 IsReady();
    virtual void Unload();
    virtual LoadableClassId GetClassId();

    i32 m_id;
    i32 m_flags;
    CDDrawSurfaceMgr* m_ownerCtx;

    virtual ~CWapObj() OVERRIDE {
        m_id = WAPOBJ_ID_NONE;
        m_flags = 0;
        m_ownerCtx = NULL;
    }

    CWapObj() {
        m_ownerCtx = NULL;
    }

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

RVA(0x000d5dd0, 0x6)
inline i32 CWapObj::IsReady() {
    return 1;
}

#endif // WAP32_CWAPOBJ_H
