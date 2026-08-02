#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/ObList.h>
#include <Ints.h>

class CDDrawWorker;

class CDDrawWorkerList : public CLoadable {
public:
    CDDrawWorkerList(CDDrawSurfaceMgr* owner) : CLoadable(owner, 0, 0) {}

    virtual ~CDDrawWorkerList() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual i32 IsReady() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual i32 GetClassId() OVERRIDE;

    virtual void* CreateWorkerA(i32 x, i32 y, i32 frame);
    virtual void* CreateWorkerB28(i32 x, i32 y, i32 frame, i32 addHead);
    virtual void* CreateWorkerB2C(i32 x, i32 y, CDDrawWorker* src, i32 frameIndex, i32 addHead);
    virtual void* CreateWorkerB30(i32 x, i32 y, const char* key, i32 frameIndex, i32 addHead);

    virtual void PruneWorkers(CDDrawSurfacePair* a, CDDrawSurfacePair* b);

    void ClearWorkers();

    CObList m_workers;
};
SIZE(0x2c);

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
