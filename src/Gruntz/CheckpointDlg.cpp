#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (g_gameReg; m_isCheckpointPrompts)
#include <Gruntz/GruntzCommand.h> // canonical g_singleCmdList/g_multiCmdList (@0x62b5d0/0x62b640)
#include <rva.h>
#include <Gruntz/CheckpointDlg.h> // ex Globals.h

DATA(0x001e94b8)
const i32 g_msgmap_CCheckpointDlg = 6205544;

VTBL(CCheckpointDlg, 0x001e9504); // vtable_names -> code (RTTI game class)
RVA(0x000234a0, 0x1e)
CCheckpointDlg::CCheckpointDlg(CWnd* pParent) : CDialog(0xcd, pParent) {}

RVA(0x00023520, 0x3e)
void CCheckpointDlg::DoDataExchange(CDataExchange* pDX) {
    if (pDX->m_bSaveAndValidate == 0) {
        NetLobby::g_curDlg = GetSafeHwnd();
        CWnd* item = GetDlgItem(0x53a);
        ::SendMessageA(item->m_hWnd, 0xf1, 0, 0); // BM_SETCHECK
    }
}

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

RVA(0x000238d0, 0xd)
void Init238d0() {
    (static_cast<CPtrList*>(&g_singleCmdList))->CPtrList::CPtrList(0xa);
}
RVA(0x00023960, 0xd)
void InitGlobalObList() {
    (static_cast<CPtrList*>(&g_multiCmdList))->CPtrList::CPtrList(0xa);
}
