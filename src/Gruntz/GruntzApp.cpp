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

DATA(0x00245df8)
char g_errorText[0x100] = {0};

RVA(0x00080770, 0x12)
CGruntzApp::CGruntzApp() {}

RVA_COMPGEN(0x000807a0, 0x1e, ??_GCGruntzApp@@UAEPAXI@Z)
RVA(0x000807d0, 0x60)
CGruntzApp::~CGruntzApp() {
    CGruntzApp::CloseResources();
}

RVA(0x00080850, 0x31)
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

RVA(0x000808a0, 0x5)
void CGruntzApp::CloseResources() {
    CGameApp::CloseResources();
}

RVA(0x000808c0, 0x57)
CGameWnd* CGruntzApp::InitializeGameWindow() {
    CGruntzWnd* p = new CGruntzWnd;
    return p;
}
RVA(0x00080940, 0x5a)
CGameMgr* CGruntzApp::InitializeGameManager() {
    return new CGruntzMgr;
}

RVA(0x000809e0, 0xf3)
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

    while (ShowCursor(true) < 0)
        ;

    DialogBoxParamA(m_hInstance, "ERROR", NULL, CGruntzApp::ErrorDialogProc, 0);
}

RVA(0x00080b20, 0x48)
void CGruntzApp::ShowMessage(const char* msg, HWND hParent) {
    strcpy(g_errorText, msg);
    DialogBoxParamA(m_hInstance, "MESSAGE", hParent, CGruntzApp::ErrorDialogProc, 0);
}

RVA(0x00080b90, 0x55)
BOOL CALLBACK CGruntzApp::ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hWnd;

    switch (message) {
        case WM_INITDIALOG:
            SetDlgItemTextA(hWnd, IDC_ERROR_TEXT, g_errorText);
            return true;

        case WM_COMMAND:
            if (wParam == IDOK || wParam == IDCANCEL) {
                EndDialog(hWnd, 0);
                return true;
            }
            break;
    }

    return false;
}

RVA_COMPGEN(0x00080c10, 0x12, ??1CGameApp@@UAE@XZ)
RVA_COMPGEN(0x00080cf0, 0x32, ??_GCGameApp@@UAEPAXI@Z)
