#include <rva.h>

#include <Net/LobbyDialogs.h>

#include <Mfc.h>

#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Ints.h>
#include <Net/NetLobby.h>
#include <Net/NetLobbyCtrlId.h>
#include <Net/NetMgr.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

namespace NetLobby {

    DATA(0x0024557c)
    HWND g_curDlg;

    DATA(0x002487e0)
    char g_sessionFlag;
    DATA(0x002496ac)
    CMulti* g_curMulti;

    RVA_DYNINIT(0x000bd7d0, 0xa, g_dropInPlayerName)
    RVA_DYNINIT(0x000bd7f0, 0xa, g_dropInPlayerName)
    RVA_DYNINIT(0x000bd810, 0xe, g_dropInPlayerName)
    RVA_DYNINIT(0x000bd830, 0xa, g_dropInPlayerName)
    DATA(0x00249618)
    CString g_dropInPlayerName;

    RVA(0x000bd850, 0x141)
    BOOL CALLBACK HostWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeHostWaitDialog(hWnd, g_curMulti);
                GetAsyncKeyState(VK_PAUSE);
                return 1;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_RESUME) || wParam == IDCANCEL) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, IDX(IDC_NET_RESUME), 1);
                    EndDialog(hWnd, IDX(IDC_NET_RESUME));
                    return 1;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case WM_TIMER:
                if (GetAsyncKeyState(VK_PAUSE) & 0x80000001) {
                    PostMessageA(hWnd, WM_COMMAND, IDX(IDC_NET_RESUME), 0);
                    return 1;
                }
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateHostWaitDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000bda00, 0x3e)
    void InitializeHostWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateHostWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, NULL);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000bda50, 0x1)
    void UpdateHostWaitDialog(HWND, CMulti*) {}

    RVA(0x000bda70, 0xda)
    BOOL CALLBACK JoinWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeJoinWaitDialog(hWnd, g_curMulti);
                return 1;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateJoinWaitDialog(hWnd, g_curMulti);
                if (g_activePlayerCount) {
                    return 1;
                }
                KillTimer(hWnd, 1);
                EndDialog(hWnd, IDX(IDC_NET_RESUME));
                return 1;
        }
        return 0;
    }

    RVA(0x000bdb90, 0x3e)
    void InitializeJoinWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateJoinWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, NULL);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000bdbe0, 0x1)
    void UpdateJoinWaitDialog(HWND, CMulti*) {}

    // @dead-code
    // Zero-ref: retail has no caller or address-taking reference.
    RVA(0x000bdc00, 0x10c)
    BOOL CALLBACK LobbyDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeLobbyDialog(hWnd, g_curMulti);
                return 1;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_LOBBY_LAUNCH)) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateLobbyDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000bdd60, 0x3e)
    void InitializeLobbyDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateLobbyDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, NULL);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000bddb0, 0x1)
    void UpdateLobbyDialog(HWND, CMulti*) {}

    RVA(0x000bddd0, 0x193)
    BOOL CALLBACK SessionWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeSessionWaitDialog(hWnd, g_curMulti);
                return 1;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_RESTART)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    }
                    EndDialog(hWnd, IDX(IDC_NET_RESTART));
                    return 1;
                }
                if (wParam == IDX(IDC_NET_CONTINUE)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    }
                    EndDialog(hWnd, IDX(IDC_NET_CONTINUE));
                    return 1;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    }
                    EndDialog(hWnd, IDX(IDC_NET_ABORT));
                    return 1;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateSessionWaitDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000bdfe0, 0x3e)
    void InitializeSessionWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            UpdateSessionWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, NULL);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000be030, 0x49)
    void UpdateSessionWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_RESTART)), ctx->m_isHost);
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_CONTINUE)), ctx->m_isHost);
        }
    }

    RVA(0x000be0a0, 0x1c7)
    BOOL CALLBACK NetGameDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeDropWaitDialog(hWnd, g_curMulti);
                return 1;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_DROP_PLAYER)) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == IDX(IDC_NET_CONTINUE)) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case WM_TIMER:
                if (g_curMulti->m_pollAbort) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, IDX(IDC_NET_CONTINUE));
                    return 1;
                }
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateDropWaitDialog(hWnd, g_curMulti);
                if (g_curMulti->Session()->AllActiveLatenciesWithin(0x2710)) {
                    PostMessageA(hWnd, WM_COMMAND, IDX(IDC_NET_CONTINUE), 0);
                }
                return 1;
        }
        return 0;
    }

    RVA(0x000be2f0, 0xb9)
    void InitializeDropWaitDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            CString banner;
            if (g_sessionName.GetLength() != 0) {
                banner.Format(
                    "Not Receiving Data From Client: %s",
                    static_cast<LPCTSTR>(g_sessionName)
                );
                SetDlgItemTextA(hWnd, 0x44b, static_cast<LPCTSTR>(banner));
            }
            UpdateDropWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, NULL);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000be3e0, 0x1)
    void UpdateDropWaitDialog(HWND, CMulti*) {}

    RVA(0x000be400, 0x6c)
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

    RVA(0x000be490, 0x84)
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

    // @dead-code
    // Zero-ref: retail has no caller or address-taking reference.
    RVA(0x000be550, 0x193)
    BOOL CALLBACK DropInDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case WM_INITDIALOG:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeDropInDialog(hWnd, g_curMulti);
                return 1;
            case WM_COMMAND:
                if (wParam == IDX(IDC_NET_DROPIN_ACCEPT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    }
                    EndDialog(hWnd, IDX(IDC_NET_DROPIN_ACCEPT));
                    return 1;
                }
                if (wParam == IDX(IDC_NET_DROPIN_REJECT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    }
                    EndDialog(hWnd, IDX(IDC_NET_DROPIN_REJECT));
                    return 1;
                }
                if (wParam == IDX(IDC_NET_ABORT)) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(NETMSG_WAIT_DIALOG_REPLY, wParam, 1);
                    }
                    EndDialog(hWnd, IDX(IDC_NET_ABORT));
                    return 1;
                }
                if (wParam == IDX(IDC_NETCHAT_SEND)) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case WM_TIMER:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateDropInDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000be760, 0x82)
    void InitializeDropInDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];

            const char* pn = g_dropInPlayerName;
            if (g_dropInPlayerName.GetLength()) {
                sprintf(buf, "New Player Drop-In Request: %s", pn);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            UpdateDropInDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, NULL);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000be820, 0x49)
    void UpdateDropInDialog(HWND hWnd, CMulti* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_DROPIN_ACCEPT)), ctx->m_isHost);
            EnableWindow(GetDlgItem(hWnd, IDX(IDC_NET_DROPIN_REJECT)), ctx->m_isHost);
        }
    }
} // namespace NetLobby
