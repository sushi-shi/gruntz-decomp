#include <rva.h>

#include <DDrawMgr/DDrawWorkerHost.h>

#include <Mfc.h>

#include <Wwd/WwdSpatialMgr.h>

RVA(0x00163a90, 0x17)
i32 CDDrawWorkerHost::IsLoaded() {
    if (m_tileGrid != 0 && m_colOffsets != 0) {
        return 1;
    }
    return 0;
}

RVA(0x00163ab0, 0x6)
i32 CDDrawWorkerHost::GetClassId() {
    return CLASSID_WORKERHOST;
}

RVA(0x00163ac0, 0x3)
void CDDrawWorkerHost::UnusedPlaneHook(i32) {}

// @early-stop
RVA_COMPGEN(0x00163ad0, 0x1e, ??_GCDDrawWorkerHost@@UAEPAXI@Z)
RVA(0x00163af0, 0xcd)
CDDrawWorkerHost::~CDDrawWorkerHost() {
    if (m_scroll != 0) {
        m_scroll->PruneCount();
    }
    if (m_scroll != 0) {

        CWwdSpatialMgr* w = m_scroll;
        if (w != 0) {
            w->FreeGrids();
            ::operator delete(w);
        }
    }
    if (m_tileGrid != 0) {
        ::operator delete(m_tileGrid);
        m_tileGrid = 0;
    }
    if (m_colOffsets != 0) {
        ::operator delete(m_colOffsets);
        m_colOffsets = 0;
    }
}

VTBL(CDDrawWorkerHost, 0x001f0270);
