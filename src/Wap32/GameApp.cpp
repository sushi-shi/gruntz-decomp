#include <rva.h>

#include <Wap32/GameApp.h>

#include <Gruntz/GruntzCommandId.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

DATA(0x00253c6c)
i32 g_gameAppInstanceCount = 0;
DATA(0x00253c70)
i32 g_gameAppNowMs = 0;
DATA(0x00253c74)
i32 g_gameAppFrameDeltaMs = 0;
DATA(0x00253c78)
i32 g_framePacingEpochMs = 0;
DATA(0x00253c7c)
i32 g_gameAppTimerRemainingMs = 0;
DATA(0x00253c80)
i32 g_gameAppTimerPeriodMs = 0;

#define FREE_GAME_MANAGER                                                                          \
    if (m_gameMgr) {                                                                               \
        delete m_gameMgr;                                                                          \
        m_gameMgr = NULL;                                                                          \
    }

#define CLEAR_GAME_MANAGER_WINDOW                                                                  \
    m_gameWnd = NULL;                                                                              \
    m_owner = NULL

RVA(0x0013d590, 0x3c)
CGameApp::CGameApp() {
    m_gameWnd = NULL;
    m_gameMgr = NULL;
    m_hAccel = NULL;
    m_hInstance = NULL;
    m_appActive = 0;
    m_errorReported = 0;
    m_errorCode = 0;
    m_errorDetail = 0;
    g_gameAppInstanceCount++;
}

RVA(0x0013d5d0, 0x1d3)
i32 CGameApp::InitInstance(
    GameInfo* pGameInfo,
    WNDCLASSA* pWndClass,
    CREATESTRUCTA* pCreateStruct
) {
    HINSTANCE hInst;

    if (g_gameAppInstanceCount > 1) {
        goto Fail;
    }
    if (!pGameInfo || pGameInfo->size != sizeof(GameInfo)) {
        goto Fail;
    }
    if (pWndClass && (!pWndClass->lpszClassName || !*pWndClass->lpszClassName)) {
        goto Fail;
    }

    m_running = 1;
    m_errorReported = 0;
    m_errorCode = 0;
    m_errorDetail = 0;
    m_gameInfo = *pGameInfo;

    if (m_gameInfo.hInstance) {
        hInst = m_gameInfo.hInstance;
    } else if (pWndClass && pWndClass->hInstance) {
        hInst = pWndClass->hInstance;
    } else if (pCreateStruct && pCreateStruct->hInstance) {
        hInst = pCreateStruct->hInstance;
    } else {
        goto Fail;
    }
    m_hInstance = hInst;

    if (!m_gameInfo.szWindowClassName[0]) {
        sprintf(m_gameInfo.szWindowClassName, "%sClass", m_gameInfo.szGameIdentifier);
    }
    if (!m_gameInfo.szWindowName[0]) {
        sprintf(m_gameInfo.szWindowName, "%s", m_gameInfo.szGameIdentifier);
    }

    if (pWndClass) {
        m_wc = *pWndClass;
    } else {
        InitializeDefaultWindowClass();
    }

    if (pCreateStruct) {
        m_createStruct = *pCreateStruct;
    } else {
        InitializeDefaultCreateStruct();
    }

    if (!RegisterClassA(&m_wc)) {
        goto Fail;
    }

    InitializeAccelerators(m_gameInfo.szGameIdentifier);

    m_gameWnd = InitializeGameWindow();
    if (!m_gameWnd) {
        goto Fail;
    }

    if (!m_gameWnd->CreateAndShow(&m_createStruct, this)) {
        delete m_gameWnd;
        m_gameWnd = NULL;
        return 0;
    }

    m_gameMgr = InitializeGameManager();
    if (!m_gameMgr) {
        goto Fail;
    }

    if (!m_gameMgr->Run(m_gameWnd, m_gameInfo.szCmdLine)) {
        delete m_gameMgr;
        m_gameMgr = NULL;
        return 0;
    }
    return 1;

Fail:
    return 0;
}

RVA(0x0013d7b0, 0x105)
i32 CGameApp::Init(
    HINSTANCE hInstance,
    char* szWindowName,
    char* szGameIdentifier,
    char* szCmdLine,
    i32 windowClassFlags,
    i32 windowWidth,
    i32 windowHeight
) {
    GameInfo gi;

    if (!hInstance) {
        return 0;
    }

    memset(&gi, 0, sizeof(gi));
    gi.hInstance = hInstance;
    gi.size = sizeof(GameInfo);
    gi.windowClassFlags = static_cast<GameWindowFlags>(windowClassFlags);
    gi.windowWidth = windowWidth;
    gi.windowHeight = windowHeight;
    if (szWindowName) {
        strcpy(gi.szWindowName, szWindowName);
    }
    if (szGameIdentifier) {
        strcpy(gi.szGameIdentifier, szGameIdentifier);
    }
    if (szCmdLine) {
        strcpy(gi.szCmdLine, szCmdLine);
    }

    return InitInstance(&gi, NULL, NULL);
}

RVA(0x0013d8c0, 0x42)
void CGameApp::CloseResources() {
    if (m_hAccel) {
        DestroyAcceleratorTable(m_hAccel);
        m_hAccel = NULL;
    }
    FREE_GAME_MANAGER
    if (m_gameWnd) {
        delete m_gameWnd;
        m_gameWnd = NULL;
    }
}

RVA(0x0013d910, 0x9f)
i32 CGameApp::RunMessageLoop() {
    MSG msg;

    HWND hwnd = m_gameWnd->m_hwnd;
    if (!hwnd) {
        return 0;
    }

    for (;;) {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            do {
                if (msg.message == WM_QUIT) {
                    return 1;
                }
                if (m_hAccel && msg.hwnd == hwnd) {
                    TranslateAcceleratorA(hwnd, m_hAccel, &msg);
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE));
        }
        OnIdle();
    }
}

RVA(0x0013d9b0, 0xa0)
void CGameApp::InitializeDefaultWindowClass() {

    memset(&m_wc, 0, sizeof(m_wc));

    HCURSOR hCursor = LoadCursorA(m_hInstance, m_gameInfo.szGameIdentifier);
    if (HAS(m_gameInfo.windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        hCursor = LoadCursorA(NULL, IDC_ARROW);
    }

    m_wc.style = CS_DBLCLKS;
    m_wc.lpfnWndProc = GameWindowProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = m_hInstance;
    m_wc.hIcon = LoadIconA(m_hInstance, m_gameInfo.szGameIdentifier);
    m_wc.hCursor = hCursor;
    m_wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    m_wc.lpszMenuName = NULL;
    m_wc.lpszClassName = m_gameInfo.szWindowClassName;
}

// @early-stop
RVA(0x0013da50, 0x10b)
void CGameApp::InitializeDefaultCreateStruct() {

    memset(&m_createStruct, 0, sizeof(m_createStruct));

    HMENU hMenu = NULL;
    if (HAS(m_gameInfo.windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        hMenu = LoadMenuA(m_hInstance, m_gameInfo.szGameIdentifier);
    }

    i32 x, y;
    if (HAS(m_gameInfo.windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        x = COORD_UNSET;
        y = COORD_UNSET;
    } else {
        x = 0;
        y = 0;
    }

    i32 cx, cy;
    if (HAS(m_gameInfo.windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        cx = m_gameInfo.windowWidth;
        cy = m_gameInfo.windowHeight;
    } else {
        cx = GetSystemMetrics(SM_CXSCREEN);
        cy = GetSystemMetrics(SM_CYSCREEN);
    }

    i32 style;
    DWORD exStyle;
    if (HAS(m_gameInfo.windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        style = WS_OVERLAPPEDWINDOW;
        exStyle = WS_EX_APPWINDOW;
        if (HAS(m_gameInfo.windowClassFlags, GAME_WINDOW_FLAG_FIXED_SIZE)) {
            style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }
    } else {
        style = WS_POPUP | WS_SYSMENU;
        exStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    }

    m_createStruct.style = style;
    m_createStruct.hInstance = m_hInstance;
    m_createStruct.hMenu = hMenu;
    m_createStruct.y = y;
    m_createStruct.cx = cx;
    m_createStruct.lpCreateParams = NULL;
    m_createStruct.hwndParent = NULL;
    m_createStruct.x = x;
    m_createStruct.cy = cy;
    m_createStruct.lpszName = m_gameInfo.szWindowName;
    m_createStruct.lpszClass = m_gameInfo.szWindowClassName;
    m_createStruct.dwExStyle = exStyle;
}

RVA(0x0013db60, 0x57)
CGameWnd* CGameApp::InitializeGameWindow() {
    return new CGameWnd;
}

RVA(0x0013dbc0, 0x57)
CGameMgr* CGameApp::InitializeGameManager() {
    return new CGameMgr;
}

RVA(0x0013dc20, 0x49)
BOOL CGameApp::InitializeAccelerators(LPCSTR lpTable) {
    if (lpTable && *lpTable) {
        if (m_hAccel) {
            DestroyAcceleratorTable(m_hAccel);
            m_hAccel = NULL;
        }
        m_hAccel = LoadAcceleratorsA(m_hInstance, lpTable);
        return m_hAccel != NULL;
    }
    return false;
}

RVA(0x0013dc70, 0x1d)
void CGameApp::OnIdle() {
    if (m_appActive && m_running) {
        m_gameMgr->PerFrameTick();
    }
}

RVA(0x0013dc90, 0x19)
void CGameApp::FreeGameManager(){FREE_GAME_MANAGER}

RVA(0x0013dcb0, 0x57)
void CGameApp::ReportError(WPARAM wParam, LPARAM lParam) {
    if (m_errorReported != 0) {
        return;
    }
    CGameWnd* wnd = m_gameWnd;
    m_errorReported = 1;
    if (wnd != NULL && wnd->m_closeGuard == 0) {
        PostMessageA(wnd->m_hwnd, WM_CLOSE, 0, 0);
    }
    m_running = 0;
    m_errorCode = wParam;
    m_errorDetail = lParam;
}

RVA(0x0013dd10, 0x35)
CGameMgr::CGameMgr() {
    m_soundEnabled = 1;
    m_musicEnabled = 1;
    CLEAR_GAME_MANAGER_WINDOW;
    m_frameGate = 0;
    m_targetFps = 0;
    ResetFpsSampleWindow(1);
    ResetFrameTiming();
}

RVA(0x0013dd50, 0x54)
i32 CGameMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {
    if (!pGameWnd) {
        return 0;
    }
    if (!pGameWnd->m_hwnd) {
        return 0;
    }

    m_gameWnd = pGameWnd;
    m_owner = pGameWnd->m_owner;
    m_targetFps = 0;
    ResetFpsSampleWindow(1);
    ResetFrameTiming();
    g_gameAppTimerPeriodMs = GAMEAPP_PERIODIC_TIMER_MS;
    g_gameAppTimerRemainingMs = GAMEAPP_PERIODIC_TIMER_MS;
    return 1;
}

RVA(0x0013ddb0, 0x9)
void CGameMgr::Close() {
    CLEAR_GAME_MANAGER_WINDOW;
}

RVA(0x0013ddc0, 0xaa)
i32 CGameMgr::PerFrameTick() {

    DWORD(WINAPI * pTGT)(void) = timeGetTime;
    u32 now = pTGT();
    u32 delta = now - static_cast<u32>(g_gameAppNowMs);
    g_gameAppNowMs = now;
    g_gameAppFrameDeltaMs = delta;
    u32 timerRemainingMs = static_cast<u32>(g_gameAppTimerRemainingMs);
    if (timerRemainingMs == 0) {
        g_gameAppTimerRemainingMs = g_gameAppTimerPeriodMs;
    } else if (delta >= timerRemainingMs) {
        g_gameAppTimerRemainingMs = 0;
    } else {
        g_gameAppTimerRemainingMs = timerRemainingMs - delta;
    }

    if (m_targetFps > 0) {
        if (static_cast<u32>(g_framePacingEpochMs) > 0) {
            u32 elapsed = pTGT() - static_cast<u32>(g_framePacingEpochMs);
            if (elapsed < static_cast<u32>(m_frameBudgetMs)) {
                SpinWaitForMs(m_frameBudgetMs - elapsed);
            }
        }
        g_framePacingEpochMs = pTGT();
    }

    u32 count = m_fpsSampleFrameCount + 1;
    m_fpsSampleFrameCount = count;
    if (static_cast<u32>(g_gameAppNowMs) - static_cast<u32>(m_fpsSampleStartMs)
        >= GAMEAPP_FPS_SAMPLE_INTERVAL_MS) {
        m_fps = count / GAMEAPP_FPS_SAMPLE_SECONDS;
        ResetFpsSampleWindow(0);
    }
    return 1;
}

RVA(0x0013de70, 0x26)
void CGameMgr::ResetFpsSampleWindow(i32 reset) {
    m_fpsSampleFrameCount = 0;
    m_fpsSampleStartMs = timeGetTime();
    if (reset) {
        m_fps = GAMEAPP_FPS_UNAVAILABLE;
    }
}

RVA(0x0013dea0, 0x18)
void CGameMgr::ResetFrameTiming() {
    g_gameAppNowMs = timeGetTime();
    g_gameAppFrameDeltaMs = 0;
    g_framePacingEpochMs = 0;
}

RVA(0x0013dec0, 0x20)
void CGameMgr::SpinWaitForMs(i32 ms) {
    DWORD(WINAPI * fn)(void) = timeGetTime;
    u32 now = fn();
    u32 end = now + static_cast<u32>(ms);
    if (now <= end) {
        do {
            now = fn();
        } while (now <= end);
    }
}

RVA(0x0013dee0, 0x1b)
void CGameMgr::SetFrameRate(i32 fps) {
    m_targetFps = fps;
    if (fps > 0) {
        m_frameBudgetMs = MILLIS_PER_SECOND / fps;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013df00, 0x25)
i32 CGameMgr::TrySetFrameRate(i32 fps) {
    if (m_targetFps > 0) {
        SetFrameRate(0);
        return 0;
    }
    SetFrameRate(fps);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013df30, 0xaf)
void WaitKeyEdge(int vk, int timeoutMs) {
    if (timeoutMs == 0) {
        SHORT(WINAPI * gaks)(int) = GetAsyncKeyState;
        while (!(static_cast<i32>(gaks(vk)) & ASYNC_KEYSTATE_DOWN))
            ;
        while (static_cast<i32>(gaks(vk)) & ASYNC_KEYSTATE_DOWN)
            ;
    } else {
        DWORD(WINAPI * tgt)(void) = timeGetTime;
        u32 deadline = tgt() + timeoutMs;
        SHORT(WINAPI * gaks)(int) = GetAsyncKeyState;
        while (!(static_cast<i32>(gaks(vk)) & ASYNC_KEYSTATE_DOWN)) {
            if (tgt() > deadline) {
                return;
            }
        }
        while (static_cast<i32>(gaks(vk)) & ASYNC_KEYSTATE_DOWN) {
            if (tgt() > deadline) {
                return;
            }
        }
    }
}
