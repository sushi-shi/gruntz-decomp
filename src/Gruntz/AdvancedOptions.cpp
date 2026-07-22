#include <Mfc.h>
#include <Utils/RegistryHelper.h>
#include <rva.h>

typedef enum AdvancedOptionsDlgId {
    IDC_DISABLE_VIDEO = 0x46c,
    IDC_DISABLE_AUDIO = 0x46d,
    IDC_DISABLE_SOUND = 0x46e,
    IDC_DISABLE_MUSIC = 0x46f,
    IDC_DISABLE_MOVIE = 0x470,
    IDC_DEFAULTS = 0x426,
} AdvancedOptionsDlgId;

// File-scope globals. The reloc that names them is masked in objdiff; only the
// address-load bytes are load-bearing. Bound to their retail DATA rvas by exact
// base-obj symbol name (MSVC internal-linkage $S mangling; clang mangles differently
// so DATA_SYMBOL pins the exact name the reloc uses).
static Utils::RegistryHelper g_registryHelper;
static HINSTANCE g_hInstance;

RVA(0x0000b110, 0x32)
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

RVA(0x0000b160, 0x37)
void SetDefaults(HWND hWnd) {
    CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_SOUND, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, 0);
}

RVA(0x0000b1b0, 0x90)
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

RVA(0x0000b270, 0x75)
void SaveOptions(HWND hWnd, Utils::RegistryHelper* pRegistryHelper) {
    if (pRegistryHelper) {
        SaveOption(hWnd, pRegistryHelper, "Disable Direct Video Access", IDC_DISABLE_VIDEO);
        SaveOption(hWnd, pRegistryHelper, "Disable Audio", IDC_DISABLE_AUDIO);
        SaveOption(hWnd, pRegistryHelper, "Disable Sound", IDC_DISABLE_SOUND);
        SaveOption(hWnd, pRegistryHelper, "Disable Music", IDC_DISABLE_MUSIC);
        SaveOption(hWnd, pRegistryHelper, "Disable High Quality Movie", IDC_DISABLE_MOVIE);
    }
}

RVA(0x0000af50, 0xb)
void ResetRegistryHelper() {
    g_registryHelper.m_open = 0;
}

RVA(0x0000afb0, 0x108)
INT_PTR CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            g_registryHelper.Close();
            g_registryHelper
                .Open("Monolith Productions", "Gruntz", "1.0", 0, HKEY_LOCAL_MACHINE, 0);
            LoadOptions(hWnd, &g_registryHelper);

            {
                HICON hIcon = LoadIconA(g_hInstance, "GRUNTZ");
                if (hIcon) {
                    SendMessageA(hWnd, WM_SETICON, 1, reinterpret_cast<LPARAM>(hIcon));
                }
            }
            if (IsIconic(hWnd)) {
                ShowWindow(hWnd, SW_RESTORE);
            }
            SetForegroundWindow(hWnd);
            BringWindowToTop(hWnd);
            return 1;

        case WM_COMMAND:
            if (wParam == 2) { // IDCANCEL
                EndDialog(hWnd, 0);
                return 1;
            }
            if (wParam == 1) { // IDOK
                SaveOptions(hWnd, &g_registryHelper);
                EndDialog(hWnd, 1);
                return 1;
            }
            if (wParam == IDC_DEFAULTS) {
                SetDefaults(hWnd);
                return 1;
            }
            break;
    }

    return 0;
}
