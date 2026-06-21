// GruntzApp.h - CGruntzApp, the game's CGameApp subclass (C:\Proj\Gruntz). One
// canonical definition: WinMain.cpp `new`s it and drives slots 2/6
// (VirtualUnknownMethod03 / RunMessageLoop); GruntzApp.cpp implements the
// overrides. Its own fields begin after the base (CGameApp ends at 0x254); the
// matched methods touch only base CGameApp fields, so no CGruntzApp-specific
// members are modeled yet.
#ifndef GRUNTZ_GRUNTZ_GRUNTZAPP_H
#define GRUNTZ_GRUNTZ_GRUNTZAPP_H
#include <rva.h>  // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
// <Mfc.h> brings <windows.h> (HWND / HINSTANCE / UINT / WPARAM / LPARAM) and INT_PTR
// (the dialog proc's return type).
#include <Mfc.h>
#include <Wap32/Wap32.h>

class CGruntzApp : public CGameApp {
public:
    CGruntzApp();                                     // ctor
    virtual ~CGruntzApp() OVERRIDE;                            // vtbl +0x00
    // Override of the base init virtual (CGameApp slot +0x08): forwards all 7
    // launch args to CGameApp::VirtualUnknownMethod03 and normalises the result
    // to a bool. WinMain calls this as the app "Init".
    virtual int VirtualUnknownMethod03(HINSTANCE hInstance, char *szWindowName,
                                       char *szGameIdentifier, char *szCmdLine,
                                       int windowClassFlags, int windowWidth,
                                       int windowHeight) OVERRIDE;                 // vtbl +0x08
    virtual void ShowError() OVERRIDE;                         // vtbl +0x30
    // Another base-init virtual override; just returns 0.
    virtual int VirtualUnknownMethod04(int a, int b, int c);
    // Shows the MESSAGE dialog with an arbitrary message string.
    void ShowMessage(char *msg, HWND hParent);
    WAP32::CGameMgr *InitializeGameManager() OVERRIDE;
    static INT_PTR __stdcall ErrorDialogProc(HWND hWnd, UINT message,
                                             WPARAM wParam, LPARAM lParam);

    // Engine-label backlog stubs.
    void Stub_112820();
};

#endif  // GRUNTZ_GRUNTZ_GRUNTZAPP_H
