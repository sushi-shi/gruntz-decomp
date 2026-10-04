#ifndef WAP32_H
#define WAP32_H

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/GruntzCommandId.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/GameApp.h>
#include <Runtime/FrameScheduler.h>
#include <Runtime/ShutdownRequest.h>

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

    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) {
        return 0;
    }

    virtual i32 HandleWindowCommand(i32, i32, i32) {
        return 0;
    }

    virtual i32 OnCreate(LPARAM lParam);
    virtual i32 OnClose();
    virtual i32 OnMove(i32 x, i32 y);
    virtual i32 OnSize(WPARAM type, i32 cx, i32 cy);
    virtual i32 OnPaint();
    virtual i32 OnChar(WPARAM charCode, LPARAM keyData);
    virtual i32 OnKeyDown(WPARAM virtualKey, LPARAM keyData);

    virtual i32 OnKeyUp(WPARAM virtualKey, LPARAM keyData) {
        return 0;
    }
    virtual i32 OnSysKeyDown(WPARAM wParam, LPARAM lParam);
    virtual i32 OnActivateApp(WPARAM wParam, LPARAM lParam);

    virtual i32 QuitMessageLoop();

    virtual i32 OnLButtonDown(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnRButtonDown(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnLButtonUp(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnRButtonUp(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnMouseMove(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnLButtonDblClk(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnRButtonDblClk(WPARAM keyFlags, i32 x, i32 y) {
        return 0;
    }
    virtual i32 OnCommand(WPARAM wParam, LPARAM lParam);

    const HWND& GetHwnd() const {
        return m_hwnd;
    }

    i32 CreateAndShow(CREATESTRUCTA* pParams, CGameApp* pOwner);
    void Destroy();

    void PumpMessages(u32 filterMsg, i32 count);

    void PumpMessagesRange(u32 filterMin, u32 filterMax, i32 count);

    HWND m_hwnd;
    CGameApp* m_owner;
    b32 m_closeGuard;
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

    virtual i32 UpdateFrame();
    u32 AdvanceFrame(u32 nowMs);
    void SuspendFrames() { m_frames.suspend(); }
    virtual i32 HandleCommand(i32, GruntzCommandId, i32);

    void ResetFrameTiming();
    void ResetFrameTiming(u32 nowMs);
    const FrameTiming& Timing() const { return m_frames.timing(); }

    b32 ToggleFrameGate() {
        m_frameGate ^= 1;
        return m_frameGate;
    }

    void SetFrameRate(i32 fps);
    i32 TrySetFrameRate(i32 fps);

    CGameWnd* m_gameWnd;
    CGameApp* m_owner;
    b32 m_frameGate;
    b32 m_soundEnabled;
    b32 m_musicEnabled;
protected:
    FrameScheduler m_frames;
};

GZ_ENUM_FLAGS_BEGIN(GameWindowFlags, i32)
    GAME_WINDOW_FLAGS_NONE = 0,
    GAME_WINDOW_FLAG_WINDOWED = 0x1,
    GAME_WINDOW_FLAG_FIXED_SIZE = 0x2
GZ_ENUM_FLAGS_END(GameWindowFlags, i32)
GZ_ENUM_FLAGS_OPS(GameWindowFlags)

struct GameInfo {
    i32 m_size;
    GameWindowFlags m_windowClassFlags;
    HINSTANCE m_hInstance;
    char m_szCmdLine[0x80];
    char m_szGameIdentifier[0x40];
    char m_szWindowName[0x40];

    char m_pad10c[0x40];
    char m_szWindowClassName[0x80];
    i32 m_windowWidth;
    i32 m_windowHeight;
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

    virtual i32 InitDefault(HINSTANCE hInstance, char* szName) {
        return Init(
            hInstance,
            szName,
            szName,
            "",
            IDX(GAME_WINDOW_FLAGS_NONE),
            COORD_UNSET,
            COORD_UNSET
        );
    }
    virtual void CloseResources();

    virtual i32 HasWindowAndManager() {
        return m_gameWnd != NULL && m_gameMgr != NULL;
    }
    virtual i32 RunMessageLoop();
    virtual void ReportError(WPARAM wParam, LPARAM lParam);
    // Returns milliseconds until another callback is useful, or all bits set
    // when suspended until an event. Event pumping and waiting belong to the host.
    u32 Step(u32 nowMs);
    void RequestQuit(u32 delayMs) { m_shutdown.request(delayMs); }
    bool IsQuitPending() const { return m_shutdown.pending(); }
    bool TakeCloseRequest() { return m_shutdown.takeCloseRequest(); }
    virtual void FreeGameManager();

    virtual i32 HandleCommand(i32, GruntzCommandId, i32) {
        return 0;
    }
    virtual BOOL InitializeAccelerators(LPCSTR lpTable);

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
    b32 m_appActive;
    b32 m_running;
    b32 m_errorReported;
    i32 m_errorCode;
    i32 m_errorDetail;

private:
    ShutdownRequest m_shutdown;
};
#endif
