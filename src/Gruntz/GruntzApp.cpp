// GruntzApp.cpp - Gruntz application object (CGruntzApp, the game's CGameApp
// subclass; C:\Proj\Gruntz). Methods byte-matched here:
//
//   CGruntzApp::InitializeGameManager - allocates and constructs the game
//       manager (operator new + ctor under a C++ EH frame) and returns it.
//   CGruntzApp::ErrorDialogProc - INT_PTR CALLBACK (__stdcall) dialog proc that
//       shows an error string and closes on OK/Cancel.
//   CGruntzApp::~CGruntzApp - virtual dtor; runs CloseResources() then chains the
//       (inlined) base ~CGameApp.
//   CGruntzApp::ShowError - virtual override; builds the error string (LoadStringA
//       / fallback literal + "(%i)" detail), forces the cursor visible, then
//       DialogBoxParamA(.., ErrorDialogProc, ..).
//
// Only offsets / control IDs / code bytes are load-bearing; class and field
// names are placeholders.
#include <Wap32/Wap32.h>
#include <rva.h>
#include <stdio.h>   // engine sprintf (reloc-masked)
#include <string.h>  // inline strlen/strcpy/strcat (rep movs/scas)

// ---------------------------------------------------------------------------
// Minimal Win32 surface (USER32 API). We deliberately do NOT pull in
// <windows.h> - keep the visible symbol SET small (the compiler hashes it;
// entropy follows header churn). Reproduces the FF15 [IAT] direct-call form.
// ---------------------------------------------------------------------------
typedef int INT_PTR;
typedef INT_PTR (__stdcall *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

extern "C" {
__declspec(dllimport) BOOL __stdcall EndDialog(HWND hDlg, INT_PTR nResult);
__declspec(dllimport) BOOL __stdcall SetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
__declspec(dllimport) int __stdcall LoadStringA(HINSTANCE hInstance, UINT uID, char *lpBuffer, int cchBufferMax);
__declspec(dllimport) int __stdcall ShowCursor(BOOL bShow);
__declspec(dllimport) INT_PTR __stdcall DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName,
                                                        HWND hWndParent, DLGPROC lpDialogFunc,
                                                        LPARAM dwInitParam);
}

#define WM_INITDIALOG 0x0110
#define WM_COMMAND    0x0111

// The control ID of the static text field that displays the error message.
#define IDC_ERROR_TEXT 0x40d

// Default resource string id used when the requested error id is 0 / missing.
#define IDS_DEFAULT_ERROR 0x8009

// ---------------------------------------------------------------------------
// The game manager. CGruntzApp::InitializeGameManager allocates the complete
// WAP32::CGameMgr size (0x0a30 bytes; definition in Wap32.h) then runs its
// throwing constructor under a C++ EH frame. The ctor is UNMATCHED but the call
// is reloc-masked, so the call bytes are still byte-exact.
// ---------------------------------------------------------------------------

// File-scope globals referenced by ErrorDialogProc / ShowError (an HWND and the
// error-text buffer). The relocs that name them are masked in objdiff; only the
// address-load / address-immediate bytes are load-bearing.
static HWND g_errorHwnd;          // last dialog HWND
static char g_errorText[0x100];   // error message buffer
// (g_gameAppInstanceCount is declared in Wap32.h, defined in
// GameApp.cpp; ~CGruntzApp's inlined base ~CGameApp decrements it.)

// CGruntzApp - the game's CGameApp subclass, defined once in <Gruntz/GruntzApp.h>.
// The matched methods touch only BASE CGameApp fields: ShowError reads m_c
// (hInstance @+0xc), m_24c (error message id @+0x24c) and m_250 (error detail
// @+0x250); the dtor / InitializeGameManager touch no CGruntzApp-specific field.
#include <Gruntz/GruntzApp.h>

// ---------------------------------------------------------------------------
// CGruntzApp::CGruntzApp
// Empty-bodied ctor: chains the base CGameApp ctor, then the compiler stores the
// CGruntzApp vftable (reloc-masked) and returns `this`. No CGruntzApp-specific
// field is set.
//   push esi; mov esi,ecx; call CGameApp::CGameApp; mov [esi],&vftable;
//   mov eax,esi; pop esi; ret
RVA(0x80850, 0x12)
CGruntzApp::CGruntzApp()
{
}

// ---------------------------------------------------------------------------
// CGruntzApp::VirtualUnknownMethod03
// CGruntzApp's override of the base init virtual (CGameApp vtbl slot +0x8). It
// re-pushes all 7 launch args in order and tail-forwards to the base
// CGameApp::VirtualUnknownMethod03 (`this` left in ecx untouched),
// then normalises the int result to a bool: `!= 0` emits the
// `neg eax; sbb eax,eax; neg eax` (0/1) idiom.
RVA(0x80930, 0x31)
int CGruntzApp::VirtualUnknownMethod03(HINSTANCE hInstance, char *szWindowName,
                                       char *szGameIdentifier, char *szCmdLine,
                                       int windowClassFlags, int windowWidth,
                                       int windowHeight)
{
    return CGameApp::VirtualUnknownMethod03(hInstance, szWindowName,
                                            szGameIdentifier, szCmdLine,
                                            windowClassFlags, windowWidth,
                                            windowHeight) != 0;
}

// ---------------------------------------------------------------------------
// CGruntzApp::~CGruntzApp
// Virtual dtor. Two-phase teardown under a C++
// EH frame: restore the CGruntzApp vftable, run this class's own body
// (CloseResources(), devirtualized to CGameApp::CloseResources), then
// the base subobject ~CGameApp runs - inlined here (CloseResources() again +
// the instance-counter decrement) with the base vftable restored. So
// CloseResources is called TWICE (once by each level's body) - that is the real
// engine code, and CloseResources is what actually `delete`s the game manager
// (CGameApp::m_8 @+0x8). No game-manager-pointer member lives on CGruntzApp.
RVA(0x808b0, 0x60)
CGruntzApp::~CGruntzApp()
{
    CloseResources();
}

// ---------------------------------------------------------------------------
// CGruntzApp::ShowError
// Virtual override. Builds the error
// message into g_errorText then shows the ERROR dialog:
//   id = m_24c ? m_24c : IDS_DEFAULT_ERROR;     // +0x24c, default 0x8009
//   detail[0] = 0; if (m_250 > 0) sprintf(detail, "(%i)", m_250);  // +0x250
//   if (LoadStringA(m_c, id, g_errorText, 0xfa) <= 0 &&
//       LoadStringA(m_c, 0x8009, g_errorText, 0xfa) <= 0)
//       strcpy(g_errorText, "Unable to continue game.");
//   strcat(g_errorText, detail);
//   while (ShowCursor(TRUE) < 0) ;              // force the cursor visible
//   DialogBoxParamA(m_c, "ERROR", 0, ErrorDialogProc, 0);
// LoadStringA/ShowCursor/DialogBoxParamA are FF15 [IAT] indirect calls; the
// ErrorDialogProc address is taken (push imm of its incremental-link thunk).
// strcpy/strcat are emitted inline (repnz scas / rep movs).
RVA(0x80ac0, 0xf3)
void CGruntzApp::ShowError()
{
    // The two error fields are read up front (the optimiser hoists the m_250
    // load above the id-default branch, keeping it live in eax across it).
    int id = m_24c;
    int detailVal = m_250;
    if (id == 0)
        id = IDS_DEFAULT_ERROR;

    char detail[0x20];
    detail[0] = 0;
    if (detailVal > 0)
        sprintf(detail, "(%i)", detailVal);

    if (LoadStringA(m_c, id, g_errorText, 0xfa) <= 0 &&
        LoadStringA(m_c, IDS_DEFAULT_ERROR, g_errorText, 0xfa) <= 0)
        strcpy(g_errorText, "Unable to continue game.");

    strcat(g_errorText, detail);

    while (ShowCursor(1) < 0)
        ;

    DialogBoxParamA(m_c, "ERROR", 0, &CGruntzApp::ErrorDialogProc, 0);
}

// ---------------------------------------------------------------------------
// CGruntzApp::InitializeGameManager
// `return new WAP32::CGameMgr;` - operator new(0xa30) then a throwing ctor under
// a C++ EH frame (this TU needs /GX). The push-ecx is MSVC reserving one dword
// of locals for the new pointer / EH-tracked object; `this` is never read.
RVA(0x80a20, 0x5a)
WAP32::CGameMgr *CGruntzApp::InitializeGameManager()
{
    return new WAP32::CGameMgr;
}

// ---------------------------------------------------------------------------
// CGruntzApp::ErrorDialogProc
// INT_PTR CALLBACK (__stdcall, `ret 0x10`). Stashes the HWND unconditionally,
// then: WM_INITDIALOG -> SetDlgItemTextA(hWnd, IDC_ERROR_TEXT, g_errorText);
// WM_COMMAND with IDOK(1)/IDCANCEL(2) -> EndDialog(hWnd, 0). Returns 1 for both
// handled cases, 0 otherwise. The switch reproduces the sub-0x110 / je / dec /
// jne message ladder with the WM_INITDIALOG body laid out at the function tail.
SYMBOL(?ErrorDialogProc@CGruntzApp@@SGHPAXIIJ@Z)
RVA(0x80c70, 0x55)
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

// ---------------------------------------------------------------------------
// CGruntzApp::VirtualUnknownMethod04
// Another base-init virtual override; the body just returns 0 (`xor eax,eax`).
RVA(0x80aa0, 0x5)
int CGruntzApp::VirtualUnknownMethod04(int a, int b, int c)
{
    return 0;
}

// ---------------------------------------------------------------------------
// CGruntzApp::ShowMessage
// Copies the message into the shared g_errorText buffer (inline strcpy) then
// shows the MESSAGE dialog (DialogBoxParamA, FF15 [IAT]) with MsgDialogProc -
// the dialog proc lives in another TU, so it is taken via an extern thunk.
extern "C" INT_PTR __stdcall MsgDialogProc(HWND, UINT, WPARAM, LPARAM);
RVA(0x80c00, 0x48)
void CGruntzApp::ShowMessage(char *msg, HWND hParent)
{
    strcpy(g_errorText, msg);
    DialogBoxParamA(m_c, "MESSAGE", hParent, &MsgDialogProc, 0);
}

// ---------------------------------------------------------------------------
// CreateU10O
// Free function `void *CreateU10O()`: `return new U10O;`
// - operator new(sizeof(U10O)) then a throwing ctor under a C++ EH frame, then
// returns the raw pointer. Only the new+ctor shape is load-bearing; a forward
// class with a declared ctor suffices to give `new U10O` a size + ctor call.
struct U10O { U10O(); };
RVA(0x809a0, 0x57)
void *CreateU10O()
{
    U10O *p = new U10O;
    return p;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: tomalla
// @stub
RVA(0x112820, 0xc)
void CGruntzApp::Stub_112820() {}
