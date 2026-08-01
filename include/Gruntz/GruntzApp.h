#ifndef GRUNTZ_GRUNTZ_GRUNTZAPP_H
#define GRUNTZ_GRUNTZ_GRUNTZAPP_H
#include <rva.h>
#include <Mfc.h>
#include <Wap32/Wap32.h>

class CGruntzApp : public CGameApp {
public:
    CGruntzApp();
    virtual ~CGruntzApp() OVERRIDE;

    virtual void CloseResources() OVERRIDE;
    virtual CGameWnd* InitializeGameWindow() OVERRIDE;

    virtual i32 Init(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    ) OVERRIDE;
    virtual void ShowError() OVERRIDE;

    RVA(0x00080aa0, 0x5)
    virtual i32 HandleCommand(i32 notifyCode, GruntzCommand cmdId, i32 lParam) OVERRIDE {
        return 0;
    }

    void ShowMessage(const char* msg, HWND hParent);
    virtual CGameMgr* InitializeGameManager() OVERRIDE;
    static INT_PTR CALLBACK ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    i32 LoadSwitchDownSprite();

    RVA(0x00112820, 0xc)
    i32 TryLoadSwitchDownSprite() {
        return LoadSwitchDownSprite() != 0;
    }
};
SIZE(0x254);

extern "C" i32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, i32);

#endif // GRUNTZ_GRUNTZ_GRUNTZAPP_H
