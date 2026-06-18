// GameWnd.cpp - WAP32 CGameWnd (Brian Goble's engine).
// Matched: ??0CGameWnd@@QAE@XZ @ RVA 0x13cf00 (byte-exact).
#include "Wap32.h"

extern "C" {
__declspec(dllimport) BOOL __stdcall IsWindow(HWND hWnd);
__declspec(dllimport) BOOL __stdcall DestroyWindow(HWND hWnd);
__declspec(dllimport) void __stdcall PostQuitMessage(int nExitCode);
}

// -------------------------------------------------------------------------
// CGameWnd::CGameWnd()
// Zeroes the OS window handle (m_4) and owner-state field (m_c) after the
// base/vftable construction.
//
// @address: 0x13cf00
// @size:    0x11
// -------------------------------------------------------------------------
CGameWnd::CGameWnd()
{
    m_4 = 0;
    m_c = 0;
}

// active-window singleton (binary: CGameWnd* @ 0x653c68). Set by CreateAndShow;
// read by GameWindowProc to dispatch messages to this object. Declared non-static
// so the Gruntz-side CGameWnd ctor/dtor (in GruntzWnd.cpp) can reference it.
CGameWnd *s_activeWnd;

// -------------------------------------------------------------------------
// CGameWnd::CreateAndShow
// Creates the OS window from the caller's CreateWindowExA params struct,
// installs this object as the active-window singleton, then ShowWindow(SW_
// SHOWNORMAL). Bails (returning 0) if params/owner is null or a window is
// already active.
//
// @address: 0x13cf20
// @size:    0x8f
// -------------------------------------------------------------------------
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
//
// @address: 0x13cfb0
// @size:    0x39
// -------------------------------------------------------------------------
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
//
// @address: 0x13d490
// @size:    0x29
// -------------------------------------------------------------------------
int CGameWnd::QuitMessageLoop()
{
    ((CGameApp *)m_8)->FreeGameManager();
    if (((CGameApp *)m_8)->m_248)
        ((CGameApp *)m_8)->ShowError();
    PostQuitMessage(0);
    return 0;
}

// Out-of-line stub so the vftable (??_7CGameWnd@@6B@) is emitted in this TU.
// Not matched / not in symbol_names.csv; present only to anchor the vftable.
// ~CGameWnd is now inline in Wap32.h (the inline body anchors the vtable slot).
int CGameWnd::Wap32GameWndVfunc0() { return 0; }

// -------------------------------------------------------------------------
// CGameWnd::SetAppField  @0x13d470 (18 B) - store wParam to owner->m_240
// -------------------------------------------------------------------------
void CGameWnd::SetAppField(int wParam, int lParam)
{
    ((CGameApp *)m_8)->m_240 = wParam;
}

// -------------------------------------------------------------------------
// CGameWnd::DestroyWindowSelf  @0x13d4c0 (30 B)
// -------------------------------------------------------------------------
int CGameWnd::DestroyWindowSelf()
{
    if (m_c == 0) { m_c = 1; DestroyWindow(m_4); }
    return 1;
}

// -------------------------------------------------------------------------
// CGameWnd::DrainMessages  @0x13d4e0 (67 B)
// -------------------------------------------------------------------------
int CGameWnd::DrainMessages(int filter, int count)
{
    int drained = 0;
    if (count > 0) {
        MSG msg;
        while (PeekMessageA(&msg, m_4, filter, filter, 1)) {
            if (++drained >= count) break;
        }
    }
    return drained;
}
