// GruntzApp.cpp - Gruntz application object (CGruntzApp, the game's CGameApp
// subclass; C:\Proj\Gruntz). Two methods byte-matched here:
//
//   CGruntzApp::InitializeGameManager @ RVA 0x080a20 (90 B) - allocates and
//       constructs the game manager (operator new + ctor under a C++ EH frame)
//       and returns it.
//   CGruntzApp::ErrorDialogProc       @ RVA 0x080c70 (85 B) - INT_PTR CALLBACK
//       (__stdcall) dialog proc that shows an error string and closes on OK/Cancel.
//
// Only offsets / control IDs / code bytes are load-bearing; class and field
// names are placeholders.
#include "../Wap32/Wap32.h"

// ---------------------------------------------------------------------------
// Minimal Win32 surface (USER32 dialog API). We deliberately do NOT pull in
// <windows.h> - keep the visible symbol SET small (the compiler hashes it;
// entropy follows header churn). Reproduces the FF15 [IAT] direct-call form.
// ---------------------------------------------------------------------------
typedef int INT_PTR;

extern "C" {
__declspec(dllimport) BOOL __stdcall EndDialog(HWND hDlg, INT_PTR nResult);
__declspec(dllimport) BOOL __stdcall SetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
}

#define WM_INITDIALOG 0x0110
#define WM_COMMAND    0x0111

// The control ID of the static text field that displays the error message.
#define IDC_ERROR_TEXT 0x40d

// ---------------------------------------------------------------------------
// The game manager. CGruntzApp::InitializeGameManager allocates 0x0a30 bytes
// (operator new) then runs its (throwing) constructor under a C++ EH frame.
// We only need the new+ctor shape, not the full layout, so a forward class with
// an opaque body of the right size suffices. The ctor is UNMATCHED but the call
// is reloc-masked, so the call bytes are still byte-exact.
// ---------------------------------------------------------------------------
namespace WAP32 {
class CGameMgr {
public:
    CGameMgr();
private:
    char m_pad[0xa30];
};
}

// File-scope globals referenced by ErrorDialogProc (binary: HWND @ 0x64557c and
// the error-text buffer @ 0x644ea0). The relocs that name them are masked in
// objdiff; only the address-load / address-immediate bytes are load-bearing.
static HWND g_errorHwnd;          // 0x64557c - last dialog HWND
static char g_errorText[1];       // 0x644ea0 - error message buffer (push imm)

// ---------------------------------------------------------------------------
// CGruntzApp - the game's CGameApp subclass. Its own fields begin after the
// base (CGameApp ends at 0x254). Neither matched method touches an instance
// field (InitializeGameManager returns the manager rather than storing it; the
// error proc is static), so no CGruntzApp-specific members are needed yet.
// ---------------------------------------------------------------------------
class CGruntzApp : public CGameApp {
public:
    WAP32::CGameMgr *InitializeGameManager();
    static INT_PTR __stdcall ErrorDialogProc(HWND hWnd, UINT message,
                                             WPARAM wParam, LPARAM lParam);
};

// ---------------------------------------------------------------------------
// CGruntzApp::InitializeGameManager  @ RVA 0x080a20 (90 B).
// `return new WAP32::CGameMgr;` - operator new(0xa30) then a throwing ctor under
// a C++ EH frame (this TU needs /GX). The push-ecx is MSVC reserving one dword
// of locals for the new pointer / EH-tracked object; `this` is never read.
// ---------------------------------------------------------------------------
WAP32::CGameMgr *CGruntzApp::InitializeGameManager()
{
    return new WAP32::CGameMgr;
}

// ---------------------------------------------------------------------------
// CGruntzApp::ErrorDialogProc  @ RVA 0x080c70 (85 B).
// INT_PTR CALLBACK (__stdcall, `ret 0x10`). Stashes the HWND unconditionally,
// then: WM_INITDIALOG -> SetDlgItemTextA(hWnd, IDC_ERROR_TEXT, g_errorText);
// WM_COMMAND with IDOK(1)/IDCANCEL(2) -> EndDialog(hWnd, 0). Returns 1 for both
// handled cases, 0 otherwise. The switch reproduces the sub-0x110 / je / dec /
// jne message ladder with the WM_INITDIALOG body laid out at the function tail.
// ---------------------------------------------------------------------------
INT_PTR __stdcall CGruntzApp::ErrorDialogProc(HWND hWnd, UINT message,
                                              WPARAM wParam, LPARAM lParam)
{
    g_errorHwnd = hWnd;

    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemTextA(hWnd, IDC_ERROR_TEXT, g_errorText);
        return 1;

    case WM_COMMAND:
        if (wParam == 1 || wParam == 2) {   // IDOK / IDCANCEL
            EndDialog(hWnd, 0);
            return 1;
        }
        break;
    }

    return 0;
}
