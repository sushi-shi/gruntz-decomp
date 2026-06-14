// AdvancedOptions.cpp - Gruntz "Advanced Options" modal dialog (C:\Proj\Gruntz).
// The dialog lets the user toggle five "Disable ..." flags, persisted under the
// HKLM\Software\Monolith Productions\Gruntz\1.0 registry key via the engine's
// Utils::RegistryHelper wrapper (src/Utils/RegistryHelper.{cpp,h}, byte-matched).
//
// Matched (all byte-exact unless noted):
//   AdvancedOptionsDialogProc @ RVA 0x00afb0 (264 B) - INT_PTR CALLBACK dialog proc
//   SaveOption                @ RVA 0x00b110 ( 50 B) - one checkbox -> RegDWORD
//   SetDefaults               @ RVA 0x00b160 ( 55 B) - clear all five checkboxes
//   LoadOptions               @ RVA 0x00b1b0 (144 B) - 5x GetValueDword -> checkbox
//   SaveOptions               @ RVA 0x00b270 (117 B) - 5x SaveOption
//
// Only offsets / control IDs / code bytes are load-bearing; names are placeholders.
#include "../Utils/RegistryHelper.h"

// ---------------------------------------------------------------------------
// Minimal Win32 surface (USER32 dialog API). We deliberately do NOT pull in
// <windows.h> - keep the visible symbol SET small (the compiler hashes it;
// entropy follows header churn - see docs/matching-patterns.md). This
// reproduces the FF15 [IAT] direct-call form for the imports.
// ---------------------------------------------------------------------------
typedef void *         HWND;
typedef void *         HINSTANCE;
typedef void *         HICON;
typedef unsigned int   UINT;
typedef int            INT_PTR;
typedef unsigned int   WPARAM;
typedef long           LPARAM;
typedef long           LRESULT;

extern "C" {
__declspec(dllimport) BOOL __stdcall EndDialog(HWND hDlg, INT_PTR nResult);
__declspec(dllimport) UINT __stdcall IsDlgButtonChecked(HWND hDlg, int nIDButton);
__declspec(dllimport) BOOL __stdcall CheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck);
__declspec(dllimport) HICON __stdcall LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName);
__declspec(dllimport) LRESULT __stdcall SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
__declspec(dllimport) BOOL __stdcall IsIconic(HWND hWnd);
__declspec(dllimport) BOOL __stdcall ShowWindow(HWND hWnd, int nCmdShow);
__declspec(dllimport) BOOL __stdcall SetForegroundWindow(HWND hWnd);
__declspec(dllimport) HWND __stdcall BringWindowToTop(HWND hWnd);
}

// Win32 message / constant literals (kept local; not from <windows.h>).
#define WM_INITDIALOG 0x0110
#define WM_COMMAND    0x0111
#define WM_SETICON    0x0080
#define BST_CHECKED   0x0001
#define SW_RESTORE    9
#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)

// Control IDs of the five "Disable ..." checkboxes and the "Defaults" button.
#define IDC_DISABLE_VIDEO   0x46c
#define IDC_DISABLE_AUDIO   0x46d
#define IDC_DISABLE_SOUND   0x46e
#define IDC_DISABLE_MUSIC   0x46f
#define IDC_DISABLE_MOVIE   0x470
#define IDC_DEFAULTS        0x426

// File-scope globals (binary: data @ 0x6295d8 and @ 0x651618). The reloc that
// names them is masked in objdiff; only the address-load bytes are load-bearing.
static Utils::RegistryHelper g_registryHelper;
static HINSTANCE             g_hInstance;

// ---------------------------------------------------------------------------
// SaveOption  @ RVA 0x00b110 (50 B).
// Reads checkbox `controlId`'s state and writes it as a REG_DWORD value.
// __cdecl free function (ends in `ret`). pRegistryHelper->SetValueDword is
// __thiscall (this in ECX).
// ---------------------------------------------------------------------------
// @address: 0xb110
void SaveOption(HWND hWnd, Utils::RegistryHelper *pRegistryHelper,
                char *szValueName, DWORD controlId)
{
    if (hWnd && szValueName && pRegistryHelper) {
        pRegistryHelper->SetValueDword(szValueName,
                                       IsDlgButtonChecked(hWnd, controlId));
    }
}

// ---------------------------------------------------------------------------
// SetDefaults  @ RVA 0x00b160 (55 B).
// Unchecks all five "Disable ..." checkboxes. MSVC5 caches the CheckDlgButton
// IAT slot in EDI (one `mov edi,ds:[__imp]; call edi` reused 4x for the four
// `BST_UNCHECKED` calls).
// ---------------------------------------------------------------------------
// @address: 0xb160
void SetDefaults(HWND hWnd)
{
    CheckDlgButton(hWnd, IDC_DISABLE_VIDEO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_AUDIO, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_SOUND, 0);
    CheckDlgButton(hWnd, IDC_DISABLE_MUSIC, 0);
}

// ---------------------------------------------------------------------------
// LoadOptions  @ RVA 0x00b1b0 (144 B).
// Reads the five flags from the registry (default 0) and reflects them into the
// checkboxes. GetValueDword is __thiscall; CheckDlgButton's IAT slot is cached
// in EBX. Note the source loads pRegistryHelper into ESI and hWnd into EDI.
// ---------------------------------------------------------------------------
// @address: 0xb1b0
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
// SaveOptions  @ RVA 0x00b270 (117 B).
// Persists all five checkboxes via SaveOption. SaveOption is reached through an
// incremental-link thunk (call rel32 -> jmp -> body); the IAT-less 5x call
// pattern with `add esp,0x10` after each is the __cdecl 4-arg call cleanup.
// ---------------------------------------------------------------------------
// @address: 0xb270
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
// AdvancedOptionsDialogProc  @ RVA 0x00afb0 (264 B).
// INT_PTR CALLBACK (__stdcall, `ret 0x10`). On WM_INITDIALOG it (re)opens the
// HKLM config key, loads the options, and activates the dialog window (setting
// its icon and restoring it if iconic). WM_COMMAND handles OK (save+close),
// Cancel (close), and the "Defaults" button.
// ---------------------------------------------------------------------------
// @address: 0xafb0
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
