#include <rva.h>
#include <Ints.h>
#include <Gruntz/State.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>

// @early-stop
RVA(0x000fa150, 0x74)
void CState::ReleaseResources() {
    if (m_world != 0) {
        if (m_scratchSurface0 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_scratchSurface0);
            m_scratchSurface0 = 0;
        }
        if (m_scratchSurface1 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_scratchSurface1);
            m_scratchSurface1 = 0;
        }
        if (m_blitSurface0 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_blitSurface0);
            m_blitSurface0 = 0;
        }
        if (m_blitSurface1 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_blitSurface1);
            m_blitSurface1 = 0;
        }
    }
    m_ready = 0;
}
