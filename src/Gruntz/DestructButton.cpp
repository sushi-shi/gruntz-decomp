#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>

RVA(0x0010bc30, 0x78)
void CStatusBarMgr::StartDestructWarning(i32 countdownMs) {
    CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
    m_destructWarningState = DESTRUCT_WARNING_FORWARD;
    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
    m_destructWarningClock.m_interval =
        static_cast<u32>(g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32));
    m_destructWarningClock.m_last = static_cast<u32>(g_frameTime);
    play->SetDefeatCountdown(true, countdownMs);
    LockDestructButton(0);
}
