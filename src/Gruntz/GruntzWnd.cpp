#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntzWnd.h>

#include <Dsndmgr/MidiManager.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Net/NetLobby.h>
#include <Wap32/Wap32.h>

#include <mmsystem.h>
#include <stddef.h>

CGruntzWnd::CGruntzWnd() {}

CGruntzWnd::~CGruntzWnd() {
    Destroy();
}

i32 CGruntzWnd::CreateAndShow(CREATESTRUCTA* params, CGameApp* owner) {
    return CGameWnd::CreateAndShow(params, owner) != 0;
}

void CGruntzWnd::Destroy() {
    CGameWnd::Destroy();
}

i32 CGruntzWnd::HandleWindowCommand(i32, i32, i32) {
    return 0;
}

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

i32 CGruntzWnd::OnChar(WPARAM charCode, LPARAM keyData) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardCharToState(charCode, keyData);
}

i32 CGruntzWnd::OnKeyDown(WPARAM virtualKey, LPARAM keyData) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardKeyDownToState(virtualKey, keyData);
}

i32 CGruntzWnd::OnKeyUp(WPARAM virtualKey, LPARAM keyData) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardKeyUpToState(virtualKey, keyData);
}

i32 CGruntzWnd::OnLButtonDown(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardLButtonDownToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnLButtonUp(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardLButtonUpToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnMouseMove(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardMouseMoveToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnRButtonDown(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardRButtonDownToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnRButtonUp(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardRButtonUpToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnLButtonDblClk(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardLButtonDblClkToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnRButtonDblClk(WPARAM keyFlags, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->ForwardRButtonDblClkToState(keyFlags, x, y);
}

i32 CGruntzWnd::OnActivateApp(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (mgr && !mgr->IsQuitPending()) {
        mgr->HandleAppActivation(wParam, lParam);
    }
    if (!wParam) {
        while (ShowCursor(true) < 0) {
        }
    }
    return CGameWnd::OnActivateApp(wParam, lParam);
}

i32 CGruntzWnd::OnClose() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->StopAudioPlayback();
    }
    return CGameWnd::OnClose();
}

i32 CGruntzWnd::OnPaint() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr && (mgr->IsQuitPending() || mgr->IsSceneFading() || mgr->IsLobbyHostReady())) {
        if (m_hwnd) {
            ValidateRect(m_hwnd, NULL);
        }
        return 1;
    }
    return 0;
}
