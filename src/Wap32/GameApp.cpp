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
i32 g_wap32Now = 0;
DATA(0x00253c74)
i32 g_wap32FrameDelta = 0;
DATA(0x00253c78)
i32 g_wap32ClockReset = 0;
DATA(0x00253c7c)
i32 g_wap32Run7c = 0;
DATA(0x00253c80)
i32 g_wap32Run80 = 0;

// @identity-TODO ?1CGameApp - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (23 fns) came from the static library. It belongs to another compiland.
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
    gi.windowClassFlags = windowClassFlags;
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

    return InitInstance(&gi, 0, 0);
}

RVA(0x0013d8c0, 0x42)
void CGameApp::CloseResources() {
    if (m_hAccel) {
        DestroyAcceleratorTable(m_hAccel);
        m_hAccel = NULL;
    }
    if (m_gameMgr) {
        delete m_gameMgr;
        m_gameMgr = NULL;
    }
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
        if (PeekMessageA(&msg, 0, 0, 0, 1)) {
            do {
                if (msg.message == WM_QUIT) {
                    return 1;
                }
                if (m_hAccel && msg.hwnd == hwnd) {
                    TranslateAcceleratorA(hwnd, m_hAccel, &msg);
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } while (PeekMessageA(&msg, 0, 0, 0, 1));
        }
        OnIdle();
    }
}

RVA(0x0013d9b0, 0xa0)
void CGameApp::InitializeDefaultWindowClass() {

    memset(&m_wc, 0, sizeof(m_wc));

    HCURSOR hCursor = LoadCursorA(m_hInstance, m_gameInfo.szGameIdentifier);
    if (m_gameInfo.windowClassFlags & 1) {
        hCursor = LoadCursorA(0, IDC_ARROW);
    }

    m_wc.style = 8;
    m_wc.lpfnWndProc = GameWindowProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = m_hInstance;
    m_wc.hIcon = LoadIconA(m_hInstance, m_gameInfo.szGameIdentifier);
    m_wc.hCursor = hCursor;
    m_wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(4));
    m_wc.lpszMenuName = NULL;
    m_wc.lpszClassName = m_gameInfo.szWindowClassName;
}

// @early-stop
RVA(0x0013da50, 0x10b)
void CGameApp::InitializeDefaultCreateStruct() {

    memset(&m_createStruct, 0, sizeof(m_createStruct));

    HMENU hMenu = 0;
    if (m_gameInfo.windowClassFlags & 1) {
        hMenu = LoadMenuA(m_hInstance, m_gameInfo.szGameIdentifier);
    }

    i32 x, y;
    if (m_gameInfo.windowClassFlags & 1) {
        x = COORD_UNSET;
        y = COORD_UNSET;
    } else {
        x = 0;
        y = 0;
    }

    i32 cx, cy;
    if (m_gameInfo.windowClassFlags & 1) {
        cx = m_gameInfo.windowWidth;
        cy = m_gameInfo.windowHeight;
    } else {
        cx = GetSystemMetrics(0);
        cy = GetSystemMetrics(1);
    }

    i32 style;
    DWORD exStyle;
    if (m_gameInfo.windowClassFlags & 1) {
        style = 0xcf0000;
        exStyle = 0x40000;
        if (m_gameInfo.windowClassFlags & 2) {
            style = 0xca0000;
        }
    } else {
        style = 0x80080000;
        exStyle = 0x40008;
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
    return 0;
}

RVA(0x0013dc70, 0x1d)
void CGameApp::OnIdle() {
    if (m_appActive && m_running) {
        m_gameMgr->PerFrameTick();
    }
}

RVA(0x0013dc90, 0x19)
void CGameApp::FreeGameManager() {
    if (m_gameMgr) {
        delete m_gameMgr;
        m_gameMgr = NULL;
    }
}

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
    m_gameWnd = NULL;
    m_owner = NULL;
    m_frameGate = 0;
    m_pacingGate = 0;
    InitTimeFields(1);
    InitializeTimeGlobal();
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
    m_pacingGate = 0;
    InitTimeFields(1);
    InitializeTimeGlobal();
    g_wap32Run80 = 0x64;
    g_wap32Run7c = 0x64;
    return 1;
}

RVA(0x0013ddb0, 0x9)
void CGameMgr::Close() {
    m_gameWnd = NULL;
    m_owner = NULL;
}

RVA(0x0013ddc0, 0xaa)
i32 CGameMgr::PerFrameTick() {

    DWORD(WINAPI * pTGT)(void) = timeGetTime;
    u32 now = pTGT();
    u32 delta = now - static_cast<u32>(g_wap32Now);
    g_wap32Now = now;
    g_wap32FrameDelta = delta;
    u32 run7c = static_cast<u32>(g_wap32Run7c);
    if (run7c == 0) {
        g_wap32Run7c = g_wap32Run80;
    } else if (delta >= run7c) {
        g_wap32Run7c = 0;
    } else {
        g_wap32Run7c = run7c - delta;
    }

    if (m_pacingGate > 0) {
        if (static_cast<u32>(g_wap32ClockReset) > 0) {
            u32 elapsed = pTGT() - static_cast<u32>(g_wap32ClockReset);
            if (elapsed < static_cast<u32>(m_frameBudgetMs)) {
                SpinWaitUntil(m_frameBudgetMs - elapsed);
            }
        }
        g_wap32ClockReset = pTGT();
    }

    u32 count = m_frameCounter + 1;
    m_frameCounter = count;
    if (static_cast<u32>(g_wap32Now) - static_cast<u32>(m_windowStartTick) >= 0x7d0) {
        m_fps = count >> 1;
        InitTimeFields(0);
    }
    return 1;
}

RVA(0x0013de70, 0x26)
void CGameMgr::InitTimeFields(i32 reset) {
    m_frameCounter = 0;
    m_windowStartTick = timeGetTime();
    if (reset) {
        m_fps = -1;
    }
}

RVA(0x0013dea0, 0x18)
void CGameMgr::InitializeTimeGlobal() {
    g_wap32Now = timeGetTime();
    g_wap32FrameDelta = 0;
    g_wap32ClockReset = 0;
}

RVA(0x0013dec0, 0x20)
void CGameMgr::SpinWaitUntil(i32 ms) {
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
    m_pacingGate = fps;
    if (fps > 0) {
        m_frameBudgetMs = 1000 / fps;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013df00, 0x25)
i32 CGameMgr::TrySetFrameRate(i32 fps) {
    if (m_pacingGate > 0) {
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
        while (!(static_cast<i32>(gaks(vk)) & 0x80000000))
            ;
        while (static_cast<i32>(gaks(vk)) & 0x80000000)
            ;
    } else {
        DWORD(WINAPI * tgt)(void) = timeGetTime;
        u32 deadline = tgt() + timeoutMs;
        SHORT(WINAPI * gaks)(int) = GetAsyncKeyState;
        while (!(static_cast<i32>(gaks(vk)) & 0x80000000)) {
            if (tgt() > deadline) {
                return;
            }
        }
        while (static_cast<i32>(gaks(vk)) & 0x80000000) {
            if (tgt() > deadline) {
                return;
            }
        }
    }
}
