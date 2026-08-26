#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/VideoConfig.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00037700, 0x6a)
BOOL CALLBACK VideoOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            DialogInit(hDlg);
            return true;
        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    SaveVideoCheckboxes(hDlg);
                    EndDialog(hDlg, true);
                    return true;
                case IDCANCEL:
                    EndDialog(hDlg, false);
                    return true;
            }
            break;
    }
    return false;
}

RVA(0x00037790, 0x3c)
void DialogInit(HWND hDlg) {
    if (g_gameReg == NULL) {
        return;
    }
    CheckDlgButton(hDlg, 0x46f, g_gameReg->m_isHighDetail);
    CheckDlgButton(hDlg, 0x4d5, g_gameReg->m_isEffectsEnabled);
}

// @early-stop
RVA(0x000377e0, 0x40)
void SaveVideoCheckboxes(HWND hDlg) {
    if (g_gameReg == NULL) {
        return;
    }
    g_gameReg->m_isHighDetail = IsDlgButtonChecked(hDlg, 0x46f);
    g_gameReg->m_isEffectsEnabled = IsDlgButtonChecked(hDlg, 0x4d5);
}
