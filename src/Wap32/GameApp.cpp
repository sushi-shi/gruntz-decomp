// GameApp.cpp - WAP32 CGameApp (Brian Goble's engine).
// Matched: CGameApp::CGameApp (byte-exact code; the instance
// counter is a file-scope global here - same store sequence, the
// reloc just names a different symbol than the Ghidra DAT_ at that address).
#include <Wap32/Wap32.h>
#include <EmptyString.h> // g_emptyString
#include <rva.h>
#include <string.h>
#include <stdio.h>
#include <Globals.h>
// timeGetTime (WINMM frame clock) comes from <Mfc.h>'s central decl (via <Wap32.h>).
// 0x13ddc0-0x13df00 are WAP32::CGameMgr's own - declared in <Wap32/Wap32.h>,
// defined below inside CGameMgr's contiguous retail method block. 0x13ddc0 is the
// base PerFrameTick: retail ??_7CGameMgr @0x5e9b8c slot 4 holds it directly.)

// Two run-state timing defaults CGameMgr::Run seeds to 0x64 (100).

// Instance counter (bumped per ctor). Canonical DATA binding (0x253c6c) lives in
// src/Globals.cpp (declared via <Globals.h>, included above) - it is the retail
// global the placeholder g_instCount653c6c named; referenced here, reloc-masked.

// -------------------------------------------------------------------------
// CGameApp::InitDefault (vtbl +0x0c) - the one-name convenience overload: forward
// to the virtual Init using `szName` for BOTH the window name and the game
// identifier, an empty command line, no flags, and default (CW_USEDEFAULT) size.

RVA(0x00080d20, 0x24)
i32 CGameApp::InitDefault(HINSTANCE hInstance, char* szName) {
    return Init(hInstance, szName, szName, g_emptyString, 0, (i32)0x80000000, (i32)0x80000000);
}

// CGameApp::HasWindowAndManager (vtbl +0x14, slot 5) - readiness gate: nonzero
// only when both the game window and game manager are constructed.
RVA(0x00080d60, 0x18)
i32 CGameApp::HasWindowAndManager() {
    return m_gameWnd != 0 && m_gameMgr != 0;
}

// CGameApp::HandleCommand (vtbl +0x28, slot 10) - the default WM_COMMAND
// handler: unhandled, returns 0. (CGruntzApp overrides it at 0x080aa0.)
RVA(0x00080d90, 0x5)
i32 CGameApp::HandleCommand(i32, i32, i32) {
    return 0;
}

// (0x133380 used to live here as a fake `WAP32::CGameMgr::vector_deleting_destructor`
// over a fabricated `deviceConfigRootTable` global and a local CInputDevRoot view. It is
// neither: it is CInputDevRoot's SCALAR-DELETING DESTRUCTOR `??_GCInputDevRoot@@UAEPAXI@Z`
// - the vptr it stamps, 0x1ef670, IS ??_7CInputDevRoot@@6B@, and retail emits the COMDAT
// inside DirectInputMgr2's block. cl already auto-emits that ??_G into directinputmgr2's
// obj, so nothing had to be written at all - it just had to be NAMED there. The label now
// lives in DinMgr2.cpp (an rva-symbol pin on that ??_G, next to VTBL(CInputDevRoot)), which
// also homes the function to its real TU. The fake global + view + method decl are gone.)

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

// -------------------------------------------------------------------------
// CGameApp::ReportError (vtbl +0x1c, slot 7; 0x13dcb0) - the engine's report-once
// error latch. It is a CGameApp base virtual: the SAME body pointer sits at
// ??_7CGameApp@@6B@+0x1c AND ??_7CGruntzApp@@6B@+0x1c (CGruntzApp inherits, no override).
// Guarded by m_errorReported: the first call latches it and - when the game window is up
// and not already closing (m_closeGuard == 0) - posts WM_CLOSE to it; then clears the run
// gate and stashes the (code, detail) pair ShowError later reads. __thiscall(2).
// The cached PostMessageA fn-ptr @0x6c44c8 (extern "C" so the reloc emits the canonical
// _g_pPostMessageA - the single name bound there).
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

// ~CGameApp is now inline in Wap32.h (CloseResources() + counter dec); it is
// still emitted in this TU (the vtable's scalar-deleting dtor references it).

// (the three "CGameMgr vtable anchor" stubs that used to sit here are GONE. They were not
// anchors, they were a SECOND definition of functions this tree already reconstructs at
// their retail rvas in src/Gruntz/GruntzMgr.cpp - Wap32GameMgrVfunc3 @0x85560,
// PerFrameTick, HandleCommand @0x85580 - and the Vfunc3 stub was WRONG: it returned a
// constant 0 where retail returns `m_gameWnd != 0` (`mov edx,[ecx+4]; xor eax,eax;
// test edx,edx; setne al; ret`). One mangled name, two byte-shapes, and MSVC5 keeps exactly
// one COMDAT - so the linker could have handed every caller in this tree the stub that says
// "no game window is ever bound". Declared-only in <Wap32/Wap32.h> is all this TU needs:
// the vtable slot's reloc binds to the real body at link.)

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
    m_pacingGate = 0;
    InitTimeFields(1);
    InitializeTimeGlobal();
}

// The WAP32 frame-timing / app-instance state owned by CGameApp/CGameMgr (.bss,
// zero-init), RVA-ascending. g_gameAppInstanceCount is the CGameApp instance counter
// (bumped in the ctor above); g_wap32Now/g_wap32FrameDelta the canonical frame clock +
// delta this TU's UpdateTimeGlobal writes; g_wap32ClockReset the timeGetTime latch
// InitializeTimeGlobal reseeds; g_wap32Run7c/g_wap32Run80 the run-timing countdown + its
// reload value (primed to 100 here). Referenced by RezMgr/GruntzMgr/GruntzApp/... too;
// reference externs stay in <Globals.h> (included above). (REHOME DD-Drain-1 / DD-D)
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
    m_pacingGate = 0;
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

// The frame clock. Retail does NOT call the WINMM import thunk directly; it caches
// timeGetTime in a game-owned global pointer (_g_pTimeGetTime @ RVA 0x2c4650, pinned
// in cplay/globals) and calls through it (ff 15). extern "C" so the reloc binds the
// canonical one-symbol-per-RVA at whole-game link (was the raw __imp__timeGetTime@0).

// -------------------------------------------------------------------------
// CGameMgr::PerFrameTick() (0x13ddc0; vtable +0x10 idx4 - retail ??_7CGameMgr
// @0x5e9b8c slot 4 holds this body DIRECTLY, byte-verified) - the base per-frame
// tick. Sample timeGetTime, derive the per-frame delta into the canonical
// g_wap32Now/g_wap32FrameDelta cells, run down the run-state countdown, then
// (when the pacing gate m_pacingGate is armed) busy-wait to the ms budget and,
// every ~2s window, fold the frame count into m_fps and rearm the window.
// CGruntzMgr overrides it (its slot 4 = thunk 0x1c7b -> 0x8b740, the game tick
// in src/Rez/RezMgr.cpp) and calls this base body first - the direct
// `call 0x13ddc0` there is the qualified base-call.
//
// RVA interleave inside CGameMgr's contiguous method block, field-for-field slot
// identity at +0x18..+0x28, and the CGameRegistry third view - lives on in the
// <Wap32/Wap32.h> member comments. Its own duplicate `InitTimeFields` decl is long
// deleted; the call below is a plain same-class member call that binds to the
// real ?InitTimeFields@CGameMgr@WAP32@@ at 0x13de70.)
RVA(0x0013ddc0, 0xaa)
i32 WAP32::CGameMgr::PerFrameTick() {
    // Cache the fnptr in a local so cl loads it once (mov edi,[_g_pTimeGetTime]) and
    // reuses it across the three samples (call edi), exactly as retail does.
    DWORD(WINAPI * pTGT)(void) = ::timeGetTime;
    u32 now = pTGT();
    u32 delta = now - (u32)g_wap32Now;
    g_wap32Now = now;
    g_wap32FrameDelta = delta;
    u32 run7c = (u32)g_wap32Run7c;
    if (run7c == 0) {
        g_wap32Run7c = g_wap32Run80;
    } else if (delta >= run7c) {
        g_wap32Run7c = 0;
    } else {
        g_wap32Run7c = run7c - delta;
    }

    if (m_pacingGate > 0) {
        if ((u32)g_wap32ClockReset > 0) {
            u32 elapsed = pTGT() - (u32)g_wap32ClockReset;
            if (elapsed < (u32)m_frameBudgetMs) {
                SpinWaitUntil(m_frameBudgetMs - elapsed);
            }
        }
        g_wap32ClockReset = pTGT();
    }

    u32 count = m_frameCounter + 1;
    m_frameCounter = count;
    if ((u32)g_wap32Now - (u32)m_windowStartTick >= 0x7d0) {
        m_fps = count >> 1;
        InitTimeFields(0); // 0x13de70 (defined below; direct call rel32)
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGameMgr::InitTimeFields  (__thiscall; ctor/Run helper @0x13de70)
// Zeroes the frame counter, samples the fps-window start tick, and (when reset) arms m_fps.
RVA(0x0013de70, 0x23)
void WAP32::CGameMgr::InitTimeFields(i32 reset) {
    m_frameCounter = 0;
    m_windowStartTick = timeGetTime();
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
void WAP32::CGameMgr::SpinWaitUntil(i32 ms) {
    DWORD(WINAPI * fn)(void) = ::timeGetTime;
    u32 now = fn();
    u32 end = now + (u32)ms;
    if (now <= end) {
        do {
            now = fn();
        } while (now <= end);
    }
}

// ---------------------------------------------------------------------------
// CGameMgr::SetFrameRate(fps) (0x13dee0; ex RezMgr::): store the frame rate in
// the pacing gate (m_pacingGate @+0x1c) and, when positive, derive the per-frame
// budget (m_frameBudgetMs @+0x28 = 1000/fps). __thiscall, 1 arg.
RVA(0x0013dee0, 0x1b)
void WAP32::CGameMgr::SetFrameRate(i32 fps) {
    m_pacingGate = fps;
    if (fps > 0) {
        m_frameBudgetMs = 1000 / fps;
    }
}

// CGameMgr::TrySetFrameRate(fps) (0x13df00; ex RezMgr::): install the rate only
// when pacing is not already active (m_pacingGate > 0 -> clear it via
// SetFrameRate(0) and fail with 0); otherwise configure to fps and succeed
// (return 1). __thiscall, 1 arg.
RVA(0x0013df00, 0x25)
i32 WAP32::CGameMgr::TrySetFrameRate(i32 fps) {
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
// @orphan: no .text caller (a free __cdecl busy-wait); no owning class.
// @early-stop
// regalloc-swap wall (~97%): byte-identical except retail pins `vk` in esi and the
// cached GetAsyncKeyState ptr in edi, while our /O2 picks the reverse (ptr->esi,
// vk->edi). Only the modrm reg fields differ; tried direct global calls (77%, no
// caching) and an `int k = vk` copy (no change). Pure register assignment.
RVA(0x0013df30, 0xaf)
void WaitKeyEdge(int vk, int timeoutMs) {
    if (timeoutMs == 0) {
        SHORT(WINAPI * gaks)(int) = ::GetAsyncKeyState;
        while (!((i32)gaks(vk) & 0x80000000))
            ;
        while ((i32)gaks(vk) & 0x80000000)
            ;
    } else {
        DWORD(WINAPI * tgt)(void) = ::timeGetTime;
        u32 deadline = tgt() + timeoutMs;
        SHORT(WINAPI * gaks)(int) = ::GetAsyncKeyState;
        while (!((i32)gaks(vk) & 0x80000000)) {
            if (tgt() > deadline) {
                return;
            }
        }
        while ((i32)gaks(vk) & 0x80000000) {
            if (tgt() > deadline) {
                return;
            }
        }
    }
}

// CGameApp::GameWindowProc (the static WNDPROC) is reconstructed in GameWnd.cpp:
// its code lives in the CGameWnd address cluster and dispatches to the active
// CGameWnd singleton (s_activeWnd). This TU only references it (the WNDCLASS
// store in InitializeDefaultWindowClass).

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

// CGameApp::~CGameApp @0x080cf0 - the STANDALONE out-of-line copy of the (inline,
// header-defined) base dtor, referenced only by a /GX EH-unwind funclet (base-subobject
// cleanup when a CGameApp-derived ctor's member throws). ??_GCGameApp above and
// CGruntzApp's cross-TU dtor keep folding their own inline copies; this forcer (the
// UserLogicCtorEmit #pragma inline_depth(0) pattern) emits the out-of-line COMDAT in
// this unit so the unwind reference resolves and the RVA is matched.
// @rva-symbol: ??1CGameApp@@UAE@XZ 0x00080cf0 0x12
static CGameApp* volatile g_forceEmitCGameApp;
#pragma inline_depth(0)
void ForceEmitCGameAppDtor() {
    g_forceEmitCGameApp->CGameApp::~CGameApp();
}
#pragma inline_depth()

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
