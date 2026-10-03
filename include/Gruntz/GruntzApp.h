#ifndef GRUNTZ_GRUNTZ_GRUNTZAPP_H
#define GRUNTZ_GRUNTZ_GRUNTZAPP_H

#include <Ints.h>

#include <Gruntz/GruntzCommandId.h>
#include <Wap32/Wap32.h>

class CGruntzApp : public CGameApp {
public:
    CGruntzApp();
    virtual ~CGruntzApp()  ;

    virtual void CloseResources()  ;
    virtual CGameWnd* InitializeGameWindow()  ;

    virtual i32 Init(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    )  ;
    virtual void ShowError()  ;

    virtual i32 HandleCommand(i32 notifyCode, GruntzCommandId cmdId, i32 lParam)   {
        return 0;
    }

    void ShowMessage(const char* msg, HWND hParent);
    virtual CGameMgr* InitializeGameManager()  ;
    static BOOL CALLBACK ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

i32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, i32);

#endif
