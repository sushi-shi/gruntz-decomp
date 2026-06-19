// GameWnd.cpp - WAP32 CGameWnd (Brian Goble's engine).
// Matched: CGameWnd::CGameWnd (byte-exact).
#include "Wap32.h"
#include "../rva.h"

extern "C" {
__declspec(dllimport) BOOL __stdcall IsWindow(HWND hWnd);
__declspec(dllimport) BOOL __stdcall DestroyWindow(HWND hWnd);
__declspec(dllimport) void __stdcall PostQuitMessage(int nExitCode);
}

// -------------------------------------------------------------------------
// CGameWnd::CGameWnd()
// Zeroes the OS window handle (m_4) and owner-state field (m_c) after the
// base/vftable construction.
RVA(0x13cf00, 0x11)
CGameWnd::CGameWnd()
{
    m_4 = 0;
    m_c = 0;
}

// File-scope active-window singleton. Set by
// CreateAndShow; read by GameWindowProc to dispatch messages to this object.
static CGameWnd *s_activeWnd;

// -------------------------------------------------------------------------
// CGameWnd::CreateAndShow
// Creates the OS window from the caller's CreateWindowExA params struct,
// installs this object as the active-window singleton, then ShowWindow(SW_
// SHOWNORMAL). Bails (returning 0) if params/owner is null or a window is
// already active.
RVA(0x13cf20, 0x8f)
int CGameWnd::CreateAndShow(CGameWndCreateParams *pParams, void *pOwner)
{
    if (!pParams)
        return (int)pParams;
    if (!pOwner)
        return 0;
    if (s_activeWnd)
        return 0;

    m_8 = pOwner;
    s_activeWnd = this;
    m_c = 0;

    m_4 = CreateWindowExA(pParams->dwExStyle, pParams->lpClassName,
                          pParams->lpWindowName, pParams->dwStyle,
                          pParams->X, pParams->Y, pParams->nWidth,
                          pParams->nHeight, pParams->hWndParent,
                          pParams->hMenu, pParams->hInstance, pParams->lpParam);
    if (!m_4)
        return 0;

    ShowWindow(m_4, 1 /*SW_SHOWNORMAL*/);
    return 1;
}

// -------------------------------------------------------------------------
// CGameWnd::Destroy
// Destroys the OS window if it is still valid, clears owner/window state, and
// clears the active-window singleton.
RVA(0x13cfb0, 0x39)
void CGameWnd::Destroy()
{
    if (m_4) {
        if (IsWindow(m_4))
            DestroyWindow(m_4);
        m_4 = 0;
    }
    m_8 = 0;
    s_activeWnd = 0;
}

// -------------------------------------------------------------------------
// CGameWnd::QuitMessageLoop
// Frees the game manager through the owning app, optionally reports the stored
// error, then posts WM_QUIT.
RVA(0x13d490, 0x29)
int CGameWnd::QuitMessageLoop()
{
    ((CGameApp *)m_8)->FreeGameManager();
    if (((CGameApp *)m_8)->m_248)
        ((CGameApp *)m_8)->ShowError();
    PostQuitMessage(0);
    return 0;
}

// Out-of-line stubs so the vftable is emitted in this TU.
// Not matched / not in symbol_names.csv; present only to anchor the vftable
// relocation that the ctor stores.
CGameWnd::~CGameWnd() {}
int CGameWnd::Wap32GameWndVfunc0() { return 0; }
