#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H

#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/MapStringToOb.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawWorker;

class CDDrawWorkerCache : public CWapObj {
public:
    CDDrawWorkerCache(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}
    virtual ~CDDrawWorkerCache() OVERRIDE;

    RVA(0x001576d0, 0x16)
    virtual i32 IsLoaded() OVERRIDE {
        if (m_ownerCtx == NULL) {
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
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_WORKERCACHE;
    }

    virtual void* CreateWorker(GameObjNotifyFn factory, const char* key, i32 flags);

    // Out of line at 0x9cab0 in StreamRecordLoaders.cpp;
    // <DDrawMgr/DDrawWorkerCacheFindInline.h> is the opt-in inline view
    // (survivor: an in-class body leaves 0x9cab0 with NO emitter - cl 5.0
    // plants the declined nested calls without emitting the COMDAT; measured
    // 2026-08-15, ledger in docs/patterns/comdat-home-adjudicates-inline-spelling.md).
    CObject* Find(const char* key);

    CString FindKeyOfValue(CObject* target);

    CMapStringToOb m_workers;
};

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
