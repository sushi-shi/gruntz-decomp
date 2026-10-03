#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawWorkerHost.h>

#include <SafeDelete.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdSpatialMgr.h>

i32 CDDrawWorkerHost::IsLoaded() {
    if (m_tileHandles != NULL && m_tileRowOffsets != NULL) {
        return 1;
    }
    return 0;
}

LoadableClassId CDDrawWorkerHost::GetClassId() {
    return CLASSID_WORKERHOST;
}

void CDDrawWorkerHost::UnusedPlaneHook(i32) {}

CDDrawWorkerHost::~CDDrawWorkerHost() {
    if (m_spatialMgr != NULL) {
        m_spatialMgr->PruneCount();
    }
    if (m_spatialMgr != NULL) {
        CWwdSpatialMgr* w = m_spatialMgr;
        delete w;
    }
    SAFE_DELETE_ARRAY(m_tileHandles);
    SAFE_DELETE_ARRAY(m_tileRowOffsets);
}
