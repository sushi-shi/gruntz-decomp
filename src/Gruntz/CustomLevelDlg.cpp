#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <rva.h>

#include <io.h>
#include <direct.h>
#include <MsgParam.h>

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
RVA(0x000180e0, 0x23f)
void CBattlezDlgCustom::DoDataExchange(CDataExchange* pDX) {
    CListBox* item = static_cast<CListBox*>(GetDlgItem(0x516));
    if (pDX->m_bSaveAndValidate == 0) {
        AfxGetApp()->BeginWaitCursor();
        {
            char buf[0x400];
            _getcwd(buf, 0x400);
            CString glob(buf);
            glob += "\\custom\\*.wwd";
            _finddata_t fd;
            i32 h = _findfirst(glob, &fd);
            static CString s_custom("custom\\");
            if (h != -1) {
                do {
                    if (g_gameReg->IsBattlezMapFile(s_custom + fd.name)) {
                        MsgParam name;
                        ::SendMessageA(
                            item->m_hWnd,
                            0x180,
                            0,
                            (name.m_str = static_cast<const char*>((s_custom + fd.name)),
                             name.m_lparam)
                        );
                    }
                } while (_findnext(h, &fd) != -1);
            }
            ::SendMessageA(item->m_hWnd, 0x186, 0, 0);
        }
        AfxGetApp()->EndWaitCursor();
        return;
    }
    i32 sel = static_cast<i32>(::SendMessageA(item->m_hWnd, 0x188, 0, 0));
    if (sel == -1) {
        return;
    }
    item->GetText(sel, m_customName);
    m_customName.MakeUpper();
}

RVA(0x000183d0, 0x6)
const AFX_MSGMAP* CBattlezDlgCustom::GetMessageMap() const {
    return &messageMap;
}

RVA(0x000183f0, 0x2e)
void CBattlezDlgCustom::PickIfSelected() {
    HWND h = GetDlgItem(0x516)->m_hWnd;
    if (::SendMessageA(h, 0x188, 0, 0) != -1) {
        CDialog::OnOK();
    }
}

RVA(0x00018430, 0xd)
void EndWaitCursorOnThread() {

    AfxGetApp()->EndWaitCursor();
}
