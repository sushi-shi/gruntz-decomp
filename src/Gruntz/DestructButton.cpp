#include <StdAfx.h>

#include <Ints.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>

void CStatusBarMgr::StartDestructWarning(i32 countdownMs) {
    CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
    m_destructWarningState = DESTRUCT_WARNING_FORWARD;
    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
    m_destructWarningClock.Start(
        g_buteMgr.GetDword("StatusBar", "DestructButtonWarningDelay", 0x32)
    );
    play->SetDefeatCountdown(true, countdownMs);
    LockDestructButton(0);
}
