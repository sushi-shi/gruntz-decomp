#include <rva.h>

#include <DDrawMgr/DDrawWorkerHost.h>

#include <Mfc.h>

#include <Wap32/WapObj.h>
#include <Wwd/WwdSpatialMgr.h>

RVA(0x00163d70, 0x17)
i32 CDDrawWorkerHost::IsLoaded() {
    if (m_tileHandles != NULL && m_tileRowOffsets != NULL) {
        return 1;
    }
    return 0;
}

RVA(0x00163d90, 0x6)
LoadableClassId CDDrawWorkerHost::GetClassId() {
    return CLASSID_WORKERHOST;
}

RVA(0x00163da0, 0x3)
void CDDrawWorkerHost::UnusedPlaneHook(i32) {}

RVA_COMPGEN(0x00163db0, 0x1e, ??_GCDDrawWorkerHost@@UAEPAXI@Z)
RVA(0x00163dd0, 0xcd)
CDDrawWorkerHost::~CDDrawWorkerHost() {
    if (m_spatialMgr != NULL) {
        m_spatialMgr->PruneCount();
    }
    if (m_spatialMgr != NULL) {
        CWwdSpatialMgr* w = m_spatialMgr;
        delete w;
    }
    if (m_tileHandles != NULL) {
        delete[] m_tileHandles;
        m_tileHandles = NULL;
    }
    if (m_tileRowOffsets != NULL) {
        delete[] m_tileRowOffsets;
        m_tileRowOffsets = NULL;
    }
}
