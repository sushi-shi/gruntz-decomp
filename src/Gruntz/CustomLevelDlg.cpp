#include <Gruntz/Dialogs.h>       // CBattlezDlgCustom (: CDialog), CDataExchange, CListBox (afxwin)
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>     // canonical CGruntzMgr (IsBattlezMapFile)
#include <Ints.h>
#include <rva.h>

#include <io.h>     // _finddata_t / _findfirst / _findnext (the custom-level dir walk)
#include <direct.h> // _getcwd (0x11fc10; the "game dir" resolver == current directory)
#include <Gruntz/CustomLevelDlg.h> // ex Globals.h

DATA(0x001e8e98)
void* g_battlezCustomMsgMap;

// @early-stop
// stack-buffer-placement wall (same as sibling CBattlezDlg::FillCustomLevelList
// @0x3af90): complete correct reconstruction - the GetDlgItem gate, the MFC wait
// cursor's /GX unwind, the "\custom\*.wwd" glob under the current dir, the
// function-static CString("custom\\") magic-static guard + atexit, the loop-rotated
// _findfirst/_findnext walk with the per-entry operator+ + by-value IsHidden query +
// LB_ADDSTRING, the LB finalize and EndWaitCursor, and the save/validate select
// branch all align by shape (llvm-objdump -dr). Residual is MSVC5's [esp+N] local
// placement (glob / _finddata_t / CString-temp arg slots land differently than
// retail) + the by-value CString-temp lifetime - not steerable from source.
RVA(0x000180e0, 0x23f)
void CBattlezDlgCustom::DoDataExchange(CDataExchange* pDX) {
    CListBox* item = static_cast<CListBox*>(GetDlgItem(0x516));
    if (pDX->m_bSaveAndValidate == 0) {
        static_cast<CCmdTarget*>(AfxGetModuleState()->m_pCurrentWinApp)->BeginWaitCursor();
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
                        ::SendMessageA(
                            item->m_hWnd,
                            0x180,
                            0,
                            reinterpret_cast<LPARAM>(static_cast<const char*>((s_custom + fd.name)))
                        );
                    }
                } while (_findnext(h, &fd) != -1);
            }
            ::SendMessageA(item->m_hWnd, 0x186, 0, 0);
        }
        static_cast<CCmdTarget*>(AfxGetModuleState()->m_pCurrentWinApp)->EndWaitCursor();
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
    return reinterpret_cast<const AFX_MSGMAP*>(&g_battlezCustomMsgMap);
}

// ---------------------------------------------------------------------------
// 0x183f0 (RVA-homed from src/Stub/ApiCallers.cpp) - CBattlezDlgCustom's
// list-item confirm (the message-map handler at messageMap+0x1c, via thunk
// 0x3d5f): GetDlgItem(0x516)'s window sends LB_GETCURSEL (0x188); on a valid
// selection run CDialog::OnOK. IDENTITY RESOLVED (2026-07-16, ex the
// `DlgHost_183f0` @identity-TODO shell): control 0x516 IS CBattlezDlgCustom's
// custom-level listbox (its DoDataExchange @0x180e0 fills the same id), the
// fn-table @0x5e8e98 IS that dialog's message map (GetMessageMap above returns
// it), and the whole 0x183d0..0x18430 run is that dialog's MFC boilerplate
// cluster, RVA-adjacent to its DDX.
RVA(0x000183f0, 0x2e)
void CBattlezDlgCustom::PickIfSelected() {
    HWND h = GetDlgItem(0x516)->m_hWnd;
    if (::SendMessageA(h, 0x188, 0, 0) != -1) {
        CDialog::OnOK(); // 0x1bacc3 ?OnOK@CDialog@@MAEXXZ (qualified base call, reloc-masked)
    }
}

RVA(0x00018430, 0xd)
void EndWaitCursorOnThread() {
    CCmdTarget* thread = *reinterpret_cast<CCmdTarget**>((reinterpret_cast<char*>(AfxGetModuleState()) + 4));
    thread->EndWaitCursor();
}
