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
        if (m_cursorSavedSurfaces[0] != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_cursorSavedSurfaces[0]);
            m_cursorSavedSurfaces[0] = NULL;
        }
        if (m_cursorSavedSurfaces[1] != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_cursorSavedSurfaces[1]);
            m_cursorSavedSurfaces[1] = NULL;
        }
        if (m_ownedSurface0 != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_ownedSurface0);
            m_ownedSurface0 = NULL;
        }
        if (m_ownedSurface1 != NULL) {
            m_world->m_deviceManager->RemoveSurface(m_ownedSurface1);
            m_ownedSurface1 = NULL;
        }
    }
    m_ready = false;
}
