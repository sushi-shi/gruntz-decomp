#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntzApp.h>

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

char g_errorText[0x100] = {0};

CGruntzApp::CGruntzApp() {}

CGruntzApp::~CGruntzApp() {
    CGruntzApp::CloseResources();
}

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

void CGruntzApp::CloseResources() {
    CGameApp::CloseResources();
}

CGameWnd* CGruntzApp::InitializeGameWindow() {
    CGruntzWnd* p = new CGruntzWnd;
    return p;
}

CGameMgr* CGruntzApp::InitializeGameManager() {
    return new CGruntzMgr;
}

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

    DialogBoxA(m_hInstance, "ERROR", NULL, CGruntzApp::ErrorDialogProc);
}

void CGruntzApp::ShowMessage(const char* msg, HWND hParent) {
    strcpy(g_errorText, msg);
    DialogBoxA(m_hInstance, "MESSAGE", hParent, CGruntzApp::ErrorDialogProc);
}

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
