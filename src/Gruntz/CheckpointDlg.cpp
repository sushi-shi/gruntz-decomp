// CheckpointDlg.cpp - CCheckpointDlg (the checkpoint-prompts CDialog, resource
// 0xcd) plus the two module dynamic-initializers that build the pair of global
// MFC CObList singletons in the same retail .text block (0x234a0..0x2396d).
// Split out of the Dialogs.cpp aggregate (matcher-1 de-fragmentation): this is
// one contiguous retail .obj, distinct from the Battlez-setup dialog .obj that
// stays in Dialogs.cpp.
//
// Built /GX (eh): the CObList base-ctor init path carries the fs:0 frame.
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include <Gruntz/Dialogs.h>
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (g_mgrSettings; m_isCheckpointPrompts)
#include <rva.h>
#include <Globals.h> // g_msgmap_CCheckpointDlg

// The game-manager view of the 0x64556c singleton; OnToggleCheckpointPrompts
// mirrors the "disable prompts" checkbox into its m_isCheckpointPrompts flag.
extern "C" CGruntzMgr* g_mgrSettings; // 0x64556c

// The game's SendMessageA fn-ptr global (reloc-masked indirect call). Bound via
// DATA(0x006c44a4) (shared with Dialogs.cpp / BattlezDlgRow.cpp).
DATA(0x006c44a4)
extern long(WINAPI* g_pSendMessageA)(void* hWnd, unsigned msg, unsigned wp, long lp);

// The active modeless-dialog HWND cache (NetLobby::g_curDlg_64557c; DATA home in
// Net/LobbyDialogs.cpp). Referenced reloc-masked.
namespace NetLobby {
    extern HWND g_curDlg_64557c;
}

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
        NetLobby::g_curDlg_64557c = GetSafeHwnd();
        CWnd* item = GetDlgItem(0x53a);
        g_pSendMessageA(item->m_hWnd, 0xf1, 0, 0); // BM_SETCHECK
    }
}

// ---------------------------------------------------------------------------
RVA(0x00023570, 0x6)
const void* CCheckpointDlg::GetMessageMap() {
    return &g_msgmap_CCheckpointDlg;
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
    i32 checked = SendMessageA(c->m_hWnd, 0xf0, 0, 0);
    g_mgrSettings->m_isCheckpointPrompts = checked == 0;
}

// ---------------------------------------------------------------------------
// 0x0238d0 / 0x023960 - the two module dynamic-initializers that construct a pair of
// global MFC CObList objects (g_container62b5d0 @0x62b5d0, g_container62b640 @0x62b640)
// with block size 0xa via the NAFXCW CObList block-size ctor (0x1b4867 ==
// ??0CObList@@QAE@H@Z, reloc-masked). __cdecl free fns. Re-homed from
// src/Stub/BoundaryMisc.cpp.
// @orphan: both callers are unrecovered fns; module-init thunks with no owner class.
// ---------------------------------------------------------------------------
DATA(0x0022b5d0)
extern CObList g_container62b5d0;
DATA(0x0022b640)
extern CObList g_container62b640;
RVA(0x000238d0, 0xd)
void Init238d0() {
    g_container62b5d0.CObList::CObList(0xa);
}
RVA(0x00023960, 0xd)
void InitGlobalObList62b640() {
    g_container62b640.CObList::CObList(0xa);
}
