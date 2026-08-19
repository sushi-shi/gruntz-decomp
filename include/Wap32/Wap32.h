#ifndef WAP32_H
#define WAP32_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GruntzCommandId.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/GameApp.h>

GZ_ENUM_FORWARD(GruntzCommandId);

class CGameApp;

class CGameWnd;
extern CGameWnd* g_activeGameWnd;

class CGameWnd {
public:
    CGameWnd();

    virtual ~CGameWnd() {
        Destroy();
        g_activeGameWnd = NULL;
    }

    RVA(0x00094c40, 0x5)
    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) {
        return 0;
    }
    RVA(0x00094c60, 0x5)
    virtual i32 HandleWindowCommand(i32, i32, i32) {
        return 0;
    }

    virtual i32 OnCreate(LPARAM lParam);
    virtual i32 OnClose();
    virtual i32 OnMove(i32 x, i32 y);
    virtual i32 OnSize(WPARAM type, i32 cx, i32 cy);
    virtual i32 OnPaint();
    virtual i32 OnChar(WPARAM wParam, LPARAM lParam);
    virtual i32 OnKeyDown(WPARAM wParam, LPARAM lParam);
    RVA(0x00094c80, 0x5)
    virtual i32 OnKeyUp(WPARAM, LPARAM) {
        return 0;
    }
    virtual i32 OnSysKeyDown(WPARAM wParam, LPARAM lParam);
    virtual i32 OnActivateApp(WPARAM wParam, LPARAM lParam);

    virtual i32 QuitMessageLoop();
    RVA(0x00094ca0, 0x5)
    virtual i32 OnLButtonDown(WPARAM, i32, i32) {
        return 0;
    }
    RVA(0x00094cc0, 0x5)
    virtual i32 OnRButtonDown(WPARAM, i32, i32) {
        return 0;
    }
    RVA(0x00094ce0, 0x5)
    virtual i32 OnLButtonUp(WPARAM, i32, i32) {
        return 0;
    }
    RVA(0x00094d00, 0x5)
    virtual i32 OnRButtonUp(WPARAM, i32, i32) {
        return 0;
    }
    RVA(0x00094d20, 0x5)
    virtual i32 OnMouseMove(WPARAM, i32, i32) {
        return 0;
    }
    RVA(0x00094d40, 0x5)
    virtual i32 OnLButtonDblClk(WPARAM, i32, i32) {
        return 0;
    }
    RVA(0x00094d60, 0x5)
    virtual i32 OnRButtonDblClk(WPARAM, i32, i32) {
        return 0;
    }
    virtual i32 OnCommand(WPARAM wParam, LPARAM lParam);

    i32 CreateAndShow(CREATESTRUCTA* pParams, CGameApp* pOwner);
    void Destroy();

    void PumpMessages(u32 filterMsg, i32 count);

    void PumpMessagesRange(u32 filterMin, u32 filterMax, i32 count);

    HWND m_hwnd;
    CGameApp* m_owner;
    i32 m_closeGuard;
};

class CGameMgr;
class CGameMgr {
public:
    CGameMgr();

    virtual ~CGameMgr() {
        Close();
    }
    virtual i32 Run(CGameWnd* pGameWnd, char* szCmdLine);
    virtual void Close();
    virtual i32 IsActive();

    virtual i32 PerFrameTick();
    virtual i32 HandleCommand(i32, GruntzCommandId, i32);

    void InitTimeFields(i32 reset);
    void InitializeTimeGlobal();

    void SpinWaitUntil(i32 ms);
    void SetFrameRate(i32 fps);
    i32 TrySetFrameRate(i32 fps);

    CGameWnd* m_gameWnd;
    CGameApp* m_owner;
    i32 m_frameGate;
    i32 m_soundEnabled;
    i32 m_musicEnabled;
    i32 m_fps;

    i32 m_pacingGate;

    i32 m_frameCounter;

    i32 m_windowStartTick;
    i32 m_frameBudgetMs;
};

struct GameInfo {
    i32 size;
    i32 windowClassFlags;
    HINSTANCE hInstance;
    char szCmdLine[0x80];
    char szGameIdentifier[0x40];
    char szWindowName[0x40];
    char _pad10c[0x40];
    char szWindowClassName[0x80];
    i32 windowWidth;
    i32 windowHeight;
};

extern i32 g_gameAppInstanceCount;

class CGameApp {
public:
    CGameApp();

    virtual ~CGameApp() {
        CloseResources();
        --g_gameAppInstanceCount;
    }

    virtual i32
    InitInstance(GameInfo* pGameInfo, WNDCLASSA* pWndClass, CREATESTRUCTA* pCreateStruct);
    virtual i32 Init(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    );
    RVA(0x00080d20, 0x24)
    virtual i32 InitDefault(HINSTANCE hInstance, char* szName) {
        return Init(hInstance, szName, szName, "", 0, COORD_UNSET, COORD_UNSET);
    }
    virtual void CloseResources();

    RVA(0x00080d60, 0x18)
    virtual i32 HasWindowAndManager() {
        return m_gameWnd != NULL && m_gameMgr != NULL;
    }
    virtual i32 RunMessageLoop();
    virtual void ReportError(WPARAM wParam, LPARAM lParam);
    virtual void OnIdle();
    virtual void FreeGameManager();

    RVA(0x00080d90, 0x5)
    virtual i32 HandleCommand(i32, GruntzCommandId, i32) {
        return 0;
    }
    virtual BOOL InitializeAccelerators(LPCSTR lpTable);
    RVA(0x00080db0, 0x1)
    virtual void ShowError() {}
    virtual CGameWnd* InitializeGameWindow();
    virtual CGameMgr* InitializeGameManager();
    virtual void InitializeDefaultWindowClass();
    virtual void InitializeDefaultCreateStruct();

    static LRESULT CALLBACK GameWindowProc(HWND, UINT, WPARAM, LPARAM);

    CGameWnd* m_gameWnd;
    CGameMgr* m_gameMgr;
    HINSTANCE m_hInstance;
    HACCEL m_hAccel;
    GameInfo m_gameInfo;
    WNDCLASSA m_wc;
    CREATESTRUCTA m_createStruct;
    i32 m_appActive;
    i32 m_running;
    i32 m_errorReported;
    i32 m_errorCode;
    i32 m_errorDetail;
};
#endif // WAP32_H
