// GameWnd.cpp - WAP32 CGameWnd (Brian Goble's engine).
// Matched: CGameWnd::CGameWnd (byte-exact).
// <Mfc.h> brings <windows.h> USER32: IsWindow / DestroyWindow / PostQuitMessage / DefWindowProcA.
#include <Mfc.h>
#include <Wap32/Wap32.h>
#include <rva.h>

// -------------------------------------------------------------------------
// CGameWnd::CGameWnd()
// Zeroes the OS window handle (m_4) and owner-state field (m_c) after the
// base/vftable construction.
RVA(0x0013cf00, 0x11)
CGameWnd::CGameWnd() {
    m_4 = 0;
    m_c = 0;
}

// File-scope active-window singleton. Set by
// CreateAndShow; read by GameWindowProc to dispatch messages to this object.
static CGameWnd* s_activeWnd;

// -------------------------------------------------------------------------
// CGameWnd::CreateAndShow
// Creates the OS window from the caller's CreateWindowExA params struct,
// installs this object as the active-window singleton, then ShowWindow(SW_
// SHOWNORMAL). Bails (returning 0) if params/owner is null or a window is
// already active.
RVA(0x0013cf20, 0x8f)
int CGameWnd::CreateAndShow(CGameWndCreateParams* pParams, void* pOwner) {
    if (!pParams) {
        return (int)pParams;
    }
    if (!pOwner) {
        return 0;
    }
    if (s_activeWnd) {
        return 0;
    }

    m_8 = pOwner;
    s_activeWnd = this;
    m_c = 0;

    m_4 = CreateWindowExA(
        pParams->dwExStyle,
        pParams->lpClassName,
        pParams->lpWindowName,
        pParams->dwStyle,
        pParams->X,
        pParams->Y,
        pParams->nWidth,
        pParams->nHeight,
        pParams->hWndParent,
        pParams->hMenu,
        pParams->hInstance,
        pParams->lpParam
    );
    if (!m_4) {
        return 0;
    }

    ShowWindow(m_4, 1 /*SW_SHOWNORMAL*/);
    return 1;
}

// -------------------------------------------------------------------------
// CGameWnd::Destroy
// Destroys the OS window if it is still valid, clears owner/window state, and
// clears the active-window singleton.
RVA(0x0013cfb0, 0x39)
void CGameWnd::Destroy() {
    if (m_4) {
        if (IsWindow(m_4)) {
            DestroyWindow(m_4);
        }
        m_4 = 0;
    }
    m_8 = 0;
    s_activeWnd = 0;
}

// -------------------------------------------------------------------------
// CGameWnd::QuitMessageLoop
// Frees the game manager through the owning app, optionally reports the stored
// error, then posts WM_QUIT.
RVA(0x0013d490, 0x29)
int CGameWnd::QuitMessageLoop() {
    ((CGameApp*)m_8)->FreeGameManager();
    if (((CGameApp*)m_8)->m_248) {
        ((CGameApp*)m_8)->ShowError();
    }
    PostQuitMessage(0);
    return 0;
}

// -------------------------------------------------------------------------
// CGameApp::GameWindowProc - the static window procedure stored into
// WNDCLASS.lpfnWndProc (so __stdcall, the Win32 WNDPROC ABI: ret 0x10). Its code
// lives in the CGameWnd cluster because it dispatches to the active CGameWnd
// singleton (s_activeWnd, set by CreateAndShow):
//
//   1. If no window is active yet, DefWindowProcA passthrough.
//   2. Give the active window a first crack via PreDispatchMessage (vtbl +0x04)
//      for EVERY message; a nonzero return swallows it (WndProc returns 0).
//   3. Otherwise a switch(uMsg) routes each WM_* to its own CGameWnd virtual
//      (the slot offsets are load-bearing - see Wap32.h's vtable map). A handler
//      returning nonzero = "handled" => return 0; returning zero falls through to
//      DefWindowProcA. The singleton global is RE-READ in every case (the source
//      uses s_activeWnd directly, not a cached local - a cached pWnd would stay
//      in a saved register across the vtbl call instead of reloading [0x653c68]).
//
// Point messages (WM_MOVE / mouse) split lParam into LOWORD(x)/HIWORD(y); the
// (int) low/high words come straight off lParam (& 0xffff / >> 16).
RVA(0x0013cff0, 0x35c)
LRESULT __stdcall CGameApp::GameWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CGameWnd* pWnd = s_activeWnd;
    if (!pWnd) {
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    if (pWnd->PreDispatchMessage(uMsg, wParam, lParam)) {
        return 0;
    }

    // Case order below is the ORIGINAL source order (MSVC emits the case bodies
    // in source order, the jump tables index into them): window lifecycle, then
    // keyboard, command, then mouse. The order is load-bearing for the byte match.
    switch (uMsg) {
        case 0x0001 /*WM_CREATE*/:
            if (s_activeWnd->OnCreate(lParam)) {
                return 0;
            }
            break;
        case 0x0003 /*WM_MOVE*/:
            if (s_activeWnd->OnMove((int)(lParam & 0xffff), (int)((unsigned long)lParam >> 16))) {
                return 0;
            }
            break;
        case 0x0005 /*WM_SIZE*/:
            if (s_activeWnd
                    ->OnSize(wParam, (int)(lParam & 0xffff), (int)((unsigned long)lParam >> 16))) {
                return 0;
            }
            break;
        case 0x0002 /*WM_DESTROY*/:
            if (s_activeWnd->QuitMessageLoop()) {
                return 0;
            }
            break;
        case 0x000f /*WM_PAINT*/:
            if (s_activeWnd->OnPaint()) {
                return 0;
            }
            break;
        case 0x0010 /*WM_CLOSE*/:
            if (s_activeWnd->OnClose()) {
                return 0;
            }
            break;
        case 0x001c /*WM_ACTIVATEAPP*/:
            if (s_activeWnd->OnActivateApp(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0102 /*WM_CHAR*/:
            if (s_activeWnd->OnChar(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0100 /*WM_KEYDOWN*/:
            if (s_activeWnd->OnKeyDown(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0101 /*WM_KEYUP*/:
            if (s_activeWnd->OnKeyUp(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0104 /*WM_SYSKEYDOWN*/:
            if (s_activeWnd->OnSysKeyDown(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0111 /*WM_COMMAND*/:
            if (s_activeWnd->OnCommand(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0201 /*WM_LBUTTONDOWN*/:
            if (s_activeWnd->OnLButtonDown(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
        case 0x0202 /*WM_LBUTTONUP*/:
            if (s_activeWnd->OnLButtonUp(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
        case 0x0204 /*WM_RBUTTONDOWN*/:
            if (s_activeWnd->OnRButtonDown(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
        case 0x0205 /*WM_RBUTTONUP*/:
            if (s_activeWnd->OnRButtonUp(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
        case 0x0200 /*WM_MOUSEMOVE*/:
            if (s_activeWnd->OnMouseMove(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
        case 0x0203 /*WM_LBUTTONDBLCLK*/:
            if (s_activeWnd->OnLButtonDblClk(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
        case 0x0206 /*WM_RBUTTONDBLCLK*/:
            if (s_activeWnd->OnRButtonDblClk(
                    wParam,
                    (int)(lParam & 0xffff),
                    (int)((unsigned long)lParam >> 16)
                )) {
                return 0;
            }
            break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Out-of-line stubs so the vftable is emitted in this TU.
// Not matched / not in symbol_names.csv; present only to anchor the vftable
// relocation that the ctor stores (the full 22-slot CGameWnd vtable). The two
// reconstructed virtuals (~CGameWnd, QuitMessageLoop) are defined above.
CGameWnd::~CGameWnd() {}
int CGameWnd::PreDispatchMessage(UINT, WPARAM, LPARAM) {
    return 0;
}
int CGameWnd::Wap32GameWndVfunc2() {
    return 0;
}
int CGameWnd::OnCreate(LPARAM) {
    return 0;
}
int CGameWnd::OnClose() {
    return 0;
}
int CGameWnd::OnMove(int, int) {
    return 0;
}
int CGameWnd::OnSize(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnPaint() {
    return 0;
}
int CGameWnd::OnChar(WPARAM, LPARAM) {
    return 0;
}
int CGameWnd::OnKeyDown(WPARAM, LPARAM) {
    return 0;
}
int CGameWnd::OnKeyUp(WPARAM, LPARAM) {
    return 0;
}
int CGameWnd::OnSysKeyDown(WPARAM, LPARAM) {
    return 0;
}
int CGameWnd::OnActivateApp(WPARAM, LPARAM) {
    return 0;
}
int CGameWnd::OnLButtonDown(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnRButtonDown(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnLButtonUp(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnRButtonUp(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnMouseMove(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnLButtonDblClk(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnRButtonDblClk(WPARAM, int, int) {
    return 0;
}
int CGameWnd::OnCommand(WPARAM, LPARAM) {
    return 0;
}

// size 0x10 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGameWnd, 0x10);
