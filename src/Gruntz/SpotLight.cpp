#include <rva.h>

#include <Gruntz/SpotLight.h>

#include <Mfc.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Rez/FrameClock.h>

#include <math.h>

// @early-stop
RVA(0x000b1ee0, 0x11d)
int CSpotLight::Update() {
    if (m_object->m_score == 1) {
        double c = cos(m_angle);
        double s = sin(m_angle);

        double newAngle = static_cast<double>(g_frameDelta) * m_angularVelocity + m_angle;
        m_position.x = -(m_offset.y * s + m_offset.x * c);
        m_position.y = m_offset.x * s - m_offset.y * c;
        if (m_focus) {
            m_center.x = static_cast<double>(m_focus->m_screenX);
            m_center.y = static_cast<double>(m_focus->m_screenY);
        }
        m_position.x = m_center.x + m_position.x;
        m_position.y = m_center.y + m_position.y;
        m_angle = newAngle;
    }
    if (g_gameReg->m_cmdGrid->m_grid[m_cellCol + m_cellRow * 15] == 0) {
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
    }
    return 0;
}
