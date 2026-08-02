#include <rva.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Mfc.h>
#include <math.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>

// @early-stop
RVA(0x000b1ee0, 0x11d)
int CSpotLight::Update() {
    if (m_object->m_score == 1) {
        double c = cos(m_90);
        double s = sin(m_90);

        double newAngle = static_cast<double>(g_frameDelta) * m_58 + m_90;
        m_60 = -(m_88 * s + m_80 * c);
        m_68 = m_80 * s - m_88 * c;
        if (m_focus) {
            m_70 = static_cast<double>(m_focus->m_screenX);
            m_78 = static_cast<double>(m_focus->m_screenY);
        }
        m_60 = m_70 + m_60;
        m_68 = m_78 + m_68;
        m_90 = newAngle;
    }
    if (g_gameReg->m_cmdGrid->m_grid[m_a0 + m_9c * 15] == 0) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");
    }
    return 0;
}
