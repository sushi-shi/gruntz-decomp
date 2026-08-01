#include <Wap32/Wap32.h>
#include <Dsndmgr/GruntzSoundZ.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzWnd.h>
#include <rva.h>
#include <Net/NetLobby.h>

RVA(0x00094640, 0x12)
CGruntzWnd::CGruntzWnd() {}

RVA_COMPGEN(0x00094670, 0x1e, ??_GCGruntzWnd@@UAEPAXI@Z)

RVA(0x000946a0, 0x5f)
CGruntzWnd::~CGruntzWnd() {
    Destroy();
}

RVA(0x00094770, 0x5)
i32 CGruntzWnd::Wap32GameWndVfunc2(i32, i32, i32) {
    return 0;
}

// @early-stop
RVA(0x00094790, 0xcd)
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
                break;
            }
            if (NetLobby::g_curDlg == 0) {
                break;
            }
            ::SendMessageA(NetLobby::g_curDlg, 0x112, wParam, lParam);
            break;
        }
        case 0x3b9: {
            CGruntzMgr* mgr = GameMgr();
            if (mgr == 0) {
                return 1;
            }
            if (mgr->m_sound == 0) {
                return 1;
            }
            EmptyMsgHook(wParam, lParam);
            if (wParam != 1) {
                return 1;
            }
            GameMgr()->RefreshGameClock();
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

i32 CGruntzWnd::Wap32GameWndVfunc0() {
    return 0;
}

VTBL(CGruntzWnd, 0x001ea2d4);
