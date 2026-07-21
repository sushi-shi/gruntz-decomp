#include <rva.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Mfc.h>
#include <math.h>
#include <Gruntz/ActNameRegistry.h> // g_buteTree / s_codeA (the "A" name->node bute map)
#include <Gruntz/SpotLight.h>       // canonical CSpotLight : CUserLogic (size 0xa8)
#include <Gruntz/GameRegistry.h>    // g_gameReg singleton (+0x68 CTriggerMgr* cmd grid)
#include <Gruntz/TriggerMgr.h>      // CTriggerMgr::m_grid (+0x1c 4x15 placed-cell grid)


// @early-stop
// x87 fp-stack scheduling wall (docs/patterns/x87-fp-stack-schedule.md, topic:wall):
// the mode gate, the target/anchor fold, the m_gridCol+m_gridRow*15 cell lookup and the
// "A" bute re-resolve match; the residual is the rotation fld/fmul/fsub tree's fxch
// interleave (and the frame-delta fild hoist), not source-steerable under MSVC5 /O2.
RVA(0x000b1ee0, 0x11d)
int CSpotLight::Update_0b1ee0() {
    if (m_object->m_114 == 1) {
        double c = cos(m_90);
        double s = sin(m_90);
        // hoist the m_90 advance so cl schedules the g_frameDelta term early (as retail)
        double newAngle = static_cast<double>(static_cast<u32>(g_frameDelta)) * m_58 + m_90;
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
        m_objAux->m_1c = g_buteTree.Find("A");
    }
    return 0;
}
