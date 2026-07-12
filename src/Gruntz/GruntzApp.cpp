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

// The dialog HWND that ErrorDialogProc caches @0x24557c is the SHARED active-dialog
// HWND NetLobby::g_curDlg_64557c (every dialog proc - error/lobby/checkpoint/
// custom-world - caches its HWND at the SAME .bss slot; DATA home Net/LobbyDialogs.cpp).
// It is a cross-TU global, not a GruntzApp static: modeling it as a local `g_errorHwnd`
// static split 0x24557c into a second name (`_g_errorHwnd$S18951`) that won the per-RVA
// keep-last and left the NetLobby users (CheckpointDlg/CustomWorldDialog/GruntzWnd)
// UNBOUND. Unified onto the shared extern here (DIR32 masked - byte-neutral).
namespace NetLobby {
    extern HWND g_curDlg_64557c;
}
// The error-text buffer @0x244ea0 is a GruntzApp file-static in retail. Bind it by RVA
// via a STABLE symbol name: as a C++ `static` it mangles to `_g_errorText$S<idx>`, whose
// per-TU index cl5 RENUMBERS on any string-pool change (measured 18949->18953->18964...),
// so a `@data-symbol: _g_errorText$S<n>` bind silently goes UNBOUND on the next pool
// churn. `extern "C"` gives the fixed, renumber-proof `_g_errorText`; DATA() pins it to
// 0x244ea0. The buffer is only address-taken here (no init/sizeof), so a definition-free
// extern is sufficient - the DIR32 loads/stores stay reloc-masked (byte-neutral).
// error message buffer @ 0x244ea0, DEFINED here (owner TU) with C linkage (_g_errorText).
// The extern "C" block keeps the fixed, renumber-proof symbol while the def line itself
// carries no `extern` keyword (a real, initialized definition).
extern "C" {
DATA(0x00244ea0)
char g_errorText[0x100] = {0};
}
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
    // CloseResources is NOT overridden by CGruntzApp - the retail dtor calls the
    // inherited base CGameApp::CloseResources directly (0x13d8c0); the explicit
    // base qualification binds that real callee (devirtualized, same call bytes).
    CGameApp::CloseResources();
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
//
// The pushed DLGPROC is ErrorDialogProc's ILT jmp-thunk (0x33c8 -> jmp 0x80c70), not
// the body - the retail /INCREMENTAL link routes an address-taken function through
// its thunk. Modeled as an extern-C thunk symbol bound to 0x33c8 (the same idiom as
// GameObjectFactory's _CreateXxx thunks); ShowMessage takes the SAME proc's address
// (both dialogs share ErrorDialogProc), so it pushes the same thunk.
// @data-symbol: _ErrorDialogProcThunk@16 0x000033c8
extern "C" INT_PTR CALLBACK ErrorDialogProcThunk(HWND, UINT, WPARAM, LPARAM);
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

    DialogBoxParamA(m_hInstance, "ERROR", 0, &ErrorDialogProcThunk, 0);
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
    NetLobby::g_curDlg_64557c = hWnd;

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

// CGruntzApp::VirtualUnknownMethod11 (0x00080aa0) is now an inline member in the header.


// ---------------------------------------------------------------------------
// CGruntzApp::ShowMessage
// Copies the message into the shared g_errorText buffer (inline strcpy) then shows
// the MESSAGE dialog (DialogBoxParamA, FF15 [IAT]). The dialog proc is the SAME
// ErrorDialogProc as the error dialog - the retail push here is the identical ILT
// thunk 0x33c8 -> jmp 0x80c70 (the former separate "MsgDialogProc" was a misnomer).
RVA(0x00080c00, 0x48)
void CGruntzApp::ShowMessage(char* msg, HWND hParent) {
    strcpy(g_errorText, msg);
    DialogBoxParamA(m_hInstance, "MESSAGE", hParent, &ErrorDialogProcThunk, 0);
}

// ---------------------------------------------------------------------------
// CreateU10O
// Free function `void *CreateU10O()`: `return new CGruntzWnd;` - operator
// new(0x10) then the throwing CGruntzWnd ctor (0x94640, ??0CGruntzWnd@@QAE@XZ -
// stamps ??_7CGruntzWnd@@6B@) under a C++ EH frame, returning the raw pointer.
// The object is the real CGruntzWnd (the game window, a CGameWnd subclass that
// adds no fields, so sizeof == the 0x10 CGameWnd base = the `new` size operand).
// Its canonical definition is .cpp-local in GruntzWnd.cpp (no shared header yet);
// a reduced forward view here gives `new CGruntzWnd` the size + ctor call, binds
// the ctor reloc to 0x94640, and (deriving CGameWnd) keeps the RTTI hierarchy
// honest for the vtable-audit. The ctor is external (no body here).
struct CGruntzWnd : public CGameWnd {
    CGruntzWnd();
};
RVA(0x000809a0, 0x57)
void* CreateU10O() {
    CGruntzWnd* p = new CGruntzWnd;
    return p;
}

// CGruntzApp::TryLoadSwitchDownSprite (0x00112820) is now an inline member in the header.
