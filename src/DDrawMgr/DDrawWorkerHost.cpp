#include <rva.h>

#include <DDrawMgr/DDrawWorkerHost.h>

#include <Mfc.h>

#include <Gruntz/Loadable.h>
#include <Wwd/WwdSpatialMgr.h>

RVA(0x00163a90, 0x17)
i32 CDDrawWorkerHost::IsLoaded() {
    if (m_tileGrid != NULL && m_colOffsets != NULL) {
        return 1;
    }
    return 0;
}

RVA(0x00163ab0, 0x6)
LoadableClassId CDDrawWorkerHost::GetClassId() {
    return CLASSID_WORKERHOST;
}

RVA(0x00163ac0, 0x3)
void CDDrawWorkerHost::UnusedPlaneHook(i32) {}

// @early-stop
RVA_COMPGEN(0x00163ad0, 0x1e, ??_GCDDrawWorkerHost@@UAEPAXI@Z)
RVA(0x00163af0, 0xcd)
CDDrawWorkerHost::~CDDrawWorkerHost() {
    if (m_scroll != NULL) {
        m_scroll->PruneCount();
    }
    if (m_scroll != NULL) {
        CWwdSpatialMgr* w = m_scroll;
        delete w;
    }
    if (m_tileGrid != NULL) {
        delete[] m_tileGrid;
        m_tileGrid = NULL;
    }
    if (m_colOffsets != NULL) {
        delete[] m_colOffsets;
        m_colOffsets = NULL;
    }
}
