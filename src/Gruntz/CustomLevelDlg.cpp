#include <StdAfx.h>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/WaitCursorScope.h>
#include <Ints.h>
#include <MsgParam.h>

#include <direct.h>
#include <io.h>

void CBattlezDlgCustom::DoDataExchange(CDataExchange* pDX) {
    CListBox* item = static_cast<CListBox*>(GetDlgItem(0x516));
    if (pDX->m_bSaveAndValidate == false) {
        CWaitCursorScope wait;
        char buf[0x400];
        _getcwd(buf, 0x400);
        CString glob(buf);
        glob += "\\custom\\*.wwd";
        _finddata_t fd;
        i32 h = _findfirst(glob, &fd);

        static CString s_custom("custom\\");
        if (h != -1) {
            if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                item->AddString(CString(fd.name));
            }
            while (_findnext(h, &fd) != -1) {
                if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                    item->AddString(CString(fd.name));
                }
            }
        }
        item->SetCurSel(0);
        return;
    }
    i32 sel = static_cast<i32>(item->GetCurSel());
    if (sel == LB_ERR) {
        return;
    }
    item->GetText(sel, m_customName);
    m_customName.MakeUpper();
}

BEGIN_MESSAGE_MAP(CBattlezDlgCustom, CDialog)
    ON_LBN_DBLCLK(0x516, CBattlezDlgCustom::PickIfSelected)
END_MESSAGE_MAP()

void CBattlezDlgCustom::PickIfSelected() {
    CListBox* list = static_cast<CListBox*>(GetDlgItem(0x516));
    if (list->GetCurSel() != LB_ERR) {
        CDialog::OnOK();
    }
}
