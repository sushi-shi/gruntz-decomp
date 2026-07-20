#ifndef GRUNTZ_GRUNTZ_GRUNTZAPP_H
#define GRUNTZ_GRUNTZ_GRUNTZAPP_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
#include <Mfc.h>
#include <Wap32/Wap32.h>

SIZE(CGruntzApp, 0x254);
VTBL(CGruntzApp, 0x001e9ab4); // vtable_names -> code (RTTI game class)
class CGruntzApp : public CGameApp {
public:
    CGruntzApp();                                      // ctor
    virtual ~CGruntzApp() OVERRIDE;                    // vtbl +0x00
    virtual void CloseResources() OVERRIDE;            // slot 4 (declared-only)
    virtual CGameWnd* InitializeGameWindow() OVERRIDE; // slot 13 (0x0809a0, GruntzApp.cpp)
    // Override of the base init virtual (CGameApp slot +0x08): forwards all 7
    // launch args to CGameApp::Init and normalises the result
    // to a bool. WinMain calls this as the app "Init".
    virtual i32 Init(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    ) OVERRIDE;                        // vtbl +0x08
    virtual void ShowError() OVERRIDE; // vtbl +0x30
    // WM_COMMAND handler override; this app claims nothing here - just returns 0.
    RVA(0x00080aa0, 0x5)
    virtual i32 HandleCommand(i32 notifyCode, GruntzCommand cmdId, i32 lParam) OVERRIDE {
        return 0;
    }
    // Shows the MESSAGE dialog with an arbitrary message string.
    void ShowMessage(char* msg, HWND hParent);
    virtual CGameMgr* InitializeGameManager() OVERRIDE;
    static INT_PTR CALLBACK ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Boolified forward: calls the switch-down-sprite loader on `this` (thiscall,
    // reloc-masked) and normalises the int result to a bool (neg/sbb/neg).
    i32 LoadSwitchDownSprite();
    // The forwarded thiscall callee (retail 0x110570); no body -> reloc-masked.
    RVA(0x00112820, 0xc)
    i32 TryLoadSwitchDownSprite() {
        return LoadSwitchDownSprite() != 0;
    }
};

#endif // GRUNTZ_GRUNTZ_GRUNTZAPP_H
