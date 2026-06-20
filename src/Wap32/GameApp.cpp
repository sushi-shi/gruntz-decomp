// GameApp.cpp - WAP32 CGameApp (Brian Goble's engine).
// Matched: CGameApp::CGameApp (byte-exact code; the instance
// counter is a file-scope global here - same store sequence, the
// reloc just names a different symbol than the Ghidra DAT_ at that address).
#include <Wap32/Wap32.h>
#include <rva.h>
#include <string.h>
#include <stdio.h>

extern "C" {
__declspec(dllimport) DWORD __stdcall timeGetTime(void);
}

DATA(0x253c70)
extern int g_wap32Now;
DATA(0x253c74)
extern int g_wap32FrameDelta;
DATA(0x253c78)
extern int g_wap32ClockReset;
// Two run-state timing defaults CGameMgr::Run seeds to 0x64 (100).
DATA(0x253c7c)
extern int g_wap32Run7c;
DATA(0x253c80)
extern int g_wap32Run80;

// Instance counter (bumped per ctor). Shared
// (declared in Wap32.h) so the inline ~CGameApp - which CGruntzApp's dtor
// inlines in another TU - resolves it; the reloc name is masked in objdiff.
int g_gameAppInstanceCount;

// -------------------------------------------------------------------------
// CGameApp::CGameApp()
// Zeroes the resource/window/manager pointers and the error-state fields,
// then bumps the file-scope instance counter.
RVA(0x13d590, 0x3c)
CGameApp::CGameApp()
{
    m_4   = 0;
    m_8   = 0;
    m_10  = 0;   // the optimiser schedules the +0x10 store before +0x0c
    m_c   = 0;
    m_240 = 0;
    m_248 = 0;
    m_24c = 0;
    m_250 = 0;
    g_gameAppInstanceCount++;
}

// -------------------------------------------------------------------------
// CGameApp::CloseResources
// Frees the accelerator table then deletes the two resource objects.
RVA(0x13d8c0, 0x42)
void CGameApp::CloseResources()
{
    if (m_10) {
        DestroyAcceleratorTable(m_10);
        m_10 = 0;
    }
    if (m_8) {
        delete m_8;
        m_8 = 0;
    }
    if (m_4) {
        delete m_4;
        m_4 = 0;
    }
}

// -------------------------------------------------------------------------
// CGameApp::InitializeAccelerators
// Reloads the accelerator table; returns whether it loaded.
RVA(0x13dc20, 0x49)
BOOL CGameApp::InitializeAccelerators(LPCSTR lpTable)
{
    if (lpTable && *lpTable) {
        if (m_10) {
            DestroyAcceleratorTable(m_10);
            m_10 = 0;
        }
        m_10 = LoadAcceleratorsA(m_c, lpTable);
        return m_10 != 0;
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGameApp::ReportError
// Records an error once (guarded by m_248), posting WM_CLOSE to the window.
RVA(0x13dcb0, 0x57)
void CGameApp::ReportError(WPARAM wParam, LPARAM lParam)
{
    if (m_248)
        return;
    m_248 = 1;
    if (m_4 && m_4->m_c == 0)
        PostMessageA((HWND)m_4->m_4, 0x10 /*WM_CLOSE*/, 0, 0);
    m_244 = 0;
    m_24c = (int)wParam;
    m_250 = (int)lParam;
}

// -------------------------------------------------------------------------
// CGameApp::RunMessageLoop - the main Win32 pump (vtbl slot +0x18; WinMain
// dispatches here). Reads the OS HWND off m_4->m_4; if there is no window,
// return 0. Otherwise the classic peek/process/idle pump: PeekMessageA
// (PM_REMOVE) drains all pending messages (WM_QUIT exits with 1); when m_10
// (HACCEL) is set AND the message targets our window, run TranslateAcceleratorA
// (return ignored); always TranslateMessage + DispatchMessageA; when the queue
// is empty, call the idle virtual (vtbl +0x20) and loop.
RVA(0x13d910, 0x9f)
int CGameApp::RunMessageLoop()
{
    MSG msg;

    HWND hwnd = (HWND)m_4->m_4;
    if (!hwnd)
        return 0;

    for (;;) {
        if (PeekMessageA(&msg, 0, 0, 0, 1)) {
            do {
                if (msg.message == 0x12 /*WM_QUIT*/)
                    return 1;
                if (m_10 && msg.hwnd == hwnd)
                    TranslateAcceleratorA(hwnd, m_10, &msg);
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } while (PeekMessageA(&msg, 0, 0, 0, 1));
        }
        VirtualUnknownMethod09();   // idle virtual (vtbl +0x20)
    }
}

// -------------------------------------------------------------------------
// CGameApp::InitializeDefaultWindowClass
// Fills the embedded WNDCLASSA (m_wc @ +0x1e8) and loads its icon/cursor.
RVA(0x13d9b0, 0xa0)
void CGameApp::InitializeDefaultWindowClass()
{
    int i;
    for (i = 0; i < 10; i++)
        ((int *)&m_wc)[i] = 0;

    HCURSOR hCursor = LoadCursorA(m_c, m_gameInfo.szGameIdentifier);
    if (m_gameInfo.windowClassFlags & 1)
        hCursor = LoadCursorA(0, (LPCSTR)0x7f00 /*IDC_ARROW*/);

    m_wc.style         = 8;   // CS_DBLCLKS
    m_wc.lpfnWndProc   = GameWindowProc;
    m_wc.cbClsExtra    = 0;
    m_wc.cbWndExtra    = 0;
    m_wc.hInstance     = m_c;
    m_wc.hIcon         = LoadIconA(m_c, m_gameInfo.szGameIdentifier);
    m_wc.hCursor       = hCursor;
    m_wc.hbrBackground = (HBRUSH)GetStockObject(4);
    m_wc.lpszMenuName  = 0;
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
RVA(0x13db60, 0x57)
CGameWnd *CGameApp::InitializeGameWindow()
{
    return new CGameWnd;
}

// -------------------------------------------------------------------------
// CGameApp::VirtualUnknownMethod03
// Builds a GameInfo descriptor on the stack from the launch parameters, then
// hands it to VirtualUnknownMethod02 (vtable +0x4) to register+create.
// hInstance is required (null -> 0). The three name strings are conditionally
// strcpy'd (inline rep movs at /O2/Oi).
RVA(0x13d7b0, 0x105)
int CGameApp::VirtualUnknownMethod03(HINSTANCE hInstance, char *szWindowName,
                                     char *szGameIdentifier, char *szCmdLine,
                                     int windowClassFlags, int windowWidth,
                                     int windowHeight)
{
    GameInfo gi;

    if (!hInstance)
        return 0;

    memset(&gi, 0, sizeof(gi));
    gi.hInstance        = hInstance;
    gi.size             = sizeof(GameInfo);
    gi.windowClassFlags = windowClassFlags;
    gi.windowWidth      = windowWidth;
    gi.windowHeight     = windowHeight;
    if (szWindowName)
        strcpy(gi.szWindowName, szWindowName);
    if (szGameIdentifier)
        strcpy(gi.szGameIdentifier, szGameIdentifier);
    if (szCmdLine)
        strcpy(gi.szCmdLine, szCmdLine);

    return VirtualUnknownMethod02(&gi, 0, 0);
}

// -------------------------------------------------------------------------
// CGameApp::InitializeDefaultCreateStruct
// Fills the embedded CREATESTRUCTA (m_createStruct @ +0x210) with default
// window geometry/style derived from the GameInfo windowClassFlags:
//   bit1 (Windowed) -> a "Gruntz" menu, gameInfo width/height, overlapped or
//   caption style; otherwise -> fullscreen popup at the screen metrics.
RVA(0x13da50, 0x10b)
void CGameApp::InitializeDefaultCreateStruct()
{
    int i;
    for (i = 0; i < 12; i++)
        ((int *)&m_createStruct)[i] = 0;

    HMENU hMenu = 0;
    if (m_gameInfo.windowClassFlags & 1)
        hMenu = LoadMenuA(m_c, m_gameInfo.szGameIdentifier);

    // x and y both == CW_USEDEFAULT (windowed) or 0 (fullscreen). Kept as two
    // separate variables (the target materializes x in a register, y in a stack
    // slot, both assigned in one branch - a single var folds to a branchless
    // neg/sbb/and on the 0x80000000 mask).
    int x, y;
    if (m_gameInfo.windowClassFlags & 1) {
        x = (int)0x80000000;
        y = (int)0x80000000;
    } else {
        x = 0;
        y = 0;
    }

    // Width/height: the requested size when windowed, the screen otherwise.
    int cx, cy;
    if (m_gameInfo.windowClassFlags & 1) {
        cx = m_gameInfo.windowWidth;
        cy = m_gameInfo.windowHeight;
    } else {
        cx = GetSystemMetrics(0); // SM_CXSCREEN
        cy = GetSystemMetrics(1); // SM_CYSCREEN
    }

    long  style;
    DWORD exStyle;
    if (m_gameInfo.windowClassFlags & 1) {
        style = 0xcf0000;         // WS_OVERLAPPEDWINDOW (default)
        exStyle = 0x40000;
        if (m_gameInfo.windowClassFlags & 2)
            style = 0xca0000;     // DialogFrame: caption + sysmenu
    } else {
        style   = 0x80080000;     // WS_POPUP | ...
        exStyle = 0x40008;
    }

    m_createStruct.style          = style;
    m_createStruct.hInstance      = m_c;
    m_createStruct.hMenu          = hMenu;
    m_createStruct.y              = y;
    m_createStruct.cx             = cx;
    m_createStruct.lpCreateParams = 0;
    m_createStruct.hwndParent     = 0;
    m_createStruct.x              = x;
    m_createStruct.cy             = cy;
    m_createStruct.lpszName       = m_gameInfo.szWindowName;
    m_createStruct.lpszClass      = m_gameInfo.szWindowClassName;
    m_createStruct.dwExStyle      = exStyle;
}

// -------------------------------------------------------------------------
// CGameApp::VirtualUnknownMethod02
// The Run/Init orchestration: validate the GameInfo, copy it into the member,
// resolve hInstance, build the class+window names, register the class, create
// the window via CGameWnd::CreateAndShow, then bring up the game manager.
RVA(0x13d5d0, 0x1d3)
int CGameApp::VirtualUnknownMethod02(GameInfo *pGameInfo, WNDCLASSA *pWndClass,
                                     CREATESTRUCTA *pCreateStruct)
{
    HINSTANCE hInst;

    if (g_gameAppInstanceCount > 1)
        goto Fail;
    if (!pGameInfo || pGameInfo->size != 0x1d4)
        goto Fail;
    if (pWndClass && (!pWndClass->lpszClassName || !*pWndClass->lpszClassName))
        goto Fail;

    m_244 = 1;
    m_248 = 0;
    m_24c = 0;
    m_250 = 0;
    m_gameInfo = *pGameInfo;

    hInst = m_gameInfo.hInstance;
    if (!hInst && (!pWndClass || !(hInst = pWndClass->hInstance))
              && (!pCreateStruct || !(hInst = pCreateStruct->hInstance)))
        goto Fail;
    m_c = hInst;

    if (!m_gameInfo.szWindowClassName[0])
        sprintf(m_gameInfo.szWindowClassName, "%sClass", m_gameInfo.szGameIdentifier);
    if (!m_gameInfo.szWindowName[0])
        sprintf(m_gameInfo.szWindowName, "%s", m_gameInfo.szGameIdentifier);

    if (pWndClass)
        m_wc = *pWndClass;
    else
        InitializeDefaultWindowClass();

    if (pCreateStruct)
        m_createStruct = *pCreateStruct;
    else
        InitializeDefaultCreateStruct();

    if (!RegisterClassA(&m_wc))
        goto Fail;

    InitializeAccelerators(m_gameInfo.szGameIdentifier);

    m_4 = (CGameResource *)InitializeGameWindow();
    if (!m_4)
        goto Fail;

    if (!((CGameWnd *)m_4)->CreateAndShow((CGameWndCreateParams *)&m_createStruct, this)) {
        delete m_4;
        m_4 = 0;
        return 0;
    }

    m_8 = (CGameResource *)InitializeGameManager();
    if (!m_8)
        goto Fail;

    if (!((WAP32::CGameMgr *)m_8)->Run((CGameWnd *)m_4, m_gameInfo.szCmdLine)) {
        delete m_8;
        m_8 = 0;
        return 0;
    }
    return 1;

Fail:
    return 0;
}

// -------------------------------------------------------------------------
// CGameApp::VirtualUnknownMethod09 - vtbl slot +0x20.
// The per-frame idle virtual the message pump calls on an empty queue
// (RunMessageLoop dispatches `call [vtbl+0x20]`). When the app is active -
// both gate words m_240 and m_244 set - it tail-calls the game manager's
// per-frame tick (m_8->vtbl +0x10, the 5th vtable slot). The tail call emits
// `mov ecx,[m_8]; mov eax,[ecx]; jmp [eax+0x10]` (no own epilogue needed since
// neither gate-load disturbs a callee-saved reg).
RVA(0x13dc70, 0x1d)
void CGameApp::VirtualUnknownMethod09()
{
    if (m_240 && m_244)
        m_8->PerFrameTick();
}

// -------------------------------------------------------------------------
// CGameApp::FreeGameManager - vtbl slot +0x24.
// `delete m_8; m_8 = 0;` - frees the game manager via its scalar-deleting
// dtor (slot 0, `push 1; call [vtbl]`) and clears the slot. `this` is spilled
// to esi at entry; the null-check skips both when m_8 is already 0.
RVA(0x13dc90, 0x19)
void CGameApp::FreeGameManager()
{
    if (m_8) {
        delete m_8;
        m_8 = 0;
    }
}

// ~CGameApp is now inline in Wap32.h (CloseResources() + counter dec); it is
// still emitted in this TU (the vtable's scalar-deleting dtor references it).

// Out-of-line stubs so referenced symbols are emitted in this TU.
CGameResource::~CGameResource() {}
void CGameResource::Wap32GameResVfunc1() {}
void CGameResource::Wap32GameResVfunc2() {}
void CGameResource::Wap32GameResVfunc3() {}
void CGameResource::PerFrameTick() {}
// CGameMgr vtable anchors (the dtor + the three otherwise-unmatched virtuals).
WAP32::CGameMgr::~CGameMgr() {}
void WAP32::CGameMgr::Wap32GameMgrVfunc3() {}
void WAP32::CGameMgr::Wap32GameMgrVfunc4() {}
void WAP32::CGameMgr::Wap32GameMgrVfunc5() {}

// -------------------------------------------------------------------------
// CGameMgr::CGameMgr()  (__thiscall, returns this in EAX; vftable @0x5e9b8c)
// Seeds the run-state flags (m_10/m_14 = 1), zeroes the owned pointers, then
// initialises the frame clock via the two time helpers. The optimiser hoists
// the m_10/m_14 stores (and the InitTimeFields `reset=1` argument push) above
// the vptr store.
RVA(0x13dd10, 0x35)
WAP32::CGameMgr::CGameMgr()
{
    m_10 = 1;
    m_14 = 1;
    m_4  = 0;
    m_8  = 0;
    m_c  = 0;
    m_1c = 0;
    InitTimeFields(1);
    UnknownMethodInitializeTimeGlobal();
}

// -------------------------------------------------------------------------
// CGameMgr::Run  (__thiscall; vtable +0x04, the engine-start entry).
// Binds the manager to the game window (records the window + its owner app),
// reseeds the frame clock, and primes the two run-timing globals to 100.
// Bails (returning 0) if there is no window, or the window has no OS HWND yet.
RVA(0x13dd50, 0x54)
int WAP32::CGameMgr::Run(CGameWnd *pGameWnd, char *szCmdLine)
{
    if (!pGameWnd)
        return 0;
    if (!pGameWnd->m_4)
        return 0;

    m_4  = (int)pGameWnd;
    m_8  = (int)pGameWnd->m_8;
    m_1c = 0;
    InitTimeFields(1);
    UnknownMethodInitializeTimeGlobal();
    g_wap32Run80 = 0x64;
    g_wap32Run7c = 0x64;
    return 1;
}

// -------------------------------------------------------------------------
// CGameMgr::UnknownClose  (vtable +0x08)
// Clears the two manager-owned pointers/handles.
RVA(0x13ddb0, 0x9)
void WAP32::CGameMgr::UnknownClose()
{
    m_4 = 0;
    m_8 = 0;
}

// -------------------------------------------------------------------------
// CGameMgr::InitTimeFields  (__thiscall; ctor/Run helper @0x13de70)
// Zeroes m_20, samples the start tick into m_24, and (when reset) arms m_18.
RVA(0x13de70, 0x23)
void WAP32::CGameMgr::InitTimeFields(int reset)
{
    m_20 = 0;
    m_24 = timeGetTime();
    if (reset)
        m_18 = -1;
}

// -------------------------------------------------------------------------
// CGameMgr::UnknownMethodInitializeTimeGlobal
// Seeds the frame clock from timeGetTime and clears its deltas.
RVA(0x13dea0, 0x18)
void WAP32::CGameMgr::UnknownMethodInitializeTimeGlobal()
{
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
// vector_deleting_destructor @0x133380 stores vftable 0x5ef670 and calls the
// 0x134d50 dtor body - it is a CGruntzMgr-family scalar-deleting dtor, NOT the
// base WAP32::CGameMgr's (whose dtor is the 0x401bb8 thunk in vtable slot 0).
// Deferred to the CGruntzMgr reconstruction; kept as a label here.
// @confidence: high
// @source: tomalla
// @stub
RVA(0x133380, 0x24)
void WAP32::CGameMgr::vector_deleting_destructor() {}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------

// @confidence: high
// @source: tomalla
// @stub
RVA(0x080dd0, 0x32)
void CGameApp::Stub_080dd0() {}
