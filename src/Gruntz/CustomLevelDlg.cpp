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
        std::string glob(buf);
        glob += "\\custom\\*.wwd";
        _finddata_t fd;
        i32 h = _findfirst((glob).c_str(), &fd);

        static std::string s_custom("custom\\");
        if (h != -1) {
            if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                item->AddString((std::string(fd.name)).c_str());
            }
            while (_findnext(h, &fd) != -1) {
                if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                    item->AddString((std::string(fd.name)).c_str());
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
    m_customName = readListBoxText(item->GetSafeHwnd(), sel, false);
    std::transform((m_customName).begin(), (m_customName).end(), (m_customName).begin(), asciiUpper);
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
