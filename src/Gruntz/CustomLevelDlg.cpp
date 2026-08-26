#include <rva.h>

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

RVA_DYNINIT(0x00017d70, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x00017d90, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x00017dc0, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x00017de0, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x00017e10, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00017e30, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00017e60, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x00017e80, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x00017eb0, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x00017ed0, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x00017f00, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00017f20, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00017f50, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x00017f70, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x00017fa0, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x00017fc0, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x00017ff0, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x00018010, 0x1a, s_gruntDirCenter)

DATA(0x001e8e98)
const AFX_MSGMAP CBattlezDlgCustom::messageMap = {
    &CDialog::messageMap,
    &CBattlezDlgCustom::_messageEntries[0],
};

DATA(0x001e8ea0)
const AFX_MSGMAP_ENTRY CBattlezDlgCustom::_messageEntries[] = {
    ON_LBN_DBLCLK(0x516, CBattlezDlgCustom::PickIfSelected){0, 0, 0, 0, AfxSig_end, 0},
};

// @early-stop
RVA(0x000180f0, 0x23f)
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
        RVA_DYNINIT(0x000183c0, 0xa, s_custom)
        DATA(0x0022ad9c)
        static CString s_custom("custom\\");
        if (h != -1) {
            if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                MsgParam name;
                item->SendMessageA(
                    LB_ADDSTRING,
                    0,
                    (name.m_str = static_cast<const char*>(CString(fd.name)), name.m_lparam)
                );
            }
            while (_findnext(h, &fd) != -1) {
                if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                    MsgParam name;
                    item->SendMessageA(
                        LB_ADDSTRING,
                        0,
                        (name.m_str = static_cast<const char*>(CString(fd.name)), name.m_lparam)
                    );
                }
            }
        }
        item->SendMessageA(LB_SETCURSEL, 0, 0);
        return;
    }
    i32 sel = static_cast<i32>(item->SendMessageA(LB_GETCURSEL, 0, 0));
    if (sel == -1) {
        return;
    }
    item->GetText(sel, m_customName);
    m_customName.MakeUpper();
}

RVA(0x000183e0, 0x6)
const AFX_MSGMAP* CBattlezDlgCustom::GetMessageMap() const {
    return &messageMap;
}

RVA(0x00018400, 0x2e)
void CBattlezDlgCustom::PickIfSelected() {
    CWnd* list = GetDlgItem(0x516);
    if (list->SendMessageA(LB_GETCURSEL, 0, 0) != -1) {
        CDialog::OnOK();
    }
}
