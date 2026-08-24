#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <Gruntz/ObList.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

class CDDrawWorker;

class CDDrawWorkerList : public CWapObj {
public:
    CDDrawWorkerList(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {}

    virtual ~CDDrawWorkerList() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual i32 IsReady() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual CDDrawWorkerA* CreatePixelWorker(i32 x, i32 y, i32 pixelValue);
    virtual CDDrawWorkerB*
    CreateFrameWorker(i32 x, i32 y, const char* workerName, i32 frameIndex, i32 addHead);
    virtual CDDrawWorkerB*
    CreateFrameWorker(i32 x, i32 y, CDDrawWorker* source, i32 frameIndex, i32 addHead);
    virtual CDDrawWorkerB* CreateFrameWorker(i32 x, i32 y, CImage* frame, i32 addHead);

    virtual void RenderAndPruneWorkers(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay);

    void ClearWorkers();

    CObList m_workers;
};

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
