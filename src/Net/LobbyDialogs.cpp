#include <StdAfx.h>

#include <Ints.h>

#include <Net/LobbyDialogs.h>

#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Utils.h>
#include <Ints.h>
#include <Net/NetLobby.h>
#include <Net/NetLobbyCtrlId.h>
#include <Net/NetMgr.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

namespace NetLobby {

    HWND g_curDlg;

    char g_sessionFlag;

    CMulti* g_curMulti;

    std::string g_dropInPlayerName;

    BOOL CALLBACK HostWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return true;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeHostWaitDialog(hWnd, g_curMulti);
                GetAsyncKeyState(VK_PAUSE);
                return true;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_RESUME) || wParam == IDCANCEL) {
                    KillTimer(hWnd, 1);
                    g_curMulti->BroadcastValueMessage(
                        NETMSG_WAIT_DIALOG_REPLY,
                        IDX(IDC_NET_RESUME),
                        DPSEND_GUARANTEED
                    );
                    EndDialog(hWnd, IDX(IDC_NET_RESUME));
                    return true;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return true;
                }
                break;
            case WM_TIMER:
                if (GetAsyncKeyState(VK_PAUSE) & 0x80000001) {
                    PostMessageA(hWnd, WM_COMMAND, IDX(IDC_NET_RESUME), 0);
                    return true;
                }
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateHostWaitDialog(hWnd, g_curMulti);
                return true;
        }
        return false;
    }

    void InitializeHostWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateHostWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, NULL);
            g_netMessageEditHwnd = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void UpdateHostWaitDialog(HWND, CMulti*) {}

    BOOL CALLBACK JoinWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return true;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeJoinWaitDialog(hWnd, g_curMulti);
                return true;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return true;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateJoinWaitDialog(hWnd, g_curMulti);
                if (g_playersInOptionsCount) {
                    return true;
                }
                KillTimer(hWnd, 1);
                EndDialog(hWnd, IDX(IDC_NET_RESUME));
                return true;
        }
        return false;
    }

    void InitializeJoinWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateJoinWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, NULL);
            g_netMessageEditHwnd = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void UpdateJoinWaitDialog(HWND, CMulti*) {}

    BOOL CALLBACK LobbyDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return true;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeLobbyDialog(hWnd, g_curMulti);
                return true;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_LOBBY_LAUNCH)) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return true;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return true;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return true;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateLobbyDialog(hWnd, g_curMulti);
                return true;
        }
        return false;
    }

    void InitializeLobbyDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateLobbyDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, NULL);
            g_netMessageEditHwnd = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void UpdateLobbyDialog(HWND, CMulti*) {}

    BOOL CALLBACK SessionWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return true;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeSessionWaitDialog(hWnd, g_curMulti);
                return true;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_RESTART)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->BroadcastValueMessage(
                            NETMSG_WAIT_DIALOG_REPLY,
                            wParam,
                            DPSEND_GUARANTEED
                        );
                    }
                    EndDialog(hWnd, IDX(IDC_NET_RESTART));
                    return true;
                }
                if (wParam == IDX(IDC_NET_CONTINUE)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->BroadcastValueMessage(
                            NETMSG_WAIT_DIALOG_REPLY,
                            wParam,
                            DPSEND_GUARANTEED
                        );
                    }
                    EndDialog(hWnd, IDX(IDC_NET_CONTINUE));
                    return true;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->BroadcastValueMessage(
                            NETMSG_WAIT_DIALOG_REPLY,
                            wParam,
                            DPSEND_GUARANTEED
                        );
                    }
                    EndDialog(hWnd, IDX(IDC_NET_ABORT));
                    return true;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return true;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateSessionWaitDialog(hWnd, g_curMulti);
                return true;
        }
        return false;
    }

    void InitializeSessionWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateSessionWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, NULL);
            g_netMessageEditHwnd = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void UpdateSessionWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_RESTART)), ctx->m_isHost);
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_CONTINUE)), ctx->m_isHost);
        }
    }

    BOOL CALLBACK NetGameDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return true;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeDropWaitDialog(hWnd, g_curMulti);
                return true;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_DROP_PLAYER)) {
                    KillTimer(hWnd, 1);
                    g_curMulti->BroadcastValueMessage(
                        NETMSG_WAIT_DIALOG_REPLY,
                        wParam,
                        DPSEND_GUARANTEED
                    );
                    EndDialog(hWnd, wParam);
                    return true;
                }
                if (wParam == IDX(IDC_NET_CONTINUE)) {
                    KillTimer(hWnd, 1);
                    g_curMulti->BroadcastValueMessage(
                        NETMSG_WAIT_DIALOG_REPLY,
                        wParam,
                        DPSEND_GUARANTEED
                    );
                    EndDialog(hWnd, wParam);
                    return true;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    g_curMulti->BroadcastValueMessage(
                        NETMSG_WAIT_DIALOG_REPLY,
                        wParam,
                        DPSEND_GUARANTEED
                    );
                    EndDialog(hWnd, wParam);
                    return true;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return true;
                }
                break;
            case WM_TIMER:
                if (g_curMulti->m_pollAbort) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, IDX(IDC_NET_CONTINUE));
                    return true;
                }
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateDropWaitDialog(hWnd, g_curMulti);
                if (g_curMulti->Session()->AllActiveLatenciesWithin(0x2710)) {
                    PostMessageA(hWnd, WM_COMMAND, IDX(IDC_NET_CONTINUE), 0);
                }
                return true;
        }
        return false;
    }

    void InitializeDropWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            std::string banner;
            if (!(g_sessionName).empty()) {
                banner = formatText("Not Receiving Data From Client: %s", (g_sessionName).c_str());
                SetDlgItemTextA(hWnd, 0x44b, (banner).c_str());
            }
            UpdateDropWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, NULL);
            g_netMessageEditHwnd = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void UpdateDropWaitDialog(HWND, CMulti*) {}

    void NetChatSubmit(HWND hWnd, CMulti* gate) {
        char buf[0x68];
        if (hWnd && gate) {
            HWND edit = GetDlgItem(hWnd, 0x4b7);
            if (edit) {
                if (GetWindowTextA(edit, buf, 0x64) > 0) {
                    g_curMulti->BroadcastChatLine(buf, 1, 1, GetDlgItem(hWnd, 0x4b6));
                    SetWindowTextA(edit, "");
                }
            }
        }
    }

    void NetDlgSessionStop(HWND hWnd, CMulti* session) {
        if (hWnd && session) {
            g_sessionFlag = 0;
            session->PollSession();
            if (session->m_waitDialogReplyReceived) {
                KillTimer(hWnd, 1);
                EndDialog(hWnd, session->m_lastSenderId);
            } else if (g_curMulti->m_sessionTerminated) {
                KillTimer(hWnd, 1);
                session->ReportVersionMsg("The game session has been terminated", 0);
                EndDialog(hWnd, IDX(IDC_NET_ABORT));
            } else {
                g_sessionFlag = 0;
            }
        }
    }

    BOOL CALLBACK DropInDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return true;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeDropInDialog(hWnd, g_curMulti);
                return true;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_DROPIN_ACCEPT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->BroadcastValueMessage(
                            NETMSG_WAIT_DIALOG_REPLY,
                            wParam,
                            DPSEND_GUARANTEED
                        );
                    }
                    EndDialog(hWnd, IDX(IDC_NET_DROPIN_ACCEPT));
                    return true;
                }
                if (wParam == IDX(IDC_NET_DROPIN_REJECT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->BroadcastValueMessage(
                            NETMSG_WAIT_DIALOG_REPLY,
                            wParam,
                            DPSEND_GUARANTEED
                        );
                    }
                    EndDialog(hWnd, IDX(IDC_NET_DROPIN_REJECT));
                    return true;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->BroadcastValueMessage(
                            NETMSG_WAIT_DIALOG_REPLY,
                            wParam,
                            DPSEND_GUARANTEED
                        );
                    }
                    EndDialog(hWnd, IDX(IDC_NET_ABORT));
                    return true;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return true;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateDropInDialog(hWnd, g_curMulti);
                return true;
        }
        return false;
    }

    void InitializeDropInDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];

            const char* pn = (g_dropInPlayerName).c_str();
            if (static_cast<i32>((g_dropInPlayerName).size())) {
                sprintf(buf, "New Player Drop-In Request: %s", pn);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            UpdateDropInDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, NULL);
            g_netMessageEditHwnd = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void UpdateDropInDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_DROPIN_ACCEPT)), ctx->m_isHost);
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_DROPIN_REJECT)), ctx->m_isHost);
        }
    }
}
