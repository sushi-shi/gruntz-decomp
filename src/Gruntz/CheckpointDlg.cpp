#include <rva.h>

#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/GruntzMgr.h>

RVA_DYNINIT(0x00023840, 0xa, s_freeList)
RVA_DYNINIT(0x00023860, 0xd, s_freeList)
RVA_DYNINIT(0x00023880, 0xe, s_freeList)
RVA_DYNINIT(0x000238a0, 0x1f, s_freeList)
template<> DATA(0x0022c528)
CPtrList CPtrListPool<CGruntzSingleCommand>::s_freeList(0xa);

RVA_DYNINIT(0x000238d0, 0xa, s_freeList)
RVA_DYNINIT(0x000238f0, 0xd, s_freeList)
RVA_DYNINIT(0x00023910, 0xe, s_freeList)
RVA_DYNINIT(0x00023930, 0x1f, s_freeList)
template<> DATA(0x0022c598)
CPtrList CPtrListPool<CGruntzMultiCommand>::s_freeList(0xa);

DATA(0x001e94b8)
const AFX_MSGMAP CCheckpointDlg::messageMap = {
    &CDialog::messageMap,
    &CCheckpointDlg::_messageEntries[0],
};

DATA(0x001e94c0)
const AFX_MSGMAP_ENTRY CCheckpointDlg::_messageEntries[] = {
    ON_BN_CLICKED(0x53a, CCheckpointDlg::OnToggleCheckpointPrompts){0, 0, 0, 0, AfxSig_end, 0},
};

RVA(0x00023430, 0x1e)
CCheckpointDlg::CCheckpointDlg(CWnd* pParent) : CDialog(0xcd, pParent) {}

RVA_COMPGEN(0x00023460, 0x1e, ??_GCCheckpointDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x00023490, 0x5, ??1CCheckpointDlg@@UAE@XZ)

RVA(0x000234b0, 0x3e)
void CCheckpointDlg::DoDataExchange(CDataExchange* pDX) {
    if (pDX->m_bSaveAndValidate == false) {
        NetLobby::g_curDlg = GetSafeHwnd();
        CWnd* item = GetDlgItem(0x53a);
        item->SendMessageA(BM_SETCHECK, 0, 0);
    }
}

RVA(0x00023500, 0x6)
const AFX_MSGMAP* CCheckpointDlg::GetMessageMap() const {
    return &messageMap;
}

RVA(0x00023520, 0x31)
void CCheckpointDlg::OnToggleCheckpointPrompts() {
    CWnd* c = GetDlgItem(0x53a);
    i32 checked = c->SendMessageA(BM_GETCHECK, 0, 0);
    CGruntzMgr* reg = g_gameReg;
    reg->m_isCheckpointPrompts = checked == 0;
}
