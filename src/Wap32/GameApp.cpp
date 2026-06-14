// GameApp.cpp - WAP32 CGameApp (Brian Goble's engine).
// Matched: ??0CGameApp@@QAE@XZ @ RVA 0x13d590 (byte-exact code; the instance
// counter at 0x653c6c is a file-scope global here - same store sequence, the
// reloc just names a different symbol than the Ghidra DAT_ at that address).
#include "Wap32.h"
#include <string.h>
#include <stdio.h>

// The game manager (engine namespace WAP32). Only the two vtable slots that
// VirtualUnknownMethod02 dispatches through are needed: the scalar-deleting
// dtor (slot 0, for `delete`) and Run (slot +0x4).
namespace WAP32 {
class CGameMgr {
public:
    virtual ~CGameMgr();
    virtual int Run(CGameWnd *pGameWnd, char *szCmdLine);
};
}

// File-scope instance counter (binary: global int @ 0x653c6c, bumped per ctor).
static int s_gameAppCount;

// -------------------------------------------------------------------------
// CGameApp::CGameApp()
// Zeroes the resource/window/manager pointers and the error-state fields,
// then bumps the file-scope instance counter (binary global @ 0x653c6c).
//
// @address: 0x13d590
// @size:    0x3c
// -------------------------------------------------------------------------
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
    s_gameAppCount++;
}

// -------------------------------------------------------------------------
// CGameApp::CloseResources
// Frees the accelerator table then deletes the two resource objects.
//
// @address: 0x13d8c0
// @size:    0x42
// -------------------------------------------------------------------------
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
//
// @address: 0x13dc20
// @size:    0x49
// -------------------------------------------------------------------------
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
//
// @address: 0x13dcb0
// @size:    0x57
// -------------------------------------------------------------------------
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
// CGameApp::InitializeDefaultWindowClass
// Fills the embedded WNDCLASSA (m_wc @ +0x1e8) and loads its icon/cursor.
//
// @address: 0x13d9b0
// @size:    0xa0
// -------------------------------------------------------------------------
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
//
// @address: 0x13db60
// @size:    0x57
// -------------------------------------------------------------------------
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
//
// @address: 0x13d7b0
// @size:    0x105
// -------------------------------------------------------------------------
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
//
// @address: 0x13da50
// @size:    0x10b
// -------------------------------------------------------------------------
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
//
// @address: 0x13d5d0
// @size:    0x1d3
// -------------------------------------------------------------------------
int CGameApp::VirtualUnknownMethod02(GameInfo *pGameInfo, WNDCLASSA *pWndClass,
                                     CREATESTRUCTA *pCreateStruct)
{
    HINSTANCE hInst;

    if (s_gameAppCount > 1)
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

CGameApp::~CGameApp() {}

// Out-of-line stubs so referenced symbols are emitted in this TU.
CGameResource::~CGameResource() {}
WAP32::CGameMgr::~CGameMgr() {}
int WAP32::CGameMgr::Run(CGameWnd *, char *) { return 0; }
LRESULT __stdcall CGameApp::GameWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
