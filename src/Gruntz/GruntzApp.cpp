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
// <Mfc.h> brings <windows.h> USER32 (EndDialog / SetDlgItemTextA / LoadStringA /
// ShowCursor / DialogBoxParamA), INT_PTR, and the WM_INITDIALOG / WM_COMMAND ids.
#include <Mfc.h>
#include <Wap32/Wap32.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strlen/strcpy/strcat (rep movs/scas)

// The control ID of the static text field that displays the error message.
#define IDC_ERROR_TEXT 0x40d

// Default resource string id used when the requested error id is 0 / missing.
#define IDS_DEFAULT_ERROR 0x8009

// ---------------------------------------------------------------------------
// The game manager. CGruntzApp::InitializeGameManager allocates the derived
// CGruntzMgr (0x0a30 bytes; definition in <Gruntz/GruntzMgr.h>) then runs its
// throwing constructor under a C++ EH frame. `new CGruntzMgr` => operator
// new(0xa30) + ??0CGruntzMgr@@QAE@XZ; the pointer is upcast to the base
// WAP32::CGameMgr* the slot returns (a no-op since CGameMgr is the first base).
// ---------------------------------------------------------------------------
#include <Gruntz/GruntzMgr.h>

// File-scope globals referenced by ErrorDialogProc / ShowError (an HWND and the
// error-text buffer). The relocs that name them are masked in objdiff; only the
// address-load / address-immediate bytes are load-bearing.
static HWND g_errorHwnd;        // last dialog HWND
static char g_errorText[0x100]; // error message buffer
// (g_gameAppInstanceCount is declared in Wap32.h, defined in
// GameApp.cpp; ~CGruntzApp's inlined base ~CGameApp decrements it.)

// CGruntzApp - the game's CGameApp subclass, defined once in <Gruntz/GruntzApp.h>.
// The matched methods touch only BASE CGameApp fields: ShowError reads m_hInstance
// (@+0xc), m_errorCode (error message id @+0x24c) and m_errorDetail (error detail
// @+0x250); the dtor / InitializeGameManager touch no CGruntzApp-specific field.
#include <Gruntz/GruntzApp.h>

// ---------------------------------------------------------------------------
// CGruntzApp::CGruntzApp
// Empty-bodied ctor: chains the base CGameApp ctor, then the compiler stores the
// CGruntzApp vftable (reloc-masked) and returns `this`. No CGruntzApp-specific
// field is set.
//   push esi; mov esi,ecx; call CGameApp::CGameApp; mov [esi],&vftable;
//   mov eax,esi; pop esi; ret
RVA(0x00080850, 0x12)
CGruntzApp::CGruntzApp() {}

// ---------------------------------------------------------------------------
// CGruntzApp::Init
// CGruntzApp's override of the base init virtual (CGameApp vtbl slot +0x8). It
// re-pushes all 7 launch args in order and tail-forwards to the base
// CGameApp::Init (`this` left in ecx untouched),
// then normalises the int result to a bool: `!= 0` emits the
// `neg eax; sbb eax,eax; neg eax` (0/1) idiom.
RVA(0x00080930, 0x31)
i32 CGruntzApp::Init(
    HINSTANCE hInstance,
    char* szWindowName,
    char* szGameIdentifier,
    char* szCmdLine,
    i32 windowClassFlags,
    i32 windowWidth,
    i32 windowHeight
) {
    return CGameApp::Init(
               hInstance,
               szWindowName,
               szGameIdentifier,
               szCmdLine,
               windowClassFlags,
               windowWidth,
               windowHeight
           )
           != 0;
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
RVA(0x000808b0, 0x60)
CGruntzApp::~CGruntzApp() {
    CloseResources();
}

// ---------------------------------------------------------------------------
// CGruntzApp::ShowError
// Virtual override. Builds the error
// message into g_errorText then shows the ERROR dialog:
//   id = m_errorCode ? m_errorCode : IDS_DEFAULT_ERROR;     // +0x24c, default 0x8009
//   detail[0] = 0; if (m_errorDetail > 0) sprintf(detail, "(%i)", m_errorDetail);  // +0x250
//   if (LoadStringA(m_hInstance, id, g_errorText, 0xfa) <= 0 &&
//       LoadStringA(m_hInstance, 0x8009, g_errorText, 0xfa) <= 0)
//       strcpy(g_errorText, "Unable to continue game.");
//   strcat(g_errorText, detail);
//   while (ShowCursor(TRUE) < 0) ;              // force the cursor visible
//   DialogBoxParamA(m_hInstance, "ERROR", 0, ErrorDialogProc, 0);
// LoadStringA/ShowCursor/DialogBoxParamA are FF15 [IAT] indirect calls; the
// ErrorDialogProc address is taken (push imm of its incremental-link thunk).
// strcpy/strcat are emitted inline (repnz scas / rep movs).
RVA(0x00080ac0, 0xf3)
void CGruntzApp::ShowError() {
    // The two error fields are read up front (the optimiser hoists the m_errorDetail
    // load above the id-default branch, keeping it live in eax across it).
    i32 id = m_errorCode;
    i32 detailVal = m_errorDetail;
    if (id == 0) {
        id = IDS_DEFAULT_ERROR;
    }

    char detail[0x20];
    detail[0] = 0;
    if (detailVal > 0) {
        sprintf(detail, "(%i)", detailVal);
    }

    if (LoadStringA(m_hInstance, id, g_errorText, 0xfa) <= 0
        && LoadStringA(m_hInstance, IDS_DEFAULT_ERROR, g_errorText, 0xfa) <= 0) {
        strcpy(g_errorText, "Unable to continue game.");
    }

    strcat(g_errorText, detail);

    while (ShowCursor(1) < 0)
        ;

    DialogBoxParamA(m_hInstance, "ERROR", 0, &CGruntzApp::ErrorDialogProc, 0);
}

// ---------------------------------------------------------------------------
// CGruntzApp::InitializeGameManager
// `return new CGruntzMgr;` - operator new(0xa30) then the throwing CGruntzMgr
// ctor under a C++ EH frame (this TU needs /GX). The push-ecx is MSVC reserving
// one dword of locals for the new pointer / EH-tracked object; `this` is never
// read. The CGruntzMgr* is returned as the base WAP32::CGameMgr* the virtual
// slot is typed to (no-op upcast; CGameMgr is the first base).
RVA(0x00080a20, 0x5a)
WAP32::CGameMgr* CGruntzApp::InitializeGameManager() {
    return new CGruntzMgr;
}

// ---------------------------------------------------------------------------
// CGruntzApp::ErrorDialogProc
// INT_PTR CALLBACK (__stdcall, `ret 0x10`). Stashes the HWND unconditionally,
// then: WM_INITDIALOG -> SetDlgItemTextA(hWnd, IDC_ERROR_TEXT, g_errorText);
// WM_COMMAND with IDOK(1)/IDCANCEL(2) -> EndDialog(hWnd, 0). Returns 1 for both
// handled cases, 0 otherwise. The switch reproduces the sub-0x110 / je / dec /
// jne message ladder with the WM_INITDIALOG body laid out at the function tail.
// (No SYMBOL() override: the real HWND signature mangles to PAUHWND__ identically
// on both sides - like the sibling DialogProcs - so the natural name pairs.)
RVA(0x00080c70, 0x55)
INT_PTR CALLBACK
CGruntzApp::ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    g_errorHwnd = hWnd;

    switch (message) {
        case WM_INITDIALOG:
            SetDlgItemTextA(hWnd, IDC_ERROR_TEXT, g_errorText);
            return 1;

        case WM_COMMAND:
            if (wParam == IDOK || wParam == IDCANCEL) {
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
RVA(0x00080aa0, 0x5)
i32 CGruntzApp::VirtualUnknownMethod11(i32 a, i32 b, i32 c) {
    return 0;
}

// ---------------------------------------------------------------------------
// CGruntzApp::ShowMessage
// Copies the message into the shared g_errorText buffer (inline strcpy) then
// shows the MESSAGE dialog (DialogBoxParamA, FF15 [IAT]) with MsgDialogProc -
// the dialog proc lives in another TU, so it is taken via an extern thunk.
extern "C" INT_PTR CALLBACK MsgDialogProc(HWND, UINT, WPARAM, LPARAM);
RVA(0x00080c00, 0x48)
void CGruntzApp::ShowMessage(char* msg, HWND hParent) {
    strcpy(g_errorText, msg);
    DialogBoxParamA(m_hInstance, "MESSAGE", hParent, &MsgDialogProc, 0);
}

// ---------------------------------------------------------------------------
// CreateU10O
// Free function `void *CreateU10O()`: `return new U10O;`
// - operator new(sizeof(U10O)) then a throwing ctor under a C++ EH frame, then
// returns the raw pointer. Only the new+ctor shape is load-bearing; a forward
// class with a declared ctor suffices to give `new U10O` a size + ctor call.
struct U10O {
    U10O();
    char m_pad[0x10]; // 0x10-byte object (the `new` size operand)
};
RVA(0x000809a0, 0x57)
void* CreateU10O() {
    U10O* p = new U10O;
    return p;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// Boolified forward to the switch-down-sprite loader (thiscall on `this`, retail
// 0x110570). `!= 0` lowers to the int->bool neg/sbb/neg normalize.
RVA(0x00112820, 0xc)
i32 CGruntzApp::TryLoadSwitchDownSprite() {
    return LoadSwitchDownSprite() != 0;
}

// size 0x254 recovered from operator-new sites (gruntz.analysis.news)

SIZE_UNKNOWN(U10O);
