// DestructButton.cpp - CStatusBarMgr::UpdateDestructButton (RVA 0x10bc30).
//
// Arms the "destruct button" status-bar warning: latch the snapshot timer on the
// active CPlay (from g_gameReg), seed the warning-delay window from the
// StatusBar/DestructButtonWarningDelay bute setting, then drop the item to mode 0.
// Field names are placeholders; only offsets + code bytes are load-bearing.
//
// The big-host CStatusBarMgr class is the ONE canonical def in <Gruntz/StatusBarMgr.h>
// (this method's fields m_558..m_56c and SetMode all live there); the former per-TU
// minimal `class CStatusBarMgr {}` view was folded onto it (P1 view fold).
#include <Bute/ButeMgr.h>        // canonical CButeMgr (one shape)
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/Play.h>         // canonical CPlay (one shape; ArmSnapshot is reached here)
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr big host
#include <rva.h>

#include <Ints.h>

// The game-manager singleton is the canonical CGameRegistry (*0x64556c, via
// CPlay.h); its +0x2c current game-state downcasts to the active CPlay.

// The CButeMgr text-config singleton (?g_buteMgr@@3VCButeMgr@@A @ VA 0x6453d8 -> RVA
// 0x2453d8). GetDwordDef (0x1721e0) is on the canonical CButeMgr (include/Bute/ButeMgr.h).
// g_buteMgr (canonical CButeMgr) comes from <Bute/ButeMgr.h>.

// g_frameTime (the free-running clock global) comes from <Gruntz/Play.h>.

RVA(0x0010bc30, 0x78)
void CStatusBarMgr::UpdateDestructButton(i32 arg) {
    CPlay* play = (CPlay*)g_gameReg->m_curState;
    m_destructWarnActive = 1;
    m_modeState = 2;
    m_destructWarnDelay = g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
    m_destructWarnDelayHi = 0;
    m_destructWarnLast = g_frameTime;
    m_destructWarnLastHi = 0;
    play->ArmSnapshot(1, arg);
    SetMode(0);
}
