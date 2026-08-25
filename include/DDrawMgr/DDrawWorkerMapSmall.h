#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H

#include <rva.h>

#include <DDrawMgr/AniRecordBase2.h>
#include <Gruntz/MapStringToOb.h>
#include <Ints.h>
#include <Rez/RezArchiveEntry.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawWorkerMapSmall : public CWapObj {
public:
    CDDrawWorkerMapSmall(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {
        m_cachedWorker = NULL;
    }
    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;

    virtual LoadableClassId GetClassId() OVERRIDE;
    virtual CAniRecordBase2*
    LoadPaletteFromSource(CRezArchiveEntry* src, const char* key, i32 flags);

    virtual CAniRecordBase2* CreateWorkerFromData(u8* data, const char* key, i32 flags);
    virtual CAniRecordBase2* CreateWorkerFromFile(char* path, const char* key, i32 flags);

    virtual CAniRecordBase2* LoadSizedPaletteFromSource(CRezArchiveEntry* src, i32 key, i32 flags);
    virtual ~CDDrawWorkerMapSmall() OVERRIDE;

    CMapStringToOb m_map1;
    CMapStringToOb m_map2;
    CMapStringToOb m_map3;

    CAniRecordBase2* m_cachedWorker;

    void ResetSlots();
    i32 RemoveByValue(CObject* obj);
    i32 RemoveByKey(const char* key);
};

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
