#include <rva.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/State.h>
#include <Ints.h>

#include <stddef.h>

// @early-stop
RVA(0x000fa150, 0x74)
void CState::ReleaseResources() {
    if (m_world != NULL) {
        if (m_scratchSurface0 != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_scratchSurface0);
            m_scratchSurface0 = NULL;
        }
        if (m_scratchSurface1 != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_scratchSurface1);
            m_scratchSurface1 = NULL;
        }
        if (m_blitSurface0 != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_blitSurface0);
            m_blitSurface0 = NULL;
        }
        if (m_blitSurface1 != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_blitSurface1);
            m_blitSurface1 = NULL;
        }
    }
    m_ready = 0;
}
