#include <Mfc.h>
#include <Wap32/Wap32.h>
#include <rva.h>


// CGameWnd::~CGameWnd @0x094c10 - the STANDALONE out-of-line copy of the (inline,
// header-defined) base dtor, referenced only by a /GX EH-unwind funclet (base-subobject
// cleanup on a ctor throw). ??_GCGameWnd above keeps folding its own inline copy; this
// #pragma inline_depth(0) forcer (the UserLogicCtorEmit pattern) emits the out-of-line
// COMDAT here so the unwind reference resolves and the RVA matches.
RVA_COMPGEN(0x00094c10, 0x16, ??1CGameWnd@@UAE@XZ)
// Out-of-line stubs so the vftable is emitted in this TU.
// Not matched / not in symbol_names.csv; present only to anchor the vftable
// relocation that the ctor stores (the full 22-slot CGameWnd vtable). ~CGameWnd
// is inline in Wap32.h (Destroy + clear singleton); QuitMessageLoop is above.
// Scalar-deleting dtor (??_G, slot 0): compiler-generated thunk wrapping the
// inline ~CGameWnd (call Destroy; clear g_activeGameWnd; conditional RezFree).
RVA_COMPGEN(0x00094d80, 0x2f, ??_GCGameWnd@@UAEPAXI@Z)

RVA(0x0013cf00, 0x11)
CGameWnd::CGameWnd() {
    m_hwnd = 0;
    m_closeGuard = 0;
}

DATA(0x00253c68)
CGameWnd* g_activeGameWnd; // 0x653c68

RVA(0x0013cf20, 0x8f)
i32 CGameWnd::CreateAndShow(CREATESTRUCTA* pParams, CGameApp* pOwner) {
    if (!pParams) {
        return reinterpret_cast<i32>(pParams);
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

    ShowWindow(m_hwnd, 1 /*SW_SHOWNORMAL*/);
    return 1;
}

RVA(0x0013cfb0, 0x39)
void CGameWnd::Destroy() {
    if (m_hwnd) {
        if (IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
        }
        m_hwnd = 0;
    }
    m_owner = 0;
    g_activeGameWnd = 0;
}

RVA(0x0013d3a0, 0x6a)
i32 CGameWnd::OnCommand(WPARAM wParam, LPARAM lParam) {
    i32 notifyCode = static_cast<i32>((wParam >> 16));
    i32 cmdId = static_cast<i32>((wParam & 0xffff));
    // Win32 boundary: the raw LOWORD message id becomes a typed GruntzCommand for
    // the two command virtuals (byte-neutral - GruntzCommand is int-width). The
    // slot-2 window hook (Wap32GameWndVfunc2) is a separate vfunc chain that still
    // takes the raw i32 id.
    if (m_owner->HandleCommand(notifyCode, static_cast<GruntzCommand>(cmdId), lParam)) {
        return 1;
    }
    if (Wap32GameWndVfunc2(notifyCode, cmdId, lParam)) {
        return 1;
    }
    return m_owner->m_gameMgr->HandleCommand(notifyCode, static_cast<GruntzCommand>(cmdId), lParam)
           != 0;
}

RVA(0x0013d4c0, 0x1e)
i32 CGameWnd::OnClose() {
    if (!m_closeGuard) {
        m_closeGuard = 1;
        DestroyWindow(m_hwnd);
    }
    return 1;
}

RVA(0x0013d470, 0xd)
i32 CGameWnd::OnActivateApp(WPARAM wParam, LPARAM /*lParam*/) {
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

RVA(0x0013cff0, 0x35c)
LRESULT CALLBACK CGameApp::GameWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CGameWnd* pWnd = g_activeGameWnd;
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
            if (g_activeGameWnd->OnCreate(lParam)) {
                return 0;
            }
            break;
        case 0x0003 /*WM_MOVE*/:
            if (g_activeGameWnd->OnMove(static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0005 /*WM_SIZE*/:
            if (g_activeGameWnd->OnSize(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0002 /*WM_DESTROY*/:
            if (g_activeGameWnd->QuitMessageLoop()) {
                return 0;
            }
            break;
        case 0x000f /*WM_PAINT*/:
            if (g_activeGameWnd->OnPaint()) {
                return 0;
            }
            break;
        case 0x0010 /*WM_CLOSE*/:
            if (g_activeGameWnd->OnClose()) {
                return 0;
            }
            break;
        case 0x001c /*WM_ACTIVATEAPP*/:
            if (g_activeGameWnd->OnActivateApp(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0102 /*WM_CHAR*/:
            if (g_activeGameWnd->OnChar(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0100 /*WM_KEYDOWN*/:
            if (g_activeGameWnd->OnKeyDown(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0101 /*WM_KEYUP*/:
            if (g_activeGameWnd->OnKeyUp(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0104 /*WM_SYSKEYDOWN*/:
            if (g_activeGameWnd->OnSysKeyDown(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0111 /*WM_COMMAND*/:
            if (g_activeGameWnd->OnCommand(wParam, lParam)) {
                return 0;
            }
            break;
        case 0x0201 /*WM_LBUTTONDOWN*/:
            if (g_activeGameWnd
                    ->OnLButtonDown(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0202 /*WM_LBUTTONUP*/:
            if (g_activeGameWnd
                    ->OnLButtonUp(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0204 /*WM_RBUTTONDOWN*/:
            if (g_activeGameWnd
                    ->OnRButtonDown(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0205 /*WM_RBUTTONUP*/:
            if (g_activeGameWnd
                    ->OnRButtonUp(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0200 /*WM_MOUSEMOVE*/:
            if (g_activeGameWnd
                    ->OnMouseMove(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0203 /*WM_LBUTTONDBLCLK*/:
            if (g_activeGameWnd
                    ->OnLButtonDblClk(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
        case 0x0206 /*WM_RBUTTONDBLCLK*/:
            if (g_activeGameWnd
                    ->OnRButtonDblClk(wParam, static_cast<i32>((lParam & 0xffff)), static_cast<i32>((static_cast<u32>(lParam) >> 16)))) {
                return 0;
            }
            break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

i32 CGameWnd::PreDispatchMessage(UINT, WPARAM, LPARAM) {
    return 0;
}

static CGameWnd* volatile g_forceEmitCGameWnd;
#pragma inline_depth(0)
void ForceEmitCGameWndDtor() {
    g_forceEmitCGameWnd->CGameWnd::~CGameWnd();
}
#pragma inline_depth()
i32 CGameWnd::Wap32GameWndVfunc2(i32, i32, i32) {
    return 0;
}
i32 CGameWnd::OnCreate(LPARAM) {
    return 0;
}
i32 CGameWnd::OnMove(i32, i32) {
    return 0;
}
i32 CGameWnd::OnSize(WPARAM, i32, i32) {
    return 0;
}
i32 CGameWnd::OnPaint() {
    return 0;
}
i32 CGameWnd::OnChar(WPARAM, LPARAM) {
    return 0;
}
i32 CGameWnd::OnKeyDown(WPARAM, LPARAM) {
    return 0;
}
RVA(0x00094c80, 0x5)
i32 CGameWnd::OnKeyUp(WPARAM, LPARAM) {
    return 0;
}
i32 CGameWnd::OnSysKeyDown(WPARAM, LPARAM) {
    return 0;
}
i32 CGameWnd::OnLButtonDown(WPARAM, i32, i32) {
    return 0;
}
RVA(0x00094cc0, 0x5)
i32 CGameWnd::OnRButtonDown(WPARAM, i32, i32) {
    return 0;
}
RVA(0x00094ce0, 0x5)
i32 CGameWnd::OnLButtonUp(WPARAM, i32, i32) {
    return 0;
}
RVA(0x00094d00, 0x5)
i32 CGameWnd::OnRButtonUp(WPARAM, i32, i32) {
    return 0;
}
i32 CGameWnd::OnMouseMove(WPARAM, i32, i32) {
    return 0;
}
RVA(0x00094d40, 0x5)
i32 CGameWnd::OnLButtonDblClk(WPARAM, i32, i32) {
    return 0;
}
i32 CGameWnd::OnRButtonDblClk(WPARAM, i32, i32) {
    return 0;
}

VTBL(CGameWnd, 0x001ea344);
