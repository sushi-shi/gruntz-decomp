#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H

#include <Ints.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <rva.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/MapStringToOb.h>

class CDDrawWorker;

class CDDrawWorkerCache : public CLoadable {
public:
    CDDrawWorkerCache(CDDrawSurfaceMgr* owner) : CLoadable(owner) {}
    virtual ~CDDrawWorkerCache() OVERRIDE;

    RVA(0x001576d0, 0x16)
    virtual i32 IsLoaded() OVERRIDE {
        if (m_ownerCtx == 0) {
            goto fail;
        }
        if (m_id != -1) {
            return 1;
        }

    fail:
        return 0;
    }

    RVA(0x00157790, 0x6)
    virtual i32 IsReady() OVERRIDE {
        return 1;
    }

    virtual void Unload() OVERRIDE;
    RVA(0x001576f0, 0x6)
    virtual i32 GetClassId() OVERRIDE {
        return CLASSID_WORKERCACHE;
    }

    virtual void* CreateWorker(GameObjNotifyFn factory, const char* key, i32 flags);

    CObject* Find(const char* key);

    CString FindKeyOfValue(CObject* target);

    CMapStringToOb m_workers;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
