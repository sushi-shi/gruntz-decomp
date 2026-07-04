// DestructButton.cpp - CSBI_RectOnly::UpdateDestructButton (RVA 0x10bc30).
//
// Arms the "destruct button" status-bar warning: latch the snapshot timer on the
// active CPlay (from g_mgrSettings), seed the warning-delay window from the
// StatusBar/DestructButtonWarningDelay bute setting, then drop the item to mode 0.
// Field names are placeholders; only offsets + code bytes are load-bearing.
//
// The big-host CSBI_RectOnly class is the ONE canonical def in <Gruntz/SBI_RectOnly.h>
// (this method's fields m_558..m_56c and SetMode all live there); the former per-TU
// minimal `class CSBI_RectOnly {}` view was folded onto it (P1 view fold).
#include <Bute/ButeMgr.h>        // canonical CButeMgr (one shape)
#include <Gruntz/Play.h>         // canonical CPlay (one shape; ArmSnapshot is reached here)
#include <Gruntz/SBI_RectOnly.h> // canonical CSBI_RectOnly big host
#include <rva.h>

#include <Ints.h>

// The game-manager singleton is the canonical CGameRegistry (*0x64556c, via
// CPlay.h); its +0x2c current game-state downcasts to the active CPlay.
extern "C" CGameRegistry* g_mgrSettings; // 0x64556c

// The CButeMgr text-config singleton (?g_buteMgr@@3VCButeMgr@@A @ 0x6453d8).
// GetDwordDef (0x1721e0) is on the canonical CButeMgr (include/Bute/ButeMgr.h).
DATA(0x006453d8)
extern CButeMgr g_buteMgr;

// g_645588 (the free-running clock global) comes from <Gruntz/Play.h>.

RVA(0x0010bc30, 0x78)
void CSBI_RectOnly::UpdateDestructButton(i32 arg) {
    CPlay* play = (CPlay*)g_mgrSettings->m_curState;
    m_destructWarnActive = 1;
    m_modeState = 2;
    m_destructWarnDelay = g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
    m_destructWarnDelayHi = 0;
    m_destructWarnLast = g_645588;
    m_destructWarnLastHi = 0;
    play->ArmSnapshot(1, arg);
    SetMode(0);
}
