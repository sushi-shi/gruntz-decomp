#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/VideoConfig.h>

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

void DialogInit(HWND hDlg) {
    if (g_gameReg == NULL) {
        return;
    }
    CheckDlgButton(hDlg, 0x46f, g_gameReg->m_isHighDetail);
    CheckDlgButton(hDlg, 0x4d5, g_gameReg->m_isEffectsEnabled);
}

void SaveVideoCheckboxes(HWND hDlg) {
    if (g_gameReg == NULL) {
        return;
    }
    g_gameReg->m_isHighDetail = IsDlgButtonChecked(hDlg, 0x46f);
    g_gameReg->m_isEffectsEnabled = IsDlgButtonChecked(hDlg, 0x4d5);
}
