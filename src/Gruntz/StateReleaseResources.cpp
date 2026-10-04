#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/State.h>
#include <Ints.h>

#include <stddef.h>

void CState::ReleaseResources() {
    CancelSceneFade();
    if (m_world != NULL) {
        if (m_cursorSavedSurfaces[0] != NULL) {
            m_world->GetDeviceManager()->RemoveSurface(m_cursorSavedSurfaces[0]);
            m_cursorSavedSurfaces[0] = NULL;
        }
        if (m_cursorSavedSurfaces[1] != NULL) {
            m_world->GetDeviceManager()->RemoveSurface(m_cursorSavedSurfaces[1]);
            m_cursorSavedSurfaces[1] = NULL;
        }
        if (m_ownedSurface0 != NULL) {
            m_world->GetDeviceManager()->RemoveSurface(m_ownedSurface0);
            m_ownedSurface0 = NULL;
        }
        if (m_ownedSurface1 != NULL) {
            m_world->GetDeviceManager()->RemoveSurface(m_ownedSurface1);
            m_ownedSurface1 = NULL;
        }
    }
    m_ready = false;
}
