// CheckpointDlg.cpp - CCheckpointDlg (the checkpoint-prompts CDialog, resource
// 0xcd) plus the two module dynamic-initializers that build the pair of global
// MFC CPtrList singletons in the same retail .text block (0x234a0..0x2396d).
// Split out of the Dialogs.cpp aggregate (matcher-1 de-fragmentation): this is
// one contiguous retail .obj, distinct from the Battlez-setup dialog .obj that
// stays in Dialogs.cpp.
//
// Built /GX (eh): the CPtrList base-ctor init path carries the fs:0 frame.
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (g_gameReg; m_isCheckpointPrompts)
#include <Gruntz/GruntzCommand.h> // canonical g_singleCmdList/g_multiCmdList (@0x62b5d0/0x62b640)
#include <rva.h>
#include <Globals.h> // g_msgmap_CCheckpointDlg

// The game-manager view of the 0x64556c singleton; OnToggleCheckpointPrompts
// mirrors the "disable prompts" checkbox into its m_isCheckpointPrompts flag.

// The game's SendMessageA fn-ptr global (reloc-masked indirect call). Reference
// the canonical extern "C" binding _g_pSendMessageA (DATA home @0x2c44a4 in
// GruntzMgr.cpp); the old local DATA(0x006c44a4) was the VA, not the RVA - it
// mis-bound the C++-mangled ?::SendMessageA to 0x6c44a4 (retail wants 0x2c44a4).

// The active modeless-dialog HWND cache (NetLobby::g_curDlg; DATA home in
// Net/LobbyDialogs.cpp) comes from <Gruntz/Dialogs.h> (included above).

// ---------------------------------------------------------------------------
RVA(0x000234a0, 0x1e)
CCheckpointDlg::CCheckpointDlg(CWnd* pParent) : CDialog(0xcd, pParent) {}

// CCheckpointDlg::DoDataExchange (0x23520): the MFC DDX. Only the LOAD pass acts
// (m_bSaveAndValidate == 0): cache the dialog's HWND (GetSafeHwnd, null-this safe)
// into NetLobby::g_curDlg, then clear the 0x53a "disable prompts" checkbox
// (BM_SETCHECK 0). Save is a no-op.
RVA(0x00023520, 0x3e)
void CCheckpointDlg::DoDataExchange(CDataExchange* pDX) {
    if (pDX->m_bSaveAndValidate == 0) {
        NetLobby::g_curDlg = GetSafeHwnd();
        CWnd* item = GetDlgItem(0x53a);
        ::SendMessageA(item->m_hWnd, 0xf1, 0, 0); // BM_SETCHECK
    }
}

// ---------------------------------------------------------------------------
RVA(0x00023570, 0x6)
const AFX_MSGMAP* CCheckpointDlg::GetMessageMap() const {
    return reinterpret_cast<const AFX_MSGMAP*>(&g_msgmap_CCheckpointDlg); // msgmap global still a placeholder type
}

// CCheckpointDlg::OnToggleCheckpointPrompts (0x23590): mirror the "disable prompts"
// checkbox (control 0x53a) into the game registry - checked -> prompts off.
// @early-stop
// zero-register-pinning regalloc coin-flip (~97.9%): instructions byte-identical,
// only the g_mgr base vs bool result land in ecx/edx swapped (sete cl+[edx] vs
// sete dl+[ecx]); not source-steerable, permuter no-change. See zero-register-pinning.md.
RVA(0x00023590, 0x31)
void CCheckpointDlg::OnToggleCheckpointPrompts() {
    CWnd* c = GetDlgItem(0x53a);
    i32 checked = ::SendMessageA(c->m_hWnd, 0xf0, 0, 0);
    g_gameReg->m_isCheckpointPrompts = checked == 0;
}

// ---------------------------------------------------------------------------
// 0x0238d0 / 0x023960 - the two module dynamic-initializers that construct the pair of
// command recycle-list globals (g_singleCmdList @0x62b5d0, g_multiCmdList @0x62b640; the
// canonical CGruntzCmdList == an MFC CPtrList) with block size 0xa via the NAFXCW CPtrList
// block-size ctor (0x1b4867 == ??0CPtrList@@QAE@H@Z, reloc-masked). __cdecl free fns.
// Re-homed from src/Stub/BoundaryMisc.cpp. The DIR32 for each global's address binds to
// the tree-winning ?g_singleCmdList/?g_multiCmdList@CGruntzCmdList symbol (DATA home
// GruntzCmdMgr.cpp) - the old TU-local CPtrList g_container62b5d0/g_container62b640 views
// were dedup-losers to those canonicals and reloc-masked UNBOUND.
// @orphan: both callers are unrecovered fns; module-init thunks with no owner class.
// ---------------------------------------------------------------------------
RVA(0x000238d0, 0xd)
void Init238d0() {
    (static_cast<CPtrList*>(&g_singleCmdList))->CPtrList::CPtrList(0xa);
}
RVA(0x00023960, 0xd)
void InitGlobalObList62b640() {
    (static_cast<CPtrList*>(&g_multiCmdList))->CPtrList::CPtrList(0xa);
}
