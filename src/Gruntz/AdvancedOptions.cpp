#include <rva.h>

#include <Gruntz/AdvancedOptions.h>

#include <Mfc.h>

#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/StartUpPrompt.h>
#include <MsgParam.h>
#include <Utils/RegistryHelper.h>

typedef enum AdvancedOptionsDlgId {
    IDC_DISABLE_VIDEO = 0x46c,
    IDC_DISABLE_AUDIO = 0x46d,
    IDC_DISABLE_SOUND = 0x46e,
    IDC_DISABLE_MUSIC = 0x46f,
    IDC_DISABLE_MOVIE = 0x470,
    IDC_DEFAULTS = 0x426,
} AdvancedOptionsDlgId;

RVA_DYNINIT(0x0000af40, 0xa, g_registryHelper)
RVA_DYNINIT(0x0000af60, 0xb, g_registryHelper)
RVA_DYNINIT(0x0000af80, 0xe, g_registryHelper)
RVA_DYNINIT(0x0000afa0, 0xa, g_registryHelper)
DATA(0x0022a530)
static Utils::RegistryHelper g_registryHelper;

RVA(0x0000afc0, 0x108)
BOOL CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            g_registryHelper.Close();
            g_registryHelper
                .Open("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL);
            LoadOptions(hWnd, &g_registryHelper);

            {
                HICON hIcon = LoadIconA(g_appResHandle, "GRUNTZ");
                if (hIcon) {
                    MsgParam icon;
                    icon.m_icon = hIcon;
                    SendMessageA(hWnd, WM_SETICON, 1, icon.m_lparam);
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
            if (wParam == 1) {
                SaveOptions(hWnd, &g_registryHelper);
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

RVA(0x0000b120, 0x32)
void SaveOption(
    HWND hWnd,
    Utils::RegistryHelper* pRegistryHelper,
    char* szValueName,
    DWORD controlId
) {
    if (hWnd && szValueName && pRegistryHelper) {
        pRegistryHelper->SetValueDword(szValueName, IsDlgButtonChecked(hWnd, controlId));
    }
}

RVA(0x0000b170, 0x37)
void SetDefaults(HWND hWnd) {
    CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_SOUND, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, 0);
}

RVA(0x0000b1c0, 0x90)
void LoadOptions(HWND hWnd, Utils::RegistryHelper* pRegistryHelper) {
    if (pRegistryHelper) {
        CheckDlgButton(
            hWnd,
            IDC_DISABLE_VIDEO,
            pRegistryHelper->GetValueDword("Disable Direct Video Access", 0)
        );
        CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, pRegistryHelper->GetValueDword("Disable Audio", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_SOUND, pRegistryHelper->GetValueDword("Disable Sound", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, pRegistryHelper->GetValueDword("Disable Music", 0));
        CheckDlgButton(
            hWnd,
            IDC_DISABLE_MOVIE,
            pRegistryHelper->GetValueDword("Disable High Quality Movie", 0)
        );
    }
}

RVA(0x0000b280, 0x75)
void SaveOptions(HWND hWnd, Utils::RegistryHelper* pRegistryHelper) {
    if (pRegistryHelper) {
        SaveOption(hWnd, pRegistryHelper, "Disable Direct Video Access", IDC_DISABLE_VIDEO);
        SaveOption(hWnd, pRegistryHelper, "Disable Audio", IDC_DISABLE_AUDIO);
        SaveOption(hWnd, pRegistryHelper, "Disable Sound", IDC_DISABLE_SOUND);
        SaveOption(hWnd, pRegistryHelper, "Disable Music", IDC_DISABLE_MUSIC);
        SaveOption(hWnd, pRegistryHelper, "Disable High Quality Movie", IDC_DISABLE_MOVIE);
    }
}
