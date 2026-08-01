#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/MapStringToOb.h>
#include <DDrawMgr/AniRecordBase2.h>

#include <Gruntz/ParseSource.h>

class CDDrawWorkerMapSmall : public CLoadable {
public:
    CDDrawWorkerMapSmall(CDDrawSurfaceMgr* owner) : CLoadable(owner, 0, 0) {
        m_cachedWorker = 0;
    }
    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;

    virtual i32 GetClassId() OVERRIDE;
    virtual void* Factory_1658c0(CParseSource* src, const char* key, i32 flags);

    virtual void* CreateWorker28(void* data, const char* key, i32 flags);
    virtual void* CreateWorker2C(char* path, const char* key, i32 flags);

    virtual void* Factory_165a90(CParseSource* src, i32 key, i32 flags);
    virtual ~CDDrawWorkerMapSmall() OVERRIDE;

    CMapStringToOb m_map1;
    CMapStringToOb m_map2;
    CMapStringToOb m_map3;

    CAniRecordBase2* m_cachedWorker;

    void ResetSlots();
    i32 RemoveByValue(CObject* obj);
    i32 RemoveByKey(const char* key);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
