#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/AdvancedOptions.h>

#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/StartUpPrompt.h>
#include <MsgParam.h>
#include <Io/Settings.h>

typedef enum AdvancedOptionsDlgId {
    IDC_DISABLE_VIDEO = 0x46c,
    IDC_DISABLE_AUDIO = 0x46d,
    IDC_DISABLE_SOUND = 0x46e,
    IDC_DISABLE_MUSIC = 0x46f,
    IDC_DISABLE_MOVIE = 0x470,
    IDC_DEFAULTS = 0x426,
} AdvancedOptionsDlgId;

static Settings s_options;

BOOL CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            if (!s_options.load(settingsPath())) {
                MessageBoxA(hWnd, "Could not load your settings.", "Gruntz", MB_OK | MB_ICONERROR);
                EndDialog(hWnd, 0);
                return true;
            }
            LoadOptions(hWnd, &s_options);

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
                SaveOptions(hWnd, &s_options);
                if (!s_options.save()) {
                    MessageBoxA(hWnd, "Could not save your settings.", "Gruntz", MB_OK | MB_ICONERROR);
                    return true;
                }
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

void SaveOption(HWND hWnd, Settings* reg, char* szValueName, DWORD controlId) {
    if (hWnd && szValueName && reg) {
        reg->setInt(szValueName, IsDlgButtonChecked(hWnd, controlId));
    }
}

void SetDefaults(HWND hWnd) {
    CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_SOUND, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, BST_UNCHECKED);
}

void LoadOptions(HWND hWnd, Settings* reg) {
    if (reg) {
        CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, reg->getInt("Disable Direct Video Access", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, reg->getInt("Disable Audio", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_SOUND, reg->getInt("Disable Sound", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, reg->getInt("Disable Music", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MOVIE, reg->getInt("Disable High Quality Movie", 0));
    }
}

void SaveOptions(HWND hWnd, Settings* reg) {
    if (reg) {
        SaveOption(hWnd, reg, "Disable Direct Video Access", IDC_DISABLE_VIDEO);
        SaveOption(hWnd, reg, "Disable Audio", IDC_DISABLE_AUDIO);
        SaveOption(hWnd, reg, "Disable Sound", IDC_DISABLE_SOUND);
        SaveOption(hWnd, reg, "Disable Music", IDC_DISABLE_MUSIC);
        SaveOption(hWnd, reg, "Disable High Quality Movie", IDC_DISABLE_MOVIE);
    }
}
