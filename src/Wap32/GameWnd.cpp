#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GruntzCommandId.h>
#include <Wap32/Wap32.h>

DATA(0x00253c68)
CGameWnd* g_activeGameWnd;

// @identity-TODO ?1CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA(0x0013cf00, 0x11)
CGameWnd::CGameWnd() {
    m_hwnd = NULL;
    m_closeGuard = 0;
}

RVA(0x0013cf20, 0x8f)
i32 CGameWnd::CreateAndShow(CREATESTRUCTA* pParams, CGameApp* pOwner) {
    if (!pParams) {
        return 0;
    }
    if (!pOwner) {
        return 0;
    }
    if (g_activeGameWnd) {
        return 0;
    }

    m_owner = pOwner;
    g_activeGameWnd = this;
    m_closeGuard = 0;

    m_hwnd = CreateWindowExA(
        pParams->dwExStyle,
        pParams->lpszClass,
        pParams->lpszName,
        pParams->style,
        pParams->x,
        pParams->y,
        pParams->cx,
        pParams->cy,
        pParams->hwndParent,
        pParams->hMenu,
        pParams->hInstance,
        pParams->lpCreateParams
    );
    if (!m_hwnd) {
        return 0;
    }

    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    return 1;
}

RVA(0x0013cfb0, 0x39)
void CGameWnd::Destroy() {
    if (m_hwnd) {
        if (IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
        }
        m_hwnd = NULL;
    }
    m_owner = NULL;
    g_activeGameWnd = NULL;
}

RVA(0x0013cff0, 0x3a0)
LRESULT CALLBACK CGameApp::GameWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CGameWnd* pWnd = g_activeGameWnd;
    if (!pWnd) {
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    if (pWnd->PreDispatchMessage(uMsg, wParam, lParam)) {
        return 0;
    }

    switch (uMsg) {
        case WM_CREATE:
            if (g_activeGameWnd->OnCreate(lParam)) {
                return 0;
            }
            break;
        case WM_MOVE:
            if (g_activeGameWnd->OnMove(
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_SIZE:
            if (g_activeGameWnd->OnSize(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_DESTROY:
            if (g_activeGameWnd->QuitMessageLoop()) {
                return 0;
            }
            break;
        case WM_PAINT:
            if (g_activeGameWnd->OnPaint()) {
                return 0;
            }
            break;
        case WM_CLOSE:
            if (g_activeGameWnd->OnClose()) {
                return 0;
            }
            break;
        case WM_ACTIVATEAPP:
            if (g_activeGameWnd->OnActivateApp(wParam, lParam)) {
                return 0;
            }
            break;
        case WM_CHAR:
            if (g_activeGameWnd->OnChar(wParam, lParam)) {
                return 0;
            }
            break;
        case WM_KEYDOWN:
            if (g_activeGameWnd->OnKeyDown(wParam, lParam)) {
                return 0;
            }
            break;
        case WM_KEYUP:
            if (g_activeGameWnd->OnKeyUp(wParam, lParam)) {
                return 0;
            }
            break;
        case WM_SYSKEYDOWN:
            if (g_activeGameWnd->OnSysKeyDown(wParam, lParam)) {
                return 0;
            }
            break;
        case WM_COMMAND:
            if (g_activeGameWnd->OnCommand(wParam, lParam)) {
                return 0;
            }
            break;
        case WM_LBUTTONDOWN:
            if (g_activeGameWnd->OnLButtonDown(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (g_activeGameWnd->OnLButtonUp(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_RBUTTONDOWN:
            if (g_activeGameWnd->OnRButtonDown(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_RBUTTONUP:
            if (g_activeGameWnd->OnRButtonUp(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_MOUSEMOVE:
            if (g_activeGameWnd->OnMouseMove(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_LBUTTONDBLCLK:
            if (g_activeGameWnd->OnLButtonDblClk(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
        case WM_RBUTTONDBLCLK:
            if (g_activeGameWnd->OnRButtonDblClk(
                    wParam,
                    static_cast<i32>((lParam & 0xffff)),
                    static_cast<i32>((static_cast<u32>(lParam) >> 16))
                )) {
                return 0;
            }
            break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

RVA(0x0013d390, 0x5)
i32 CGameWnd::OnCreate(LPARAM) {
    return 0;
}

RVA(0x0013d3a0, 0x6a)
i32 CGameWnd::OnCommand(WPARAM wParam, LPARAM lParam) {
    i32 notifyCode = static_cast<i32>((wParam >> 16));
    i32 cmdId = static_cast<i32>((wParam & 0xffff));

    if (m_owner->HandleCommand(notifyCode, static_cast<GruntzCommandId>(cmdId), lParam)) {
        return 1;
    }
    if (HandleWindowCommand(notifyCode, cmdId, lParam)) {
        return 1;
    }
    return m_owner->m_gameMgr
               ->HandleCommand(notifyCode, static_cast<GruntzCommandId>(cmdId), lParam)
           != 0;
}

RVA(0x0013d410, 0x5)
i32 CGameWnd::OnMove(i32, i32) {
    return 0;
}

RVA(0x0013d420, 0x5)
i32 CGameWnd::OnSize(WPARAM, i32, i32) {
    return 0;
}

RVA(0x0013d430, 0x3)
i32 CGameWnd::OnPaint() {
    return 0;
}

RVA(0x0013d440, 0x5)
i32 CGameWnd::OnChar(WPARAM, LPARAM) {
    return 0;
}

RVA(0x0013d450, 0x5)
i32 CGameWnd::OnKeyDown(WPARAM, LPARAM) {
    return 0;
}

RVA(0x0013d460, 0x5)
i32 CGameWnd::OnSysKeyDown(WPARAM, LPARAM) {
    return 0;
}

RVA(0x0013d470, 0x12)
i32 CGameWnd::OnActivateApp(WPARAM wParam, LPARAM) {
    m_owner->m_appActive = wParam;
    return 0;
}

RVA(0x0013d490, 0x29)
i32 CGameWnd::QuitMessageLoop() {
    m_owner->FreeGameManager();
    if (m_owner->m_errorReported) {
        m_owner->ShowError();
    }
    PostQuitMessage(0);
    return 0;
}

RVA(0x0013d4c0, 0x1e)
i32 CGameWnd::OnClose() {
    if (!m_closeGuard) {
        m_closeGuard = 1;
        DestroyWindow(m_hwnd);
    }
    return 1;
}

RVA(0x0013d4e0, 0x43)
void CGameWnd::PumpMessages(u32 filterMsg, i32 count) {
    MSG msg;
    for (i32 i = 0; i < count; ++i) {
        if (!PeekMessageA(&msg, m_hwnd, filterMsg, filterMsg, PM_REMOVE)) {
            break;
        }
    }
}

RVA(0x0013d530, 0x55)
void CGameWnd::PumpMessagesRange(u32 filterMin, u32 filterMax, i32 count) {
    MSG msg;
    for (i32 i = 0; i < count; ++i) {
        if (!PeekMessageA(&msg, m_hwnd, filterMin, filterMax, PM_REMOVE)) {
            break;
        }
    }
}
