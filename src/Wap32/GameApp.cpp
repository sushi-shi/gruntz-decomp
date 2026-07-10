// GameApp.cpp - WAP32 CGameApp (Brian Goble's engine).
// Matched: CGameApp::CGameApp (byte-exact code; the instance
// counter is a file-scope global here - same store sequence, the
// reloc just names a different symbol than the Ghidra DAT_ at that address).
#include <Wap32/Wap32.h>
#include <rva.h>
#include <string.h>
#include <stdio.h>
#include <Globals.h>
// timeGetTime (WINMM frame clock) comes from <Mfc.h>'s central decl (via <Wap32.h>).

// Two run-state timing defaults CGameMgr::Run seeds to 0x64 (100).

// Instance counter (bumped per ctor). Shared
// (declared in Wap32.h) so the inline ~CGameApp - which CGruntzApp's dtor
// inlines in another TU - resolves it; the reloc name is masked in objdiff.
i32 g_gameAppInstanceCount;

// -------------------------------------------------------------------------
// CGameApp::CGameApp()
// Zeroes the resource/window/manager pointers and the error-state fields,
// then bumps the file-scope instance counter.
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

// -------------------------------------------------------------------------
// CGameApp::CloseResources
// Frees the accelerator table then deletes the two resource objects.
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

// -------------------------------------------------------------------------
// CGameApp::InitializeAccelerators
// Reloads the accelerator table; returns whether it loaded.
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

// -------------------------------------------------------------------------
// CGameApp::ReportError
// Records an error once (guarded by m_errorReported), posting WM_CLOSE to the window.
RVA(0x0013dcb0, 0x57)
void CGameApp::ReportError(WPARAM wParam, LPARAM lParam) {
    if (m_errorReported) {
        return;
    }
    m_errorReported = 1;
    if (m_gameWnd && m_gameWnd->m_closeGuard == 0) {
        PostMessageA(m_gameWnd->m_hwnd, 0x10 /*WM_CLOSE*/, 0, 0);
    }
    m_running = 0;
    m_errorCode = (i32)wParam;
    m_errorDetail = (i32)lParam;
}

// -------------------------------------------------------------------------
// CGameApp::RunMessageLoop - the main Win32 pump (vtbl slot +0x18; WinMain
// dispatches here). Reads the OS HWND off m_gameWnd->m_hwnd; if there is no
// window, return 0. Otherwise the classic peek/process/idle pump: PeekMessageA
// (PM_REMOVE) drains all pending messages (WM_QUIT exits with 1); when m_hAccel
// (HACCEL) is set AND the message targets our window, run TranslateAcceleratorA
// (return ignored); always TranslateMessage + DispatchMessageA; when the queue
// is empty, call the idle virtual (vtbl +0x20) and loop.
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

// -------------------------------------------------------------------------
// CGameApp::InitializeDefaultWindowClass
// Fills the embedded WNDCLASSA (m_wc @ +0x1e8) and loads its icon/cursor.
RVA(0x0013d9b0, 0xa0)
void CGameApp::InitializeDefaultWindowClass() {
    i32 i;
    for (i = 0; i < 10; i++) {
        ((i32*)&m_wc)[i] = 0;
    }

    HCURSOR hCursor = LoadCursorA(m_hInstance, m_gameInfo.szGameIdentifier);
    if (m_gameInfo.windowClassFlags & 1) {
        hCursor = LoadCursorA(0, (LPCSTR)0x7f00 /*IDC_ARROW*/);
    }

    m_wc.style = 8; // CS_DBLCLKS
    m_wc.lpfnWndProc = GameWindowProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = m_hInstance;
    m_wc.hIcon = LoadIconA(m_hInstance, m_gameInfo.szGameIdentifier);
    m_wc.hCursor = hCursor;
    m_wc.hbrBackground = (HBRUSH)GetStockObject(4);
    m_wc.lpszMenuName = 0;
    m_wc.lpszClassName = m_gameInfo.szWindowClassName;
}

// -------------------------------------------------------------------------
// CGameApp::InitializeGameWindow
// `return new CGameWnd;` - operator new(0x10) then the CGameWnd ctor under a
// C++ EH frame (so this TU is built with /GX). The push-ecx at entry is MSVC
// reserving one dword of locals for the new pointer / EH-tracked object;
// `this` (the CGameApp) is never touched - this is the CGameWnd-allocation
// analog of CGruntzApp::InitializeGameManager, and it sits in the CGameApp
// address cluster, so it belongs to CGameApp (not CGruntzApp; uses no
// game-app-specific >=0x254 fields).
RVA(0x0013db60, 0x57)
CGameWnd* CGameApp::InitializeGameWindow() {
    return new CGameWnd;
}

// -------------------------------------------------------------------------
// CGameApp::InitializeGameManager (vtbl +0x38) - the base engine's manager
// factory: `return new WAP32::CGameMgr;` (operator new(0x2c) then the CGameMgr
// ctor at 0x13dd10, under the C++ EH frame). The CGameMgr-allocation analog of
// InitializeGameWindow; CGruntzApp overrides it (new CGruntzMgr) in GruntzApp.cpp.
RVA(0x0013dbc0, 0x57)
WAP32::CGameMgr* CGameApp::InitializeGameManager() {
    return new WAP32::CGameMgr;
}

// -------------------------------------------------------------------------
// CGameApp::Init
// Builds a GameInfo descriptor on the stack from the launch parameters, then
// hands it to InitInstance (vtable +0x4) to register+create.
// hInstance is required (null -> 0). The three name strings are conditionally
// strcpy'd (inline rep movs at /O2/Oi).
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

// -------------------------------------------------------------------------
// CGameApp::InitDefault (vtbl +0x0c) - the one-name convenience overload: forward
// to the virtual Init using `szName` for BOTH the window name and the game
// identifier, an empty command line, no flags, and default (CW_USEDEFAULT) size.
extern "C" char g_emptyString[]; // 0x6293f4 (empty command-line default)

RVA(0x00080d20, 0x24)
i32 CGameApp::InitDefault(HINSTANCE hInstance, char* szName) {
    return Init(hInstance, szName, szName, g_emptyString, 0, (i32)0x80000000, (i32)0x80000000);
}

// -------------------------------------------------------------------------
// CGameApp::InitializeDefaultCreateStruct
// Fills the embedded CREATESTRUCTA (m_createStruct @ +0x210) with default
// window geometry/style derived from the GameInfo windowClassFlags:
//   bit1 (Windowed) -> a "Gruntz" menu, gameInfo width/height, overlapped or
//   caption style; otherwise -> fullscreen popup at the screen metrics.
RVA(0x0013da50, 0x10b)
void CGameApp::InitializeDefaultCreateStruct() {
    i32 i;
    for (i = 0; i < 12; i++) {
        ((i32*)&m_createStruct)[i] = 0;
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
        x = (i32)0x80000000;
        y = (i32)0x80000000;
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

// -------------------------------------------------------------------------
// CGameApp::InitInstance
// The Run/Init orchestration: validate the GameInfo, copy it into the member,
// resolve hInstance, build the class+window names, register the class, create
// the window via CGameWnd::CreateAndShow, then bring up the game manager.
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

// -------------------------------------------------------------------------
// CGameApp::OnIdle - vtbl slot +0x20.
// The per-frame idle virtual the message pump calls on an empty queue
// (RunMessageLoop dispatches `call [vtbl+0x20]`). When the app is active -
// both gate words m_appActive and m_running set - it tail-calls the game manager's
// per-frame tick (m_8->vtbl +0x10, the 5th vtable slot). The tail call emits
// `mov ecx,[m_8]; mov eax,[ecx]; jmp [eax+0x10]` (no own epilogue needed since
// neither gate-load disturbs a callee-saved reg).
RVA(0x0013dc70, 0x1d)
void CGameApp::OnIdle() {
    if (m_appActive && m_running) {
        m_gameMgr->PerFrameTick();
    }
}

// -------------------------------------------------------------------------
// CGameApp::FreeGameManager - vtbl slot +0x24.
// `delete m_8; m_8 = 0;` - frees the game manager via its scalar-deleting
// dtor (slot 0, `push 1; call [vtbl]`) and clears the slot. `this` is spilled
// to esi at entry; the null-check skips both when m_8 is already 0.
RVA(0x0013dc90, 0x19)
void CGameApp::FreeGameManager() {
    if (m_gameMgr) {
        delete m_gameMgr;
        m_gameMgr = 0;
    }
}

// ~CGameApp is now inline in Wap32.h (CloseResources() + counter dec); it is
// still emitted in this TU (the vtable's scalar-deleting dtor references it).

// CGameMgr vtable anchors (~CGameMgr is now inline in Wap32.h; the three
// otherwise-unmatched virtuals anchor here).
i32 WAP32::CGameMgr::Wap32GameMgrVfunc3() {
    return 0;
}
void WAP32::CGameMgr::PerFrameTick() {}
i32 WAP32::CGameMgr::HandleCommand(i32, i32, i32) {
    return 0;
}

// -------------------------------------------------------------------------
// CGameMgr::CGameMgr()  (__thiscall, returns this in EAX; vftable @0x5e9b8c)
// Seeds the sound/music-on flags (m_soundEnabled/m_musicEnabled = 1), zeroes the owned pointers, then
// initialises the frame clock via the two time helpers. The optimiser hoists
// the m_soundEnabled/m_musicEnabled stores (and the InitTimeFields `reset=1` argument push) above
// the vptr store.
RVA(0x0013dd10, 0x35)
WAP32::CGameMgr::CGameMgr() {
    m_soundEnabled = 1;
    m_musicEnabled = 1;
    m_gameWnd = 0;
    m_owner = 0;
    m_frameGate = 0;
    m_pauseFlag = 0;
    InitTimeFields(1);
    InitializeTimeGlobal();
}

// -------------------------------------------------------------------------
// CGameMgr::Run  (__thiscall; vtable +0x04, the engine-start entry).
// Binds the manager to the game window (records the window + its owner app),
// reseeds the frame clock, and primes the two run-timing globals to 100.
// Bails (returning 0) if there is no window, or the window has no OS HWND yet.
RVA(0x0013dd50, 0x54)
i32 WAP32::CGameMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {
    if (!pGameWnd) {
        return 0;
    }
    if (!pGameWnd->m_hwnd) {
        return 0;
    }

    m_gameWnd = pGameWnd;
    m_owner = pGameWnd->m_owner;
    m_pauseFlag = 0;
    InitTimeFields(1);
    InitializeTimeGlobal();
    g_wap32Run80 = 0x64;
    g_wap32Run7c = 0x64;
    return 1;
}

// -------------------------------------------------------------------------
// CGameMgr::Close  (vtable +0x08)
// Clears the two manager-owned pointers/handles.
RVA(0x0013ddb0, 0x9)
void WAP32::CGameMgr::Close() {
    m_gameWnd = 0;
    m_owner = 0;
}

// -------------------------------------------------------------------------
// CGameMgr::InitTimeFields  (__thiscall; ctor/Run helper @0x13de70)
// Zeroes m_elapsedMs, samples the start tick into m_startTick, and (when reset) arms m_fps.
RVA(0x0013de70, 0x23)
void WAP32::CGameMgr::InitTimeFields(i32 reset) {
    m_elapsedMs = 0;
    m_startTick = timeGetTime();
    if (reset) {
        m_fps = -1;
    }
}

// -------------------------------------------------------------------------
// CGameMgr::InitializeTimeGlobal
// Seeds the frame clock from timeGetTime and clears its deltas.
RVA(0x0013dea0, 0x18)
void WAP32::CGameMgr::InitializeTimeGlobal() {
    g_wap32Now = timeGetTime();
    g_wap32FrameDelta = 0;
    g_wap32ClockReset = 0;
}

// CGameApp::GameWindowProc (the static WNDPROC) is reconstructed in GameWnd.cpp:
// its code lives in the CGameWnd address cluster and dispatches to the active
// CGameWnd singleton (s_activeWnd). This TU only references it (the WNDCLASS
// store in InitializeDefaultWindowClass).

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// 0x133380 stores vftable 0x5ef670 and calls dtor body FUN_00534d50 - the
// scalar-deleting destructor of some OTHER engine class the delinker labelled
// under CGameMgr@WAP32; it is NEITHER the base WAP32::CGameMgr (vftable
// 0x5e9b8c) NOR CGruntzMgr (vftable 0x5e9b64). It is a (storage-free) method on
// CGameMgr ONLY so MSVC mangles it to the retail symbol name; see the note on
// the declaration in Wap32.h.
// The DirectInput device-config grand-base (vftable 0x5ef670 == ??_7CInputDevRoot,
// named in DirectInputMgr2.cpp) + its base-subobject teardown (0x134d50). The
// engine allocator's operator delete. All reloc-masked.
extern void* deviceConfigRootTable; // 0x5ef670 (the CInputDevRoot vtable datum)
// The DirectInput device-config grand-base (full class in <DinMgr2/DirectInputMgr2.h>);
// partial view for the base teardown this dtor runs.
struct CInputDevRoot {
    void ReleaseDevices(); // 0x134d50
};
void operator delete(void*); // engine allocator (0x1b9b82)

// WAP32::CGameMgr::vector_deleting_destructor @0x133380 - the CInputDevRoot scalar-
// deleting dtor (mangled through CGameMgr for the retail symbol name): stamp the C
// vftable, run the base teardown, conditionally free, return `this`.
// @early-stop
// cross-class-alias wall: this is really a CInputDevRoot scalar-deleting dtor but
// the delinker mangled it under CGameMgr, so it cannot be expressed as a real
// ~CInputDevRoot here (that ??_G is auto-emitted, unbound, by DirectInputMgr2). The
// vptr re-stamp of a FOREIGN class's vtable is a non-ctor stamp cl cannot realize
// (vtable-realization-ctor-boundary), so it stays an explicit `*(void**)this` store
// of the reloc-masked CInputDevRoot vtable datum. Code bytes match.
RVA(0x00133380, 0x24)
void* WAP32::CGameMgr::vector_deleting_destructor(unsigned int flags) {
    *(void**)this = &deviceConfigRootTable;
    ((CInputDevRoot*)this)->ReleaseDevices();
    if (flags & 1) {
        operator delete(this);
    }
    return this;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------

// CGameApp::scalar-dtor @0x080dd0 - the CGameApp scalar-deleting destructor (the
// ??_GCGameApp thunk with ~CGameApp inlined). Real polymorphic: the explicit
// qualified this->CGameApp::~CGameApp() inlines the (inline, virtual) dtor, whose
// auto vptr-restore stamps ??_7CGameApp@@6B@ (0x5e9b0c) and whose body runs
// CloseResources() + the instance-counter decrement - so the manual vtable stamp
// (and the retail-vtable / instance-counter aliases) are gone.
// @rva-symbol: ??_GCGameApp@@UAEPAXI@Z 0x00080dd0 0x32  (cl-auto-gen scalar-deleting dtor)

// size 0x2c recovered from operator-new sites (gruntz.analysis.news)
SIZE(WAP32::CGameMgr, 0x2c);

// Wap32.h class metadata (hosted here at the owning .cpp's EOF so the hot
// engine header stays untouched; EOF append is line-/parse-shift-neutral).
// The real polymorphic CGameApp emits ??_7CGameApp@@6B@ from this TU; bind its
// retail vtable RVA here (moved from the deleted src/Stub/BoundaryLowerThunks.cpp).
VTBL(CGameApp, 0x001e9b0c);
SIZE(
    CGameApp,
    0x254
); // == SIZE(CGruntzApp): the derived app adds no fields (m_errorDetail@0x250 last)
SIZE(GameInfo, 0x1d4); // self-describing: size field == sizeof == 0x1d4
