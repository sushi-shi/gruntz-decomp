#include <rva.h>

#include <Gruntz/GruntzApp.h>

#include <Mfc.h>

#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzWnd.h>
#include <Net/NetLobby.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

typedef enum GruntzAppResId {
    IDC_ERROR_TEXT = 0x40d,
} GruntzAppResId;

DATA(0x00244ea0)
char g_errorText[0x100] = {0};

RVA(0x00080850, 0x12)
CGruntzApp::CGruntzApp() {}

RVA_COMPGEN(0x00080880, 0x1e, ??_GCGruntzApp@@UAEPAXI@Z)
RVA(0x000808b0, 0x60)
CGruntzApp::~CGruntzApp() {
    CGruntzApp::CloseResources();
}

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

RVA(0x00080980, 0x5)
void CGruntzApp::CloseResources() {
    CGameApp::CloseResources();
}

RVA(0x000809a0, 0x57)
CGameWnd* CGruntzApp::InitializeGameWindow() {
    CGruntzWnd* p = new CGruntzWnd;
    return p;
}
RVA(0x00080a20, 0x5a)
CGameMgr* CGruntzApp::InitializeGameManager() {
    return new CGruntzMgr;
}

RVA(0x00080ac0, 0xf3)
void CGruntzApp::ShowError() {

    i32 id = m_errorCode;
    i32 detailVal = m_errorDetail;
    if (id == 0) {
        id = IDX(IDS_DEFAULT_ERROR);
    }

    char detail[0x20];
    detail[0] = 0;
    if (detailVal > 0) {
        sprintf(detail, " (%i)", detailVal);
    }

    if (LoadStringA(m_hInstance, id, g_errorText, 0xfa) <= 0
        && LoadStringA(m_hInstance, IDX(IDS_DEFAULT_ERROR), g_errorText, 0xfa) <= 0) {
        strcpy(g_errorText, "Unable to continue game.");
    }

    strcat(g_errorText, detail);

    while (ShowCursor(1) < 0)
        ;

    DialogBoxParamA(m_hInstance, "ERROR", 0, CGruntzApp::ErrorDialogProc, 0);
}

RVA(0x00080c00, 0x48)
void CGruntzApp::ShowMessage(const char* msg, HWND hParent) {
    strcpy(g_errorText, msg);
    DialogBoxParamA(m_hInstance, "MESSAGE", hParent, CGruntzApp::ErrorDialogProc, 0);
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

RVA_COMPGEN(0x00080cf0, 0x12, ??1CGameApp@@UAE@XZ)
RVA_COMPGEN(0x00080db0, 0x1, ?ShowError@CGameApp@@UAEXXZ)
RVA_COMPGEN(0x00080dd0, 0x32, ??_GCGameApp@@UAEPAXI@Z)
