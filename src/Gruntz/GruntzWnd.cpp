#include <Wap32/Wap32.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzWnd.h>
#include <rva.h>

RVA(0x00094640, 0x12)
CGruntzWnd::CGruntzWnd() {}

RVA(0x000946a0, 0x5f)
CGruntzWnd::~CGruntzWnd() {
    Destroy();
}

#include <Net/NetLobby.h> // NetLobby::g_curDlg (0x64557c, active modeless-dialog HWND)
extern void __stdcall Sub_138940(WPARAM, LPARAM); // 0x138940 (empty hook)

// -------------------------------------------------------------------------
// CGruntzWnd::PreDispatchMessage (vtable slot 1). The window's pre-translate hook,
// dispatched on three messages: WM_ERASEBKGND(0x14) is swallowed (1); WM_SYSCOMMAND
// (0x112) blocks the screensaver / monitor-power sys-commands while the window is not
// iconic and re-routes the rest to the active NetLobby dialog; the private 0x3b9 tick
// pumps the manager RefreshGameClock when the game manager + its sound chain are live. All
// other messages fall through as not-handled here (0 = keep dispatching).
// @early-stop
// 73%: complete + correct logic/control-flow (the 3-message compare ladder aligns via
// the switch form). Residual is a codegen-layout wall: (1) `this` lands in edi (retail
// ebx), the msg-compare scheduled after the reg-saves not interleaved; (2) the default /
// case-0x112 `return 0` blocks tail-merge differently (retail runs the 0x3b9 case as the
// ladder fall-through with a shared ret-0, our cl emits the default ret-0 inline + jumps
// into the 0x3b9 body). Not source-steerable (3 spellings + permuter no-change). On top,
// the reloc-masked/plateau operands: the IAT-mirror calls (IsIconic/SendMessageA bare-
// absolute ds slots), the g_curDlg DIR32, the unnamed empty-hook 0x138940 target.
// topic:regalloc topic:tail-merge.
RVA(0x00094790, 0xc2)
i32 CGruntzWnd::PreDispatchMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x14:
            return 1;
        case 0x112: {
            if (wParam == 0xf100) {
                return 1;
            }
            BOOL(WINAPI * isIconic)(HWND) = ::IsIconic;
            i32 mm = wParam & 0xfff0;
            if (mm == 0xf140 || mm == 0xf170) {
                if (!isIconic(m_hwnd)) {
                    return 1;
                }
            }
            if (!isIconic(m_hwnd)) {
                return 0;
            }
            if (NetLobby::g_curDlg == 0) {
                return 0;
            }
            ::SendMessageA(NetLobby::g_curDlg, 0x112, wParam, lParam);
            return 0;
        }
        case 0x3b9: {
            CGruntzMgr* mgr = GameMgr();
            if (mgr == 0) {
                return 1;
            }
            if (mgr->m_sound == 0) {
                return 1;
            }
            Sub_138940(wParam, lParam);
            if (wParam != 1) {
                return 1;
            }
            GameMgr()->RefreshGameClock(); // 0x8f620 direct (thunk 0x3d23)
            return wParam;
        }
    }
    return 0;
}

RVA(0x000948a0, 0x21)
i32 CGruntzWnd::OnChar(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0b(wParam, lParam);
}

RVA(0x000948e0, 0x21)
i32 CGruntzWnd::OnKeyDown(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0c(wParam, lParam);
}

RVA(0x00094920, 0x21)
i32 CGruntzWnd::OnKeyUp(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0d(wParam, lParam);
}

RVA(0x00094960, 0x26)
i32 CGruntzWnd::OnLButtonDown(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0e(keys, x, y);
}

RVA(0x000949a0, 0x26)
i32 CGruntzWnd::OnLButtonUp(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0f(keys, x, y);
}

RVA(0x000949e0, 0x26)
i32 CGruntzWnd::OnMouseMove(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState14(keys, x, y);
}

RVA(0x00094a20, 0x26)
i32 CGruntzWnd::OnRButtonDown(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState11(keys, x, y);
}

RVA(0x00094a60, 0x26)
i32 CGruntzWnd::OnRButtonUp(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState12(keys, x, y);
}

RVA(0x00094aa0, 0x26)
i32 CGruntzWnd::OnLButtonDblClk(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState10(keys, x, y);
}

RVA(0x00094ae0, 0x26)
i32 CGruntzWnd::OnRButtonDblClk(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState13(keys, x, y);
}

RVA(0x00094b20, 0x49)
i32 CGruntzWnd::OnActivateApp(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->AdvanceFrame(wParam, lParam);
    }
    if (!wParam) {
        while (ShowCursor(TRUE) < 0) {
        }
    }
    return CGameWnd::OnActivateApp(wParam, lParam);
}

RVA(0x00094b90, 0x1b)
i32 CGruntzWnd::OnClose() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->UnloadSoundChain();
    }
    return CGameWnd::OnClose();
}

RVA(0x00094bc0, 0x31)
i32 CGruntzWnd::OnPaint() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr && mgr->IsLobbyHostReady()) {
        if (m_hwnd) {
            ValidateRect(m_hwnd, 0);
        }
        return 1;
    }
    return 0;
}

// Out-of-line stubs so the CGruntzWnd vftable is emitted in this TU;
// not matched / not @address-annotated.
i32 CGruntzWnd::Wap32GameWndVfunc0() {
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzWnd::`scalar deleting destructor' (0x094670) - the compiler-generated
// ??_G thunk (call ~CGruntzWnd; if (flags & 1) operator delete(this); return this).
// It has no source body (cl synthesizes it into the emitted vftable), so pin it by
// mangled name; the dtor + delete rel32 calls reloc-mask.
// @rva-symbol: ??_GCGruntzWnd@@UAEPAXI@Z 0x00094670 0x1e

VTBL(CGruntzWnd, 0x001ea2d4); // vtable_names -> code (RTTI game class)
