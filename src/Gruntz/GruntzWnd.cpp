#include <rva.h>

#include <Gruntz/GruntzWnd.h>

#include <Dsndmgr/MidiManager.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Net/NetLobby.h>
#include <Wap32/Wap32.h>

#include <mmsystem.h>
#include <stddef.h>

RVA(0x00094640, 0x12)
CGruntzWnd::CGruntzWnd() {}

RVA_COMPGEN(0x00094670, 0x1e, ??_GCGruntzWnd@@UAEPAXI@Z)

RVA(0x000946a0, 0x5f)
CGruntzWnd::~CGruntzWnd() {
    Destroy();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00094720, 0x18)
i32 CGruntzWnd::CreateAndShow(CREATESTRUCTA* params, CGameApp* owner) {
    return CGameWnd::CreateAndShow(params, owner) != 0;
}

RVA(0x00094750, 0x5)
void CGruntzWnd::Destroy() {
    CGameWnd::Destroy();
}

RVA(0x00094770, 0x5)
i32 CGruntzWnd::HandleWindowCommand(i32, i32, i32) {
    return 0;
}

// @early-stop
RVA(0x00094790, 0xcd)
i32 CGruntzWnd::PreDispatchMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_SYSCOMMAND: {
            if (wParam == SC_KEYMENU) {
                return 1;
            }
            i32 mm = wParam & 0xfff0;
            if (mm == SC_SCREENSAVE || mm == SC_MONITORPOWER) {
                if (!IsIconic(m_hwnd)) {
                    return 1;
                }
            }
            if (!IsIconic(m_hwnd)) {
                break;
            }
            if (NetLobby::g_curDlg == NULL) {
                break;
            }
            SendMessageA(NetLobby::g_curDlg, WM_SYSCOMMAND, wParam, lParam);
            break;
        }
        case MM_MCINOTIFY: {
            CGruntzMgr* mgr = GameMgr();
            if (mgr == NULL) {
                return 1;
            }
            if (mgr->m_midi == NULL) {
                return 1;
            }
            IgnoreMciNotification(wParam, lParam);
            if (wParam != MCI_NOTIFY_SUCCESSFUL) {
                return 1;
            }
            GameMgr()->RefreshGameClock();
            return wParam;
        }
    }
    return 0;
}

RVA(0x000948a0, 0x21)
i32 CGruntzWnd::OnChar(WPARAM charCode, LPARAM keyData) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardCharToState(charCode, keyData);
}

RVA(0x000948e0, 0x21)
i32 CGruntzWnd::OnKeyDown(WPARAM virtualKey, LPARAM keyData) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardKeyDownToState(virtualKey, keyData);
}

RVA(0x00094920, 0x21)
i32 CGruntzWnd::OnKeyUp(WPARAM virtualKey, LPARAM keyData) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardKeyUpToState(virtualKey, keyData);
}

RVA(0x00094960, 0x26)
i32 CGruntzWnd::OnLButtonDown(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardLButtonDownToState(keyFlags, x, y);
}

RVA(0x000949a0, 0x26)
i32 CGruntzWnd::OnLButtonUp(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardLButtonUpToState(keyFlags, x, y);
}

RVA(0x000949e0, 0x26)
i32 CGruntzWnd::OnMouseMove(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardMouseMoveToState(keyFlags, x, y);
}

RVA(0x00094a20, 0x26)
i32 CGruntzWnd::OnRButtonDown(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardRButtonDownToState(keyFlags, x, y);
}

RVA(0x00094a60, 0x26)
i32 CGruntzWnd::OnRButtonUp(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardRButtonUpToState(keyFlags, x, y);
}

RVA(0x00094aa0, 0x26)
i32 CGruntzWnd::OnLButtonDblClk(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardLButtonDblClkToState(keyFlags, x, y);
}

RVA(0x00094ae0, 0x26)
i32 CGruntzWnd::OnRButtonDblClk(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardRButtonDblClkToState(keyFlags, x, y);
}

RVA(0x00094b20, 0x49)
i32 CGruntzWnd::OnActivateApp(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->HandleAppActivation(wParam, lParam);
    }
    if (!wParam) {
        while (ShowCursor(true) < 0) {
        }
    }
    return CGameWnd::OnActivateApp(wParam, lParam);
}

RVA(0x00094b90, 0x1b)
i32 CGruntzWnd::OnClose() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->StopAudioPlayback();
    }
    return CGameWnd::OnClose();
}

RVA(0x00094bc0, 0x31)
i32 CGruntzWnd::OnPaint() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr && mgr->IsLobbyHostReady()) {
        if (m_hwnd) {
            ValidateRect(m_hwnd, NULL);
        }
        return 1;
    }
    return 0;
}

RVA_COMPGEN(0x00094c10, 0x16, ??1CGameWnd@@UAE@XZ)
RVA_COMPGEN(0x00094d80, 0x2f, ??_GCGameWnd@@UAEPAXI@Z)
