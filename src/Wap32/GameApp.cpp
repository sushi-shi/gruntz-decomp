#include <Wap32/Wap32.h>
#include <EmptyString.h> // g_emptyString
#include <rva.h>
#include <string.h>
#include <stdio.h>
#include <Wap32/GameApp.h> // own exported globals (ex Globals.h)

// CGameApp::~CGameApp @0x080cf0 - the STANDALONE out-of-line copy of the (inline,
// header-defined) base dtor, referenced only by a /GX EH-unwind funclet (base-subobject
// cleanup when a CGameApp-derived ctor's member throws). ??_GCGameApp above and
// CGruntzApp's cross-TU dtor keep folding their own inline copies; this forcer (the
// UserLogicCtorEmit #pragma inline_depth(0) pattern) emits the out-of-line COMDAT in
// this unit so the unwind reference resolves and the RVA is matched.
RVA_COMPGEN(0x00080cf0, 0x12, ??1CGameApp@@UAE@XZ)

RVA(0x00080d20, 0x24)
i32 CGameApp::InitDefault(HINSTANCE hInstance, char* szName) {
    return Init(hInstance, szName, szName, g_emptyString, 0, static_cast<i32>(0x80000000), static_cast<i32>(0x80000000));
}

RVA(0x00080d60, 0x18)
i32 CGameApp::HasWindowAndManager() {
    return m_gameWnd != 0 && m_gameMgr != 0;
}

RVA(0x00080d90, 0x5)
i32 CGameApp::HandleCommand(i32, GruntzCommand, i32) {
    return 0;
}

// CGameApp::scalar-dtor @0x080dd0 - the CGameApp scalar-deleting destructor (the
// ??_GCGameApp thunk with ~CGameApp inlined). Real polymorphic: the explicit
// qualified this->CGameApp::~CGameApp() inlines the (inline, virtual) dtor, whose
// auto vptr-restore stamps ??_7CGameApp@@6B@ (0x5e9b0c) and whose body runs
// CloseResources() + the instance-counter decrement - so the manual vtable stamp
// (and the retail-vtable / instance-counter aliases) are gone.
RVA_COMPGEN(0x00080dd0, 0x32, ??_GCGameApp@@UAEPAXI@Z) // (cl-auto-gen scalar-deleting dtor)

RVA(0x0013d590, 0x3c)
CGameApp::CGameApp() {
    m_gameWnd = 0;
    m_gameMgr = 0;
    m_hAccel = 0; // the optimiser schedules the +0x10 store before +0x0c
    m_hInstance = 0;
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
    if (!pGameInfo || pGameInfo->size != 0x1d4) {
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

    hInst = m_gameInfo.hInstance;
    if (!hInst && (!pWndClass || !(hInst = pWndClass->hInstance))
        && (!pCreateStruct || !(hInst = pCreateStruct->hInstance))) {
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
        m_gameWnd = 0;
        return 0;
    }

    m_gameMgr = InitializeGameManager();
    if (!m_gameMgr) {
        goto Fail;
    }

    if (!m_gameMgr->Run(m_gameWnd, m_gameInfo.szCmdLine)) {
        delete m_gameMgr;
        m_gameMgr = 0;
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
        m_hAccel = 0;
    }
    if (m_gameMgr) {
        delete m_gameMgr;
        m_gameMgr = 0;
    }
    if (m_gameWnd) {
        delete m_gameWnd;
        m_gameWnd = 0;
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
                if (msg.message == 0x12 /*WM_QUIT*/) {
                    return 1;
                }
                if (m_hAccel && msg.hwnd == hwnd) {
                    TranslateAcceleratorA(hwnd, m_hAccel, &msg);
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } while (PeekMessageA(&msg, 0, 0, 0, 1));
        }
        OnIdle(); // idle virtual (vtbl +0x20)
    }
}

RVA(0x0013d9b0, 0xa0)
void CGameApp::InitializeDefaultWindowClass() {
    i32 i;
    for (i = 0; i < 10; i++) {
        (reinterpret_cast<i32*>(&m_wc))[i] = 0;
    }

    HCURSOR hCursor = LoadCursorA(m_hInstance, m_gameInfo.szGameIdentifier);
    if (m_gameInfo.windowClassFlags & 1) {
        hCursor = LoadCursorA(0, reinterpret_cast<LPCSTR>(0x7f00) /*IDC_ARROW*/);
    }

    m_wc.style = 8; // CS_DBLCLKS
    m_wc.lpfnWndProc = GameWindowProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = m_hInstance;
    m_wc.hIcon = LoadIconA(m_hInstance, m_gameInfo.szGameIdentifier);
    m_wc.hCursor = hCursor;
    m_wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(4));
    m_wc.lpszMenuName = 0;
    m_wc.lpszClassName = m_gameInfo.szWindowClassName;
}

RVA(0x0013da50, 0x10b)
void CGameApp::InitializeDefaultCreateStruct() {
    i32 i;
    for (i = 0; i < 12; i++) {
        (reinterpret_cast<i32*>(&m_createStruct))[i] = 0;
    }

    HMENU hMenu = 0;
    if (m_gameInfo.windowClassFlags & 1) {
        hMenu = LoadMenuA(m_hInstance, m_gameInfo.szGameIdentifier);
    }

    // x and y both == CW_USEDEFAULT (windowed) or 0 (fullscreen). Kept as two
    // separate variables (the target materializes x in a register, y in a stack
    // slot, both assigned in one branch - a single var folds to a branchless
    // neg/sbb/and on the 0x80000000 mask).
    i32 x, y;
    if (m_gameInfo.windowClassFlags & 1) {
        x = static_cast<i32>(0x80000000);
        y = static_cast<i32>(0x80000000);
    } else {
        x = 0;
        y = 0;
    }

    // Width/height: the requested size when windowed, the screen otherwise.
    i32 cx, cy;
    if (m_gameInfo.windowClassFlags & 1) {
        cx = m_gameInfo.windowWidth;
        cy = m_gameInfo.windowHeight;
    } else {
        cx = GetSystemMetrics(0); // SM_CXSCREEN
        cy = GetSystemMetrics(1); // SM_CYSCREEN
    }

    i32 style;
    DWORD exStyle;
    if (m_gameInfo.windowClassFlags & 1) {
        style = 0xcf0000; // WS_OVERLAPPEDWINDOW (default)
        exStyle = 0x40000;
        if (m_gameInfo.windowClassFlags & 2) {
            style = 0xca0000; // DialogFrame: caption + sysmenu
        }
    } else {
        style = 0x80080000; // WS_POPUP | ...
        exStyle = 0x40008;
    }

    m_createStruct.style = style;
    m_createStruct.hInstance = m_hInstance;
    m_createStruct.hMenu = hMenu;
    m_createStruct.y = y;
    m_createStruct.cx = cx;
    m_createStruct.lpCreateParams = 0;
    m_createStruct.hwndParent = 0;
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
            m_hAccel = 0;
        }
        m_hAccel = LoadAcceleratorsA(m_hInstance, lpTable);
        return m_hAccel != 0;
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
        m_gameMgr = 0;
    }
}

RVA(0x0013dcb0, 0x57)
void CGameApp::ReportError(WPARAM wParam, LPARAM lParam) {
    if (m_errorReported != 0) {
        return;
    }
    CGameWnd* wnd = m_gameWnd;
    m_errorReported = 1;
    if (wnd != 0 && wnd->m_closeGuard == 0) {
        ::PostMessageA(wnd->m_hwnd, 0x10, 0, 0); // WM_CLOSE
    }
    m_running = 0;
    m_errorCode = wParam;
    m_errorDetail = lParam;
}

RVA(0x0013dd10, 0x35)
CGameMgr::CGameMgr() {
    m_soundEnabled = 1;
    m_musicEnabled = 1;
    m_gameWnd = 0;
    m_owner = 0;
    m_frameGate = 0;
    m_pacingGate = 0;
    InitTimeFields(1);
    InitializeTimeGlobal();
}

VTBL(CGameMgr, 0x001e9b8c); // ??_7CGameMgr@@6B@ (RTTI-real, global-ns)
DATA(0x00253c6c)
i32 g_gameAppInstanceCount = 0; // CGameApp instance counter
DATA(0x00253c70)
i32 g_wap32Now = 0; // frame clock (timeGetTime latch)
DATA(0x00253c74)
i32 g_wap32FrameDelta = 0; // frame delta
DATA(0x00253c78)
i32 g_wap32ClockReset = 0; // 0x653c78
DATA(0x00253c7c)
i32 g_wap32Run7c = 0; // 0x653c7c  run-state countdown
DATA(0x00253c80)
i32 g_wap32Run80 = 0; // 0x653c80  run-state reload value

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
    m_gameWnd = 0;
    m_owner = 0;
}

RVA(0x0013ddc0, 0xaa)
i32 CGameMgr::PerFrameTick() {
    // Cache the fnptr in a local so cl loads it once (mov edi,[_g_pTimeGetTime]) and
    // reuses it across the three samples (call edi), exactly as retail does.
    DWORD(WINAPI * pTGT)(void) = ::timeGetTime;
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
        InitTimeFields(0); // 0x13de70 (defined below; direct call rel32)
    }
    return 1;
}

RVA(0x0013de70, 0x23)
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

// -------------------------------------------------------------------------
// CGameMgr::SpinWaitUntil(ms) (0x13dec0; ex RezMgr::) - the ms frame-pacing
// busy-wait PerFrameTick calls: sample timeGetTime through the game-owned fn-ptr
// and spin until `now` passes `start + ms` (unsigned, overflow-guarded). `this`
// is unused (ecx ignored); the fn-ptr is cached in a callee-save.
// @early-stop
// ~83.9% regalloc wall: body byte-exact, but retail pins the cached fn-ptr in edi
// and the deadline in esi (pushing both callee-saves upfront), while MSVC5 swaps
// them (fn-ptr in esi, deadline in edi, edi shrink-wrapped). No source spelling
// flips the esi/edi pair; logic complete.
RVA(0x0013dec0, 0x20)
void CGameMgr::SpinWaitUntil(i32 ms) {
    DWORD(WINAPI * fn)(void) = ::timeGetTime;
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

RVA(0x0013df00, 0x25)
i32 CGameMgr::TrySetFrameRate(i32 fps) {
    if (m_pacingGate > 0) {
        SetFrameRate(0);
        return 0;
    }
    SetFrameRate(fps);
    return 1;
}

// ---------------------------------------------------------------------------
// WaitKeyEdge (0x13df30; moved from RezMgr.cpp in wave4-K) - busy-wait for a
// key down-then-up edge on virtual-key `vk`, with an optional `timeoutMs` deadline
// (through the game-owned timeGetTime fn-ptr ::timeGetTime, above). __cdecl, two
// stack args. Reads the OS key state through the engine's cached GetAsyncKeyState
// fn-ptr (::GetAsyncKeyState @0x6c4500).
// ORPHAN: no .text caller (a free __cdecl busy-wait); no owning class.
// @early-stop
// regalloc-swap wall (~97%): byte-identical except retail pins `vk` in esi and the
// cached GetAsyncKeyState ptr in edi, while our /O2 picks the reverse (ptr->esi,
// vk->edi). Only the modrm reg fields differ; tried direct global calls (77%, no
// caching) and an `int k = vk` copy (no change). Pure register assignment.
RVA(0x0013df30, 0xaf)
void WaitKeyEdge(int vk, int timeoutMs) {
    if (timeoutMs == 0) {
        SHORT(WINAPI * gaks)(int) = ::GetAsyncKeyState;
        while (!(static_cast<i32>(gaks(vk)) & 0x80000000))
            ;
        while (static_cast<i32>(gaks(vk)) & 0x80000000)
            ;
    } else {
        DWORD(WINAPI * tgt)(void) = ::timeGetTime;
        u32 deadline = tgt() + timeoutMs;
        SHORT(WINAPI * gaks)(int) = ::GetAsyncKeyState;
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


static CGameApp* volatile g_forceEmitCGameApp;
#pragma inline_depth(0)
void ForceEmitCGameAppDtor() {
    g_forceEmitCGameApp->CGameApp::~CGameApp();
}
#pragma inline_depth()


VTBL(CGameApp, 0x001e9b0c);
