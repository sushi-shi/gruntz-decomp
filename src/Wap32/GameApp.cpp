#include <StdAfx.h>

#include <Ints.h>

#include <Wap32/GameApp.h>

#include <Gruntz/GruntzCommandId.h>
#include <SafeDelete.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

i32 g_gameAppInstanceCount = 0;

CGameApp::CGameApp() {
    m_gameWnd = NULL;
    m_gameMgr = NULL;
    m_hAccel = NULL;
    m_hInstance = NULL;
    m_appActive = false;
    m_errorReported = false;
    m_errorCode = 0;
    m_errorDetail = 0;
    g_gameAppInstanceCount++;
}

i32 CGameApp::InitInstance(
    GameInfo* pGameInfo,
    WNDCLASSA* pWndClass,
    CREATESTRUCTA* pCreateStruct
) {
    HINSTANCE hInst;

    if (g_gameAppInstanceCount > 1) {
        goto Fail;
    }
    if (!pGameInfo || pGameInfo->m_size != sizeof(GameInfo)) {
        goto Fail;
    }
    if (pWndClass && (!pWndClass->lpszClassName || !*pWndClass->lpszClassName)) {
        goto Fail;
    }

    m_running = true;
    m_errorReported = false;
    m_errorCode = 0;
    m_errorDetail = 0;
    m_gameInfo = *pGameInfo;

    if (m_gameInfo.m_hInstance) {
        hInst = m_gameInfo.m_hInstance;
    } else if (pWndClass && pWndClass->hInstance) {
        hInst = pWndClass->hInstance;
    } else if (pCreateStruct && pCreateStruct->hInstance) {
        hInst = pCreateStruct->hInstance;
    } else {
        goto Fail;
    }
    m_hInstance = hInst;

    if (!m_gameInfo.m_szWindowClassName[0]) {
        sprintf(m_gameInfo.m_szWindowClassName, "%sClass", m_gameInfo.m_szGameIdentifier);
    }
    if (!m_gameInfo.m_szWindowName[0]) {
        sprintf(m_gameInfo.m_szWindowName, "%s", m_gameInfo.m_szGameIdentifier);
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

    InitializeAccelerators(m_gameInfo.m_szGameIdentifier);

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

    if (!m_gameMgr->Run(m_gameWnd, m_gameInfo.m_szCmdLine)) {
        delete m_gameMgr;
        m_gameMgr = NULL;
        return 0;
    }
    return 1;

Fail:
    return 0;
}

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
    gi.m_hInstance = hInstance;
    gi.m_size = sizeof(GameInfo);
    gi.m_windowClassFlags = static_cast<GameWindowFlags>(windowClassFlags);
    gi.m_windowWidth = windowWidth;
    gi.m_windowHeight = windowHeight;
    if (szWindowName) {
        strcpy(gi.m_szWindowName, szWindowName);
    }
    if (szGameIdentifier) {
        strcpy(gi.m_szGameIdentifier, szGameIdentifier);
    }
    if (szCmdLine) {
        strcpy(gi.m_szCmdLine, szCmdLine);
    }

    return InitInstance(&gi, NULL, NULL);
}

void CGameApp::CloseResources() {
    if (m_hAccel) {
        DestroyAcceleratorTable(m_hAccel);
        m_hAccel = NULL;
    }
    FREE_GAME_MANAGER
    SAFE_DELETE(m_gameWnd);
}

i32 CGameApp::RunMessageLoop() {
    HWND hwnd = m_gameWnd->GetHwnd();
    if (!hwnd) return 0;

    for (;;) {
        MSG msg;
        unsigned int dispatched = 0;
        while (dispatched < 64 && PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 1;
            ++dispatched;
            if (m_hAccel && msg.hwnd == hwnd
                && TranslateAcceleratorA(hwnd, m_hAccel, &msg)) {
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        const u32 delay = Step(timeGetTime());
        // A capped batch may leave previously observed input in the queue.
        // Drain it on the next iteration instead of waiting for new input.
        if (delay && dispatched < 64) {
            MsgWaitForMultipleObjects(0, NULL, FALSE, delay, QS_ALLINPUT);
        }
    }
}

void CGameApp::InitializeDefaultWindowClass() {

    memset(&m_wc, 0, sizeof(m_wc));

    HCURSOR hCursor = LoadCursorA(m_hInstance, m_gameInfo.m_szGameIdentifier);
    if (HAS(m_gameInfo.m_windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        hCursor = LoadCursorA(NULL, IDC_ARROW);
    }

    m_wc.style = CS_DBLCLKS;
    m_wc.lpfnWndProc = GameWindowProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = m_hInstance;
    m_wc.hIcon = LoadIconA(m_hInstance, m_gameInfo.m_szGameIdentifier);
    m_wc.hCursor = hCursor;
    m_wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    m_wc.lpszMenuName = NULL;
    m_wc.lpszClassName = m_gameInfo.m_szWindowClassName;
}

void CGameApp::InitializeDefaultCreateStruct() {

    memset(&m_createStruct, 0, sizeof(m_createStruct));

    HMENU hMenu = NULL;
    if (HAS(m_gameInfo.m_windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        hMenu = LoadMenuA(m_hInstance, m_gameInfo.m_szGameIdentifier);
    }

    i32 x, y;
    if (HAS(m_gameInfo.m_windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        x = COORD_UNSET;
        y = COORD_UNSET;
    } else {
        x = 0;
        y = 0;
    }

    i32 cx, cy;
    if (HAS(m_gameInfo.m_windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        cx = m_gameInfo.m_windowWidth;
        cy = m_gameInfo.m_windowHeight;
    } else {
        cx = GetSystemMetrics(SM_CXSCREEN);
        cy = GetSystemMetrics(SM_CYSCREEN);
    }

    i32 style;
    DWORD exStyle;
    if (HAS(m_gameInfo.m_windowClassFlags, GAME_WINDOW_FLAG_WINDOWED)) {
        style = WS_OVERLAPPEDWINDOW;
        exStyle = WS_EX_APPWINDOW;
        if (HAS(m_gameInfo.m_windowClassFlags, GAME_WINDOW_FLAG_FIXED_SIZE)) {
            style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }
    } else {
        style = WS_POPUP | WS_SYSMENU;
        exStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    }

    m_createStruct.lpCreateParams = NULL;
    m_createStruct.hInstance = m_hInstance;
    m_createStruct.hMenu = hMenu;
    m_createStruct.hwndParent = NULL;
    m_createStruct.x = x;
    m_createStruct.y = y;
    m_createStruct.cx = cx;
    m_createStruct.cy = cy;
    m_createStruct.style = style;
    m_createStruct.lpszName = m_gameInfo.m_szWindowName;
    m_createStruct.lpszClass = m_gameInfo.m_szWindowClassName;
    m_createStruct.dwExStyle = exStyle;
}

CGameWnd* CGameApp::InitializeGameWindow() {
    return new CGameWnd;
}

CGameMgr* CGameApp::InitializeGameManager() {
    return new CGameMgr;
}

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

u32 CGameApp::Step(u32 nowMs) {
    if (m_appActive && m_running && m_gameMgr) {
        return m_gameMgr->AdvanceFrame(nowMs);
    }
    if (m_gameMgr) m_gameMgr->SuspendFrames();
    return 0xffffffffU;
}

void CGameApp::FreeGameManager(){FREE_GAME_MANAGER}

void CGameApp::ReportError(WPARAM wParam, LPARAM lParam) {
    if (m_errorReported != false) {
        return;
    }
    CGameWnd* wnd = m_gameWnd;
    m_errorReported = true;
    if (wnd != NULL && wnd->m_closeGuard == false) {
        PostMessageA(wnd->GetHwnd(), WM_CLOSE, 0, 0);
    }
    m_running = false;
    m_errorCode = wParam;
    m_errorDetail = lParam;
}

CGameMgr::CGameMgr() {
    m_soundEnabled = true;
    m_musicEnabled = true;
    CLEAR_GAME_MANAGER_WINDOW;
    m_frameGate = false;
    m_frames.reset(timeGetTime());
}

i32 CGameMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {
    if (!pGameWnd) {
        return 0;
    }
    if (!pGameWnd->GetHwnd()) {
        return 0;
    }

    m_gameWnd = pGameWnd;
    m_owner = pGameWnd->m_owner;
    m_frames.reset(timeGetTime());
    return 1;
}

void CGameMgr::Close() {
    CLEAR_GAME_MANAGER_WINDOW;
}

u32 CGameMgr::AdvanceFrame(u32 nowMs) {
    if (!m_frames.poll(nowMs)) return m_frames.delayMs(nowMs);
    UpdateFrame();
    return 0;
}

i32 CGameMgr::UpdateFrame() {
    return 1;
}

void CGameMgr::ResetFrameTiming() {
    ResetFrameTiming(timeGetTime());
}

void CGameMgr::ResetFrameTiming(u32 nowMs) {
    m_frames.resetFrameTime(nowMs);
}

void CGameMgr::SetFrameRate(i32 fps) {
    m_frames.timing().setFrameRate(fps);
}

i32 CGameMgr::TrySetFrameRate(i32 fps) {
    if (Timing().targetFps() > 0) {
        SetFrameRate(0);
        return 0;
    }
    SetFrameRate(fps);
    return 1;
}

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
