// NetMgr.cpp - CNetMgr (DirectPlay networking manager) handlers + config writer.
// Matched methods:
//   CNetMgr::OnMultiOptions       @ RVA 0x0badd0 (67 B)
//   CNetMgr::OnMultiPause         @ RVA 0x0bad40 (108 B)
//   CNetMgr::OnOutOfSync          @ RVA 0x0bae40 (132 B)
//   CNetMgr::ApplyCmdDelayDefaults@ RVA 0x0b85a0 (210 B)
//
// The three message handlers fire a multiplayer command through the engine
// dispatcher (MultiDispatch, external, via incremental-link thunk), guarded by
// a reentrancy flag, and (Pause/OutOfSync) forward a WM_COMMAND to the engine
// window via PostMessageA when the dispatch result matches. ApplyCmdDelayDefaults
// persists the command-timing config (m_5a4/m_5a8) to the game's RegistryHelper.
#include "NetMgr.h"

CGameMgr *g_pGameMgr;                     // @0x64556c

// File-scope reentrancy guards.
static int g_optionzGuard;               // @0x648d08
static int g_pauseGuard;                 // @0x648d04
static int g_sharedFlag;                 // @0x648ce0

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiOptions  @ 0x0badd0  (__thiscall, ret).
// Reentrancy-guarded fire of the MULTI_OPTIONZ command. Clears m_584, dispatches
// (return value ignored), then clears the shared flag.
//
// @address: 0x0badd0
// @size:    0x43
// ---------------------------------------------------------------------------
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
// CNetMgr::OnMultiPause  @ 0x0bad40  (__thiscall, ret).
// Reentrancy-guarded fire of MULTI_PAUSE. When the dispatch returns 0x4cc,
// forwards WM_COMMAND(0x80d7, m_1c) to the engine window.
//
// @address: 0x0bad40
// @size:    0x6c
// ---------------------------------------------------------------------------
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
// CNetMgr::OnOutOfSync  @ 0x0bae40  (__thiscall, ret).
// Per-instance reentrancy-guarded fire of MULTI_OUTOFSYNC. Switches on the
// dispatch result: 0x4cc -> the same WM_COMMAND(0x80d7, m_1c) as Pause;
// 0x4cd -> nothing; otherwise -> WM_COMMAND(0x8023, 0).
//
// @address: 0x0bae40
// @size:    0x84
// ---------------------------------------------------------------------------
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
// CNetMgr::ApplyCmdDelayDefaults  @ 0x0b85a0  (__thiscall, ret; C++ EH frame).
// Persists the command-timing config to the game RegistryHelper (the singleton's
// +0x38 member). Builds three value-name strings "m_598 + _Suffix" via operator+
// (each a stack CString temp), writes m_5a4 under "_CmdDelay" and m_5a8 under
// "_Resend"; the "_DynCmdDelay" temp is built but its write is elided here. The
// three temporaries' dtors run under the C++ EH frame (=> /GX).
//
// @address: 0x0b85a0
// @size:    0xd2
// ---------------------------------------------------------------------------
void CNetMgr::ApplyCmdDelayDefaults()
{
    Utils::RegistryHelper *reg = g_pGameMgr->m_38;

    AfxString cmdDelayName  = m_598 + "_CmdDelay";
    AfxString resendName    = m_598 + "_Resend";
    AfxString dynCmdName    = m_598 + "_DynCmdDelay";

    reg->SetValueDword((char *)(const char *)cmdDelayName, m_5a4);
    reg->SetValueDword((char *)(const char *)resendName, m_5a8);
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: rtti-vptr
// @address: 0x0b5460
// @size:    0x914
// @stub
void CNetMgr::Stub_0b5460() {}

// @confidence: high
// @source: rtti-vptr
// @address: 0x0b6000
// @size:    0x6d
// @stub
void CNetMgr::Stub_0b6000() {}

// @confidence: med
// @source: string-xref
// @address: 0x0b78b0
// @size:    0x17f
// @stub
void CNetMgr::Stub_0b78b0() {}

// @confidence: med
// @source: decomp-xref
// @address: 0x0b7b10
// @size:    0x27c
// @stub
void CNetMgr::Stub_0b7b10() {}

// @confidence: high
// @source: decomp-xref
// @address: 0x0b82e0
// @size:    0x230
// @stub
void CNetMgr::Stub_0b82e0() {}

// @confidence: med
// @source: decomp-xref
// @address: 0x0b8b10
// @size:    0x175
// @stub
void CNetMgr::Stub_0b8b10() {}

// @confidence: low
// @source: decomp-xref
// @address: 0x0b8cf0
// @size:    0x23b
// @stub
void CNetMgr::Stub_0b8cf0() {}

// @confidence: med
// @source: decomp-xref
// @address: 0x0b9750
// @size:    0x74e
// @stub
void CNetMgr::Stub_0b9750() {}

// @confidence: high
// @source: call-xref
// @address: 0x0bc070
// @size:    0x73
// @stub
void CNetMgr::Stub_0bc070() {}

// @confidence: med
// @source: decomp-xref
// @address: 0x0bc110
// @size:    0xf6
// @stub
void CNetMgr::Stub_0bc110() {}

// @confidence: med
// @source: decomp-xref
// @address: 0x0bc460
// @size:    0x24e
// @stub
void CNetMgr::Stub_0bc460() {}

// @confidence: med
// @source: decomp-xref
// @address: 0x0bca50
// @size:    0x155
// @stub
void CNetMgr::Stub_0bca50() {}

// @confidence: low
// @source: call-xref
// @address: 0x0bcf20
// @size:    0xaf
// @stub
void CNetMgr::Stub_0bcf20() {}

// @confidence: low
// @source: call-xref
// @address: 0x0bd000
// @size:    0x19
// @stub
void CNetMgr::Stub_0bd000() {}

// @confidence: med
// @source: call-xref
// @address: 0x0bd030
// @size:    0x5d
// @stub
void CNetMgr::Stub_0bd030() {}

// @confidence: high
// @source: call-xref
// @address: 0x0bd0b0
// @size:    0x9a
// @stub
void CNetMgr::Stub_0bd0b0() {}

// @confidence: low
// @source: call-xref
// @address: 0x0bd180
// @size:    0x66
// @stub
void CNetMgr::Stub_0bd180() {}

// @confidence: high
// @source: tomalla
// @address: 0x1776a0
// @size:    0xa01
// @stub
void CNetMgr::Stub_1776a0() {}

// @confidence: high
// @source: import:DPLAYX.dll!#1
// @address: 0x1780b0
// @size:    0xbb
// @stub
void CNetMgr::Stub_1780b0() {}

// @confidence: high
// @source: import:DPLAYX.dll!#2
// @address: 0x178280
// @size:    0x43
// @stub
void CNetMgr::Stub_178280() {}

// @confidence: high
// @source: import:DPLAYX.dll!#1
// @address: 0x1782d0
// @size:    0x86
// @stub
void CNetMgr::Stub_1782d0() {}

// @confidence: med
// @source: call-xref
// @address: 0x178e20
// @size:    0x33
// @stub
void CNetMgr::Stub_178e20() {}

// @confidence: med
// @source: call-xref
// @address: 0x178e90
// @size:    0x20
// @stub
void CNetMgr::Stub_178e90() {}

// @confidence: med
// @source: call-xref
// @address: 0x178eb0
// @size:    0x3f
// @stub
void CNetMgr::Stub_178eb0() {}

// @confidence: med
// @source: call-xref
// @address: 0x178ef0
// @size:    0x5c
// @stub
void CNetMgr::Stub_178ef0() {}

// @confidence: med
// @source: call-xref
// @address: 0x178fc0
// @size:    0x44
// @stub
void CNetMgr::Stub_178fc0() {}

// @confidence: med
// @source: call-xref
// @address: 0x179090
// @size:    0x4c
// @stub
void CNetMgr::Stub_179090() {}

// @confidence: med
// @source: call-xref
// @address: 0x179130
// @size:    0x5d
// @stub
void CNetMgr::Stub_179130() {}
