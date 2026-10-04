#include <StdAfx.h>

#include <rva.h>

#include <DDrawMgr/DDrawWorkerHost.h>

#include <SafeDelete.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdSpatialMgr.h>

RVA(0x00163a90, 0x17)
i32 CLevelPlane::IsLoaded() {
    if (m_tileHandles != NULL && m_tileRowOffsets != NULL) {
        return 1;
    }
    return 0;
}

RVA(0x00163ab0, 0x6)
LoadableClassId CLevelPlane::GetClassId() {
    return CLASSID_LEVEL_PLANE;
}

RVA(0x00163ac0, 0x3)
void CLevelPlane::UnusedPlaneHook(i32) {}

RVA_COMPGEN(0x00163ad0, 0x1e, ??_GCLevelPlane@@UAEPAXI@Z)
RVA(0x00163af0, 0xcd)
CLevelPlane::~CLevelPlane() {
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
