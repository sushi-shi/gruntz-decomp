#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

#include <string>

#include <list>
class CDDrawPlacedWorker;

#include <Ints.h>

#include <DDrawMgr/DDrawPlacedWorker.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Gruntz/ObList.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

class CDDrawWorker;

class CDDrawWorkerList : public CWapObj {
public:
    CDDrawWorkerList(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {}

    virtual ~CDDrawWorkerList()  ;

    virtual i32 IsLoaded()  ;

    virtual i32 IsReady()  ;

    virtual void Unload()  ;
    virtual LoadableClassId GetClassId()  ;

    virtual CDDrawPixelWorker* CreatePixelWorker(i32 x, i32 y, i32 pixelValue);
    virtual CDDrawFrameWorker*
    CreateFrameWorker(i32 x, i32 y, const std::string& workerName, i32 frameIndex, i32 addHead);
    virtual CDDrawFrameWorker*
    CreateFrameWorker(i32 x, i32 y, CDDrawWorker* source, i32 frameIndex, i32 addHead);
    virtual CDDrawFrameWorker* CreateFrameWorker(i32 x, i32 y, CImage* frame, i32 addHead);

    virtual void RenderAndPruneWorkers(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay);

    void ClearWorkers();

    std::list<CDDrawPlacedWorker*> m_workers;
};

#endif
