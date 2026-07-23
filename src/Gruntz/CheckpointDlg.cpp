#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (g_gameReg; m_isCheckpointPrompts)
#include <Gruntz/GruntzCommand.h>
#include <rva.h>
#include <Gruntz/CheckpointDlg.h> // ex Globals.h

template<> DATA(0x0022b5d0)
CPtrList CPtrListPool<CGruntzSingleCommand>::s_freeList(0xa);

template<> DATA(0x0022b640)
CPtrList CPtrListPool<CGruntzMultiCommand>::s_freeList(0xa);

DATA(0x001e94b8)
const i32 g_msgmap_CCheckpointDlg = 6205544;

VTBL(CCheckpointDlg, 0x001e9504); // vtable_names -> code (RTTI game class)
RVA(0x000234a0, 0x1e)
CCheckpointDlg::CCheckpointDlg(CWnd* pParent) : CDialog(0xcd, pParent) {}

RVA_COMPGEN(0x000234d0, 0x1e, ??_GCCheckpointDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x00023500, 0x5, ??1CCheckpointDlg@@UAE@XZ)

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
    return reinterpret_cast<const AFX_MSGMAP*>(
        &g_msgmap_CCheckpointDlg
    ); // msgmap global still a placeholder type
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

// MSVC's compiler-private initializer families for the two explicit template
// static specializations above. The guarded 0x1f-byte destructor helpers share
// one compiler guard byte and use bits 1 and 2 respectively.
