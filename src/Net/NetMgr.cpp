// NetMgr.cpp - CNetMgr (DirectPlay networking manager) handlers + config writer.
// Matched methods:
//   CNetMgr::OnMultiOptions
//   CNetMgr::OnMultiPause
//   CNetMgr::OnOutOfSync
//   CNetMgr::ApplyCmdDelayDefaults
//
// The three message handlers fire a multiplayer command through the engine
// dispatcher (MultiDispatch, external, via incremental-link thunk), guarded by
// a reentrancy flag, and (Pause/OutOfSync) forward a WM_COMMAND to the engine
// window via PostMessageA when the dispatch result matches. ApplyCmdDelayDefaults
// persists the command-timing config (m_5a4/m_5a8) to the game's RegistryHelper.
#include <Net/NetMgr.h>
#include <rva.h>

CGameMgr *g_pGameMgr;

// File-scope reentrancy guards.
static int g_optionzGuard;
static int g_pauseGuard;
static int g_sharedFlag;

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiOptions
// Reentrancy-guarded fire of the MULTI_OPTIONZ command. Clears m_584, dispatches
// (return value ignored), then clears the shared flag.
RVA(0x0badd0, 0x43)
void CNetMgr::OnMultiOptions()
{
    if (g_optionzGuard)
        return;

    m_584 = 0;
    g_optionzGuard = 1;
    MultiDispatch("MULTI_OPTIONZ", MultiOptionzCallback, 0);
    g_optionzGuard = 0;
    g_sharedFlag = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiPause
// Reentrancy-guarded fire of MULTI_PAUSE. When the dispatch returns 0x4cc,
// forwards WM_COMMAND(0x80d7, m_1c) to the engine window.
RVA(0x0bad40, 0x6c)
void CNetMgr::OnMultiPause()
{
    if (g_pauseGuard)
        return;

    m_584 = 0;
    g_pauseGuard = 1;
    int r = MultiDispatch("MULTI_PAUSE", MultiPauseCallback, 0);
    g_pauseGuard = 0;
    g_sharedFlag = 0;

    if (r == 0x4cc) {
        void *hwnd = ((CNetHwndHolder *)((CNetHwndHolder *)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x80d7, m_1c);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::OnOutOfSync
// Per-instance reentrancy-guarded fire of MULTI_OUTOFSYNC. Switches on the
// dispatch result: 0x4cc -> the same WM_COMMAND(0x80d7, m_1c) as Pause;
// 0x4cd -> nothing; otherwise -> WM_COMMAND(0x8023, 0).
RVA(0x0bae40, 0x84)
void CNetMgr::OnOutOfSync()
{
    if (m_574)
        return;

    m_574 = 1;
    m_584 = 0;
    int r = MultiDispatch("MULTI_OUTOFSYNC", MultiOutOfSyncCallback, 0);
    g_sharedFlag = 0;

    switch (r) {
    case 0x4cc: {
        void *hwnd = ((CNetHwndHolder *)((CNetHwndHolder *)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x80d7, m_1c);
        break;
    }
    case 0x4cd:
        break;
    default: {
        void *hwnd = ((CNetHwndHolder *)((CNetHwndHolder *)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x8023, 0);
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::ApplyCmdDelayDefaults  (C++ EH frame).
// Persists the command-timing config to the game RegistryHelper (the singleton's
// +0x38 member). Builds three value-name strings "m_598 + _Suffix" via operator+
// (each a stack CString temp), writes m_5a4 under "_CmdDelay" and m_5a8 under
// "_Resend"; the "_DynCmdDelay" temp is built but its write is elided here. The
// three temporaries' dtors run under the C++ EH frame (=> /GX).
RVA(0x0b85a0, 0xd2)
void CNetMgr::ApplyCmdDelayDefaults()
{
    Utils::RegistryHelper *reg = g_pGameMgr->m_38;

    CString cmdDelayName  = m_598 + "_CmdDelay";
    CString resendName    = m_598 + "_Resend";
    CString dynCmdName    = m_598 + "_DynCmdDelay";

    reg->SetValueDword((char *)(const char *)cmdDelayName, m_5a4);
    reg->SetValueDword((char *)(const char *)resendName, m_5a8);
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x0b5460, 0x914)
void CNetMgr::Stub_0b5460() {}

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x0b6000, 0x6d)
void CNetMgr::Stub_0b6000() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0b78b0, 0x17f)
void CNetMgr::Stub_0b78b0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0b7b10, 0x27c)
void CNetMgr::Stub_0b7b10() {}

// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x0b82e0, 0x230)
void CNetMgr::Stub_0b82e0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0b8b10, 0x175)
void CNetMgr::Stub_0b8b10() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x0b8cf0, 0x23b)
void CNetMgr::Stub_0b8cf0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0b9750, 0x74e)
void CNetMgr::Stub_0b9750() {}

// @confidence: high
// @source: call-xref
// @stub
RVA(0x0bc070, 0x73)
void CNetMgr::Stub_0bc070() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0bc110, 0xf6)
void CNetMgr::Stub_0bc110() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0bc460, 0x24e)
void CNetMgr::Stub_0bc460() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0bca50, 0x155)
void CNetMgr::Stub_0bca50() {}

// @confidence: low
// @source: call-xref
// @stub
RVA(0x0bcf20, 0xaf)
void CNetMgr::Stub_0bcf20() {}

// @confidence: low
// @source: call-xref
// @stub
RVA(0x0bd000, 0x19)
void CNetMgr::Stub_0bd000() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x0bd030, 0x5d)
void CNetMgr::Stub_0bd030() {}

// @confidence: high
// @source: call-xref
// @stub
RVA(0x0bd0b0, 0x9a)
void CNetMgr::Stub_0bd0b0() {}

// @confidence: low
// @source: call-xref
// @stub
RVA(0x0bd180, 0x66)
void CNetMgr::Stub_0bd180() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1776a0, 0xa01)
void CNetMgr::Stub_1776a0() {}

// @confidence: high
// @source: import:DPLAYX.dll!#1
// @stub
RVA(0x1780b0, 0xbb)
void CNetMgr::Stub_1780b0() {}

// @confidence: high
// @source: import:DPLAYX.dll!#2
// @stub
RVA(0x178280, 0x43)
void CNetMgr::Stub_178280() {}

// @confidence: high
// @source: import:DPLAYX.dll!#1
// @stub
RVA(0x1782d0, 0x86)
void CNetMgr::Stub_1782d0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x178e20, 0x33)
void CNetMgr::Stub_178e20() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x178e90, 0x20)
void CNetMgr::Stub_178e90() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x178eb0, 0x3f)
void CNetMgr::Stub_178eb0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x178ef0, 0x5c)
void CNetMgr::Stub_178ef0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x178fc0, 0x44)
void CNetMgr::Stub_178fc0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x179090, 0x4c)
void CNetMgr::Stub_179090() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x179130, 0x5d)
void CNetMgr::Stub_179130() {}
