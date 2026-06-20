// AdvancedOptions.cpp - Gruntz "Advanced Options" modal dialog (C:\Proj\Gruntz).
// The dialog lets the user toggle five "Disable ..." flags, persisted under the
// HKLM\Software\Monolith Productions\Gruntz\1.0 registry key via the engine's
// Utils::RegistryHelper wrapper (src/Utils/RegistryHelper.{cpp,h}, byte-matched).
//
// Matched (all byte-exact unless noted):
//   AdvancedOptionsDialogProc - INT_PTR CALLBACK dialog proc
//   SaveOption                - one checkbox -> RegDWORD
//   SetDefaults               - clear all five checkboxes
//   LoadOptions               - 5x GetValueDword -> checkbox
//   SaveOptions               - 5x SaveOption
//
// Only offsets / control IDs / code bytes are load-bearing; names are placeholders.
#include <Utils/RegistryHelper.h>
#include <rva.h>

// The USER32 dialog/window API, HWND/HINSTANCE/HICON/UINT, INT_PTR, and the
// WM_*/BST_*/SW_*/HKEY_* literals all come from the real <windows.h>, pulled in
// (the MFC-safe way) by RegistryHelper.h -> Mfc.h.

// Control IDs of the five "Disable ..." checkboxes and the "Defaults" button.
#define IDC_DISABLE_VIDEO   0x46c
#define IDC_DISABLE_AUDIO   0x46d
#define IDC_DISABLE_SOUND   0x46e
#define IDC_DISABLE_MUSIC   0x46f
#define IDC_DISABLE_MOVIE   0x470
#define IDC_DEFAULTS        0x426

// File-scope globals. The reloc that
// names them is masked in objdiff; only the address-load bytes are load-bearing.
static Utils::RegistryHelper g_registryHelper;
static HINSTANCE             g_hInstance;

// ---------------------------------------------------------------------------
// SaveOption
// Reads checkbox `controlId`'s state and writes it as a REG_DWORD value.
// __cdecl free function (ends in `ret`). pRegistryHelper->SetValueDword is
// __thiscall (this in ECX).
RVA(0xb110, 0x32)
void SaveOption(HWND hWnd, Utils::RegistryHelper *pRegistryHelper,
                char *szValueName, DWORD controlId)
{
    if (hWnd && szValueName && pRegistryHelper) {
        pRegistryHelper->SetValueDword(szValueName,
                                       IsDlgButtonChecked(hWnd, controlId));
    }
}

// ---------------------------------------------------------------------------
// SetDefaults
// Unchecks all five "Disable ..." checkboxes. MSVC5 caches the CheckDlgButton
// IAT slot in EDI (one `mov edi,ds:[__imp]; call edi` reused 4x for the four
// `BST_UNCHECKED` calls).
RVA(0xb160, 0x37)
void SetDefaults(HWND hWnd)
{
    CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_SOUND, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, 0);
}

// ---------------------------------------------------------------------------
// LoadOptions
// Reads the five flags from the registry (default 0) and reflects them into the
// checkboxes. GetValueDword is __thiscall; CheckDlgButton's IAT slot is cached
// in EBX. Note the source loads pRegistryHelper into ESI and hWnd into EDI.
RVA(0xb1b0, 0x90)
void LoadOptions(HWND hWnd, Utils::RegistryHelper *pRegistryHelper)
{
    if (pRegistryHelper) {
        CheckDlgButton(hWnd, IDC_DISABLE_VIDEO,
                       pRegistryHelper->GetValueDword("Disable Direct Video Access", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_AUDIO,
                       pRegistryHelper->GetValueDword("Disable Audio", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_SOUND,
                       pRegistryHelper->GetValueDword("Disable Sound", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MUSIC,
                       pRegistryHelper->GetValueDword("Disable Music", 0));
        CheckDlgButton(hWnd, IDC_DISABLE_MOVIE,
                       pRegistryHelper->GetValueDword("Disable High Quality Movie", 0));
    }
}

// ---------------------------------------------------------------------------
// SaveOptions
// Persists all five checkboxes via SaveOption. SaveOption is reached through an
// incremental-link thunk (call rel32 -> jmp -> body); the IAT-less 5x call
// pattern with `add esp,0x10` after each is the __cdecl 4-arg call cleanup.
RVA(0xb270, 0x75)
void SaveOptions(HWND hWnd, Utils::RegistryHelper *pRegistryHelper)
{
    if (pRegistryHelper) {
        SaveOption(hWnd, pRegistryHelper, "Disable Direct Video Access", IDC_DISABLE_VIDEO);
        SaveOption(hWnd, pRegistryHelper, "Disable Audio", IDC_DISABLE_AUDIO);
        SaveOption(hWnd, pRegistryHelper, "Disable Sound", IDC_DISABLE_SOUND);
        SaveOption(hWnd, pRegistryHelper, "Disable Music", IDC_DISABLE_MUSIC);
        SaveOption(hWnd, pRegistryHelper, "Disable High Quality Movie", IDC_DISABLE_MOVIE);
    }
}

// ---------------------------------------------------------------------------
// AdvancedOptionsDialogProc
// INT_PTR CALLBACK (__stdcall, `ret 0x10`). On WM_INITDIALOG it (re)opens the
// HKLM config key, loads the options, and activates the dialog window (setting
// its icon and restoring it if iconic). WM_COMMAND handles OK (save+close),
// Cancel (close), and the "Defaults" button.
RVA(0xafb0, 0x108)
INT_PTR __stdcall AdvancedOptionsDialogProc(HWND hWnd, UINT message,
                                            WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        g_registryHelper.Close();
        g_registryHelper.Open("Monolith Productions", "Gruntz", "1.0", 0,
                              HKEY_LOCAL_MACHINE, 0);
        LoadOptions(hWnd, &g_registryHelper);

        {
            HICON hIcon = LoadIconA(g_hInstance, "GRUNTZ");
            if (hIcon)
                SendMessageA(hWnd, WM_SETICON, 1, (LPARAM)hIcon);
        }
        if (IsIconic(hWnd))
            ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        BringWindowToTop(hWnd);
        return 1;

    case WM_COMMAND:
        if (wParam == 2) {              // IDCANCEL
            EndDialog(hWnd, 0);
            return 1;
        }
        if (wParam == 1) {             // IDOK
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
