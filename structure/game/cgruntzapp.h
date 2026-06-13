#ifndef GAME_CGRUNTZAPP_H
#define GAME_CGRUNTZAPP_H

/*
 * CGruntzApp — the Gruntz application object (CGameApp subclass; also a CWinApp).
 * .?AVCGruntzApp@@
 *
 * Layout PORTED FROM tomalla (refs/tomalla-gruntz/gruntz/cgruntzapp.h), attributed.
 * @address values = 1.0.1.77. Window class registered as "GruntzClass".
 */

#include "../wap32/cgameapp.h"

class CGruntzApp : public WAP32::CGameApp
{
public:
    //@vftable: 0  vector_deleting_destructor (-> @address: 004807a0)

    virtual ~CGruntzApp();

    //@todo
    virtual bool VirtualUnknownMethod03(HINSTANCE hInstance, char* szWindowName, char* szGameIdentifier, char* szCmdLine, char windowClassFlags, int windowWidth, int windowHeight);
    //@todo
    virtual void VirtualUnknownMethod11() {}
    virtual void ShowError();
    //@todo
    virtual WAP32::CGameWnd* InitializeGameWindow();
    //@todo
    virtual WAP32::CGameMgr* InitializeGameManager();

private:
    //@address: 00645df8
    static char error_dialog_message[256];
    static INT_PTR CALLBACK ErrorDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif /* GAME_CGRUNTZAPP_H */
