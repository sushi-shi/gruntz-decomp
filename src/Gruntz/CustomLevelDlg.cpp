// CustomLevelDlg.cpp - CBattlezDlgCustom::DoDataExchange (0x000180e0), the DDX of
// the Battlez custom-rules dialog (RTTI-proven: the fn is slot 35 == DoDataExchange
// of ??_7CBattlezDlgCustom@@6B@ @0x1e8ee4). Re-homed out of src/Stub/ApiCallers.cpp.
//
// The MFC DDX contract: pDX->m_bSaveAndValidate FALSE => transfer member->control
// (re)fill the custom-level listbox (item 0x516) by walking "<curdir>\custom\*.wwd"
// under the MFC wait cursor, prefixing each match with a function-static
// CString("custom\\"), asking the settings manager whether to show it, and
// LB_ADDSTRING-ing it; TRUE => control->member, read the current selection's text
// into m_customName (+0x5c) and MakeUpper it. /GX for the by-value CString arg +
// the local CString glob unwind.
//
// This DEFINES the body the CBattlezDlgCustom vtable slot 35 reloc pointed at (the
// former `?Populate180e0@CustomLevelDlg@m4dlg@@...` fake view was a phantom for the
// real ?DoDataExchange@CBattlezDlgCustom@@UAEXPAVCDataExchange@@@Z). Placeholder
// names elsewhere; only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>       // CBattlezDlgCustom (: CDialog), CDataExchange, CListBox (afxwin)
#include <Gruntz/GruntzMgr.h>     // canonical CGruntzMgr (IsBattlezMapFile)
#include <Gruntz/WaitCursorApp.h> // CWaitCursorApp (Begin/EndWaitCursor via AfxGetModuleState)
#include <Ints.h>
#include <rva.h>

#include <io.h>     // _finddata_t / _findfirst / _findnext (the custom-level dir walk)
#include <direct.h> // _getcwd (0x11fc10; the "game dir" resolver == current directory)

// The settings-manager singleton == *0x64556c (the real CGruntzMgr); IsBattlezMapFile
// takes the display name by value (callee destroys).
extern "C" CGruntzMgr* g_gameReg; // 0x0064556c

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
    CListBox* item = (CListBox*)GetDlgItem(0x516);
    if (pDX->m_bSaveAndValidate == 0) {
        ((CWaitCursorApp*)AfxGetModuleState()->m_pCurrentWinApp)->BeginWaitCursor();
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
                            (LPARAM)(const char*)(s_custom + fd.name)
                        );
                    }
                } while (_findnext(h, &fd) != -1);
            }
            ::SendMessageA(item->m_hWnd, 0x186, 0, 0);
        }
        ((CWaitCursorApp*)AfxGetModuleState()->m_pCurrentWinApp)->EndWaitCursor();
        return;
    }
    i32 sel = (i32)::SendMessageA(item->m_hWnd, 0x188, 0, 0);
    if (sel == -1) {
        return;
    }
    item->GetText(sel, m_customName);
    m_customName.MakeUpper();
}
