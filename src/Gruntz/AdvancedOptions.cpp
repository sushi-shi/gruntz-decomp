#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/AdvancedOptions.h>

#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/StartUpPrompt.h>
#include <MsgParam.h>
#include <Utils/RegMgr.h>

typedef enum AdvancedOptionsDlgId {
    IDC_DISABLE_VIDEO = 0x46c,
    IDC_DISABLE_AUDIO = 0x46d,
    IDC_DISABLE_SOUND = 0x46e,
    IDC_DISABLE_MUSIC = 0x46f,
    IDC_DISABLE_MOVIE = 0x470,
    IDC_DEFAULTS = 0x426,
} AdvancedOptionsDlgId;

static CRegMgr s_registryHelper;

BOOL CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            s_registryHelper.Term();
            s_registryHelper
                .Init("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL);
            LoadOptions(hWnd, &s_registryHelper);

            {
                HICON hIcon = LoadIconA(g_appResHandle, "GRUNTZ");
                if (hIcon) {
                    MsgParam icon;
                    icon.m_icon = hIcon;
                    SendMessageA(hWnd, WM_SETICON, ICON_BIG, icon.m_lparam);
                }
            }
            if (IsIconic(hWnd)) {
                ShowWindow(hWnd, SW_RESTORE);
            }
            SetForegroundWindow(hWnd);
            BringWindowToTop(hWnd);
            return true;

        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hWnd, 0);
                return true;
            }
            if (wParam == IDOK) {
                SaveOptions(hWnd, &s_registryHelper);
                EndDialog(hWnd, 1);
                return true;
            }
            if (wParam == IDC_DEFAULTS) {
                SetDefaults(hWnd);
                return true;
            }
            break;
    }

    return false;
}

void SaveOption(HWND hWnd, CRegMgr* reg, char* szValueName, DWORD controlId) {
    if (hWnd && szValueName && reg) {
        reg->Set(szValueName, IsDlgButtonChecked(hWnd, controlId));
    }
}

void SetDefaults(HWND hWnd) {
    CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_SOUND, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, BST_UNCHECKED);
}

void LoadOptions(HWND hWnd, CRegMgr* reg) {
    if (reg) {
        CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, reg->Get("Disable Direct Video Access", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, reg->Get("Disable Audio", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_SOUND, reg->Get("Disable Sound", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, reg->Get("Disable Music", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MOVIE, reg->Get("Disable High Quality Movie", 0));
    }
}

void SaveOptions(HWND hWnd, CRegMgr* reg) {
    if (reg) {
        SaveOption(hWnd, reg, "Disable Direct Video Access", IDC_DISABLE_VIDEO);
        SaveOption(hWnd, reg, "Disable Audio", IDC_DISABLE_AUDIO);
        SaveOption(hWnd, reg, "Disable Sound", IDC_DISABLE_SOUND);
        SaveOption(hWnd, reg, "Disable Music", IDC_DISABLE_MUSIC);
        SaveOption(hWnd, reg, "Disable High Quality Movie", IDC_DISABLE_MOVIE);
    }
}
