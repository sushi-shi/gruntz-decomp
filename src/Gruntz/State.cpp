#include <Gruntz/State.h>
#include <rva.h>

RVA_COMPGEN(0x0008c710, 0x24, ??_GCState@@UAEPAXI@Z)

RVA(0x0008c750, 0xa9)
CState::CState() {
    m_mgr = 0;
    m_symParser = 0;
    m_world = 0;
    m_levelBank = 0;
    m_stateBank = 0;
    m_blitSurface0 = 0;
    m_blitSurface1 = 0;
    m_38 = 0;
    m_ready = 0;
    m_versionString[0] = 0;
    m_previousStateId = 0;
    m_scratchSurface0 = 0;
    m_scratchSurface1 = 0;
    m_cursorSaveSrc0.left = 0;
    m_cursorSaveSrc0.right = 0x40;
    m_cursorSaveSrc0.top = 0;
    m_cursorSaveSrc0.bottom = 0x40;
    m_cursorSaveSrc1.left = 0;
    m_cursorSaveSrc1.right = 0x40;
    m_cursorSaveSrc1.top = 0;
    m_cursorSaveSrc1.bottom = 0x40;
    m_cursorSaveDst0.left = 0;
    m_cursorSaveDst0.right = 0;
    m_cursorSaveDst0.top = 0;
    m_cursorSaveDst0.bottom = 0;
    m_cursorSaveDst1.left = 0;
    m_cursorSaveDst1.right = 0;
    m_cursorSaveDst1.top = 0;
    m_cursorSaveDst1.bottom = 0;
    m_cursorX = 0;
    m_cursorY = 0;
}
