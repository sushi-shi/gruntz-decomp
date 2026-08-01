#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
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
