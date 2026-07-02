// GruntzApp.h - CGruntzApp, the game's CGameApp subclass (C:\Proj\Gruntz). One
// canonical definition: WinMain.cpp `new`s it and drives slots 2/6
// (VirtualUnknownMethod03 / RunMessageLoop); GruntzApp.cpp implements the
// overrides. Its own fields begin after the base (CGameApp ends at 0x254); the
// matched methods touch only base CGameApp fields, so no CGruntzApp-specific
// members are modeled yet.
#ifndef GRUNTZ_GRUNTZ_GRUNTZAPP_H
#define GRUNTZ_GRUNTZ_GRUNTZAPP_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
// <Mfc.h> brings <windows.h> (HWND / HINSTANCE / UINT / WPARAM / LPARAM) and INT_PTR
// (the dialog proc's return type).
#include <Mfc.h>
#include <Wap32/Wap32.h>

SIZE(CGruntzApp, 0x254);
class CGruntzApp : public CGameApp {
public:
    CGruntzApp();                   // ctor
    virtual ~CGruntzApp() OVERRIDE; // vtbl +0x00
    // Override of the base init virtual (CGameApp slot +0x08): forwards all 7
    // launch args to CGameApp::VirtualUnknownMethod03 and normalises the result
    // to a bool. WinMain calls this as the app "Init".
    virtual i32 VirtualUnknownMethod03(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    ) OVERRIDE;                        // vtbl +0x08
    virtual void ShowError() OVERRIDE; // vtbl +0x30
    // Another base-init virtual override; just returns 0.
    virtual i32 VirtualUnknownMethod04(i32 a, i32 b, i32 c);
    // Shows the MESSAGE dialog with an arbitrary message string.
    void ShowMessage(char* msg, HWND hParent);
    WAP32::CGameMgr* InitializeGameManager() OVERRIDE;
    static INT_PTR __stdcall ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Boolified forward: calls the switch-down-sprite loader on `this` (thiscall,
    // reloc-masked) and normalises the int result to a bool (neg/sbb/neg).
    i32 Stub_112820();
    // The forwarded thiscall callee (retail 0x110570); no body -> reloc-masked.
    i32 LoadSwitchDownSprite();
};

#endif // GRUNTZ_GRUNTZ_GRUNTZAPP_H
