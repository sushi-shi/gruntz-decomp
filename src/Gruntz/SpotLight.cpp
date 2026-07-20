// SpotLight.cpp - the CSpotLight per-tick eyecandy update (0x0b1ee0). CSpotLight is
// modeled canonically in <Gruntz/SpotLight.h> (a CUserLogic leaf, vftable 0x5e75bc,
// size 0xa8 - the operator new(0xa8) at its new-site 0xaf210); this TU contributes
// only the Update_0b1ee0 method. When the owner object's mode (m_object->m_114) is 1
// it rotates the (m_80,m_88) offset by the running angle m_90 (advanced by the per-tick
// rate m_58 scaled by the frame delta g_frameDelta), folds in the tracked target's
// screen pos (m_focus->m_screenX/m_screenY), and advances m_90; then it (re)resolves the
// light's "A" bute node into m_objAux->m_1c when the trigger manager's per-cell grid
// slot (m_cmdGrid->m_grid[col + row*15]) is empty. Engine globals + CButeTree::Find are
// external (reloc-masked). flags=base (/O2 /Oi -> fsin/fcos).
//
// (The former flat CSpotLight carcass here was a DOUBLE DEFINITION whose fields were all
// +0x30 shifted - it inherited CUserLogic (0x30) AND added a pad00[0x10], so sizeof was
// 0xd8 and every field offset was wrong, capping this fn at 56%. Dissolved onto the one
// canonical CSpotLight (SpotLight.h). The SpotM10/SpotM14/SpotM98 views were CGameObject/
// AnimWorkerObj/CGameObject facets and MgrObj68 was CTriggerMgr::m_grid - all dissolved.)
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Mfc.h>
#include <math.h>
#include <Gruntz/ActNameRegistry.h> // g_buteTree / s_codeA (the "A" name->node bute map)
#include <Gruntz/SpotLight.h>       // canonical CSpotLight : CUserLogic (size 0xa8)
#include <Gruntz/GameRegistry.h>    // g_gameReg singleton (+0x68 CTriggerMgr* cmd grid)
#include <Gruntz/TriggerMgr.h>      // CTriggerMgr::m_grid (+0x1c 4x15 placed-cell grid)

extern "C" unsigned g_frameDelta;    // 0x645584 frame delta (canonical ?n@@3HA; reloc-masked)

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
        m_60 = -(m_88 * s + m_80 * c);
        m_68 = m_80 * s - m_88 * c;
        if (m_focus) {
            m_70 = static_cast<double>(m_focus->m_screenX);
            m_78 = static_cast<double>(m_focus->m_screenY);
        }
        m_60 = m_70 + m_60;
        m_68 = m_78 + m_68;
        m_90 = static_cast<double>(g_frameDelta) * m_58 + m_90;
    }
    if (g_gameReg->m_cmdGrid->m_grid[m_a0 + m_9c * 15] == 0) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("A");
    }
    return 0;
}
