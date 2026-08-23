#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/VideoConfig.h>

// The tail of retail's video-options dialog code is a SECOND translation unit:
// the region carries its own GruntDirStatics $E initializer at 0x375d2 (between
// ScrollDialog and VideoOptionsDlgProc) constructing a second direction-cell
// copy at 0x22be40, and one TU can only ever emit one. Split out of
// VideoConfig.cpp 2026-08-09; the retail filename is unrecovered.

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000377e0, 0x6a)
BOOL CALLBACK VideoOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            DialogInit(hDlg);
            return TRUE;
        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    SaveVideoCheckboxes(hDlg);
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

RVA(0x00037870, 0x3c)
void DialogInit(HWND hDlg) {
    if (g_gameReg == NULL) {
        return;
    }
    CheckDlgButton(hDlg, 0x46f, g_gameReg->m_isHighDetail);
    CheckDlgButton(hDlg, 0x4d5, g_gameReg->m_isEffectsEnabled);
}

// @early-stop
// Register-rotation cursor phase on the second g_gameReg re-read; the streams are
// otherwise identical (docs/patterns/register-colour-is-cursor-phase-not-a-work-item.md).
RVA(0x000378c0, 0x40)
void SaveVideoCheckboxes(HWND hDlg) {
    if (g_gameReg == NULL) {
        return;
    }
    g_gameReg->m_isHighDetail = IsDlgButtonChecked(hDlg, 0x46f);
    g_gameReg->m_isEffectsEnabled = IsDlgButtonChecked(hDlg, 0x4d5);
}
