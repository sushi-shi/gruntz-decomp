#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>         // canonical CPlay (one shape; ArmSnapshot is reached here)
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr big host
#include <rva.h>

#include <Ints.h>

RVA(0x0010bc30, 0x78)
void CStatusBarMgr::UpdateDestructButton(i32 arg) {
    CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
    m_destructWarnActive = 1;
    m_modeState = 2;
    m_destructWarnDelay =
        static_cast<u32>(g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32));
    m_destructWarnLast = static_cast<u32>(g_frameTime);
    play->ArmSnapshot(1, arg);
    SetMode(0);
}
