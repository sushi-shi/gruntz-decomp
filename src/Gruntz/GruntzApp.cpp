#include <Mfc.h>
#include <Wap32/Wap32.h>
#include <Gruntz/GruntzWnd.h> // the real CGruntzWnd (InitializeGameWindow news it)
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strlen/strcpy/strcat (rep movs/scas)

typedef enum GruntzAppResId {
    IDC_ERROR_TEXT = 0x40d,     // static text field displaying the error message
    IDS_DEFAULT_ERROR = 0x8009, // default string-resource id when the requested id is 0/missing
} GruntzAppResId;

#include <Gruntz/GruntzMgr.h>

#include <Net/NetLobby.h> // NetLobby::g_curDlg
// The error-text buffer @0x244ea0 is a GruntzApp file-static in retail. Bind it by RVA
// via a STABLE symbol name: as a C++ `static` it mangles to `_g_errorText$S<idx>`, whose
// per-TU index cl5 RENUMBERS on any string-pool change (measured 18949->18953->18964...),
// so a `// churn. `extern "C"` gives the fixed, renumber-proof `_g_errorText`; DATA() pins it to
// 0x244ea0. The buffer is only address-taken here (no init/sizeof), so a definition-free
// extern is sufficient - the DIR32 loads/stores stay reloc-masked (byte-neutral).
// error message buffer @ 0x244ea0, DEFINED here (owner TU) with C linkage (_g_errorText).
// The extern "C" block keeps the fixed, renumber-proof symbol while the def line itself
// carries no `extern` keyword (a real, initialized definition).
VTBL(CGruntzApp, 0x001e9ab4); // vtable_names -> code (RTTI game class)
DATA(0x00244ea0)
char g_errorText[0x100] = {0};

#include <Gruntz/GruntzApp.h>

RVA(0x00080850, 0x12)
CGruntzApp::CGruntzApp() {}

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

RVA(0x000808b0, 0x60)
CGruntzApp::~CGruntzApp() {
    // The retail dtor calls the BASE body directly (0x13d8c0) - the qualified
    // spelling keeps that direct binding (the CGruntzApp override is @0x80980).
    CGameApp::CloseResources();
}

// Slot-4 override: a pure forwarder - cl /O2 tail-jumps the base body (retail
// 0x80980 = `jmp 0x13d8c0`, reached via ILT thunk 0x1b8b from the vtable).
RVA(0x00080980, 0x5)
void CGruntzApp::CloseResources() {
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
DATA_SYMBOL(0x000033c8, 0x0, _ErrorDialogProcThunk@16)
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

RVA(0x00080a20, 0x5a)
CGameMgr* CGruntzApp::InitializeGameManager() {
    return new CGruntzMgr;
}

RVA(0x00080c70, 0x55)
INT_PTR CALLBACK
CGruntzApp::ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hWnd;

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

RVA(0x00080c00, 0x48)
void CGruntzApp::ShowMessage(const char* msg, HWND hParent) {
    strcpy(g_errorText, msg);
    DialogBoxParamA(m_hInstance, "MESSAGE", hParent, &ErrorDialogProcThunk, 0);
}

RVA(0x000809a0, 0x57)
CGameWnd* CGruntzApp::InitializeGameWindow() {
    CGruntzWnd* p = new CGruntzWnd;
    return p;
}
