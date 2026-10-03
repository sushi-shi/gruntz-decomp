#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/GruntzMgr.h>

template<>
CPtrList CPtrListPool<CGruntzSingleCommand>::s_freeList(0xa);

template<>
CPtrList CPtrListPool<CGruntzMultiCommand>::s_freeList(0xa);

CCheckpointDlg::CCheckpointDlg(CWnd* pParent) : CDialog(0xcd, pParent) {}

void CCheckpointDlg::DoDataExchange(CDataExchange* pDX) {
    if (pDX->m_bSaveAndValidate == false) {
        NetLobby::g_curDlg = GetSafeHwnd();
        CButton* item = static_cast<CButton*>(GetDlgItem(0x53a));
        item->SetCheck(BST_UNCHECKED);
    }
}

BEGIN_MESSAGE_MAP(CCheckpointDlg, CDialog)
    ON_BN_CLICKED(0x53a, CCheckpointDlg::OnToggleCheckpointPrompts)
END_MESSAGE_MAP()

void CCheckpointDlg::OnToggleCheckpointPrompts() {
    CButton* c = static_cast<CButton*>(GetDlgItem(0x53a));
    i32 checked = c->GetCheck();
    CGruntzMgr* reg = g_gameReg;
    reg->m_isCheckpointPrompts = checked == BST_UNCHECKED;
}
