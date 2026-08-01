#include <Net/LobbyDialogs.h>
#include <Mfc.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Net/NetLobby.h>
#include <EmptyString.h>
#include <Ints.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Multi.h>
#include <Net/NetMgr.h>
#include <Wap32/Wap32.h>
#include <string.h>
#include <stdio.h>

DATA(0x001ea448)
const AFX_MSGMAP CMultiHelpDlg::messageMap = {
    &CDialog::messageMap,
    &CMultiHelpDlg::_messageEntries[0],
};

DATA(0x001ea450)
const AFX_MSGMAP_ENTRY CMultiHelpDlg::_messageEntries[] = {
    {0, 0, 0, 0, AfxSig_end, 0},
};

namespace NetLobby {

    DATA(0x0024557c)
    HWND g_curDlg;

    DATA(0x002487e0)
    char g_sessionFlag;
    DATA(0x002496ac)
    CMulti* g_curMulti;

    DATA(0x00249618)
    CString g_str649618;

    RVA(0x000bd850, 0x141)
    i32 CALLBACK HostWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeHostWaitDialog(hWnd, g_curMulti);
                GetAsyncKeyState(0x13);
                return 1;
            case 0x111:
                if (wParam == 0x4d2 || wParam == 2) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(0x402, 0x4d2, 1);
                    EndDialog(hWnd, 0x4d2);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                if (GetAsyncKeyState(0x13) & 0x80000001) {
                    PostMessageA(hWnd, 0x111, 0x4d2, 0);
                    return 1;
                }
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateHostWaitDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000bda00, 0x3e)
    void InitializeHostWaitDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            UpdateHostWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000bda50, 0x1)
    void UpdateHostWaitDialog(HWND, void*) {}

    RVA(0x000bda70, 0xda)
    i32 CALLBACK JoinWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeJoinWaitDialog(hWnd, g_curMulti);
                return 1;
            case 0x111:
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateJoinWaitDialog(hWnd, g_curMulti);
                if (g_activePlayerCount) {
                    return 1;
                }
                KillTimer(hWnd, 1);
                EndDialog(hWnd, 0x4d2);
                return 1;
        }
        return 0;
    }

    RVA(0x000bdb90, 0x3e)
    void InitializeJoinWaitDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            UpdateJoinWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000bdbe0, 0x1)
    void UpdateJoinWaitDialog(HWND, void*) {}

    RVA(0x000bdc00, 0x10c)
    i32 CALLBACK LobbyDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeLobbyDialog(hWnd, g_curMulti);
                return 1;
            case 0x111:
                if (wParam == 0x4f7) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateLobbyDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000bdd60, 0x3e)
    void InitializeLobbyDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            UpdateLobbyDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000bddb0, 0x1)
    void UpdateLobbyDialog(HWND, void*) {}

    RVA(0x000bddd0, 0x193)
    i32 CALLBACK SessionWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeSessionWaitDialog(hWnd, g_curMulti);
                return 1;
            case 0x111:
                if (wParam == 0x4cc) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(0x402, wParam, 1);
                    }
                    EndDialog(hWnd, 0x4cc);
                    return 1;
                }
                if (wParam == 0x4cd) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(0x402, wParam, 1);
                    }
                    EndDialog(hWnd, 0x4cd);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(0x402, wParam, 1);
                    }
                    EndDialog(hWnd, 0x4ce);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateSessionWaitDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000bdfe0, 0x3e)
    void InitializeSessionWaitDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            UpdateSessionWaitDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000be030, 0x49)
    void UpdateSessionWaitDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, 0x4cc), (static_cast<CMulti*>(ctx))->m_isHost);
            EnableWindow(GetDlgItem(hWnd, 0x4cd), (static_cast<CMulti*>(ctx))->m_isHost);
        }
    }

    RVA(0x000be0a0, 0x1c7)
    i32 CALLBACK NetGameDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeDropWaitDialog(hWnd, g_curMulti);
                return 1;
            case 0x111:
                if (wParam == 0x4ea) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4cd) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    g_curMulti->SendNetStat(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                if (g_curMulti->m_pollAbort) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, 0x4cd);
                    return 1;
                }
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateDropWaitDialog(hWnd, g_curMulti);
                if (g_curMulti->Session()->CheckLatency(0x2710)) {
                    PostMessageA(hWnd, 0x111, 0x4cd, 0);
                }
                return 1;
        }
        return 0;
    }

    RVA(0x000be2f0, 0xb9)
    void InitializeDropWaitDialog(HWND hWnd, void* ctx) {
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
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000be3e0, 0x1)
    void UpdateDropWaitDialog(HWND, void*) {}

    RVA(0x000be400, 0x6c)
    void NetChatSubmit(HWND hWnd, void* gate) {
        char buf[0x68];
        if (hWnd && gate) {
            HWND edit = GetDlgItem(hWnd, 0x4b7);
            if (edit) {
                if (GetWindowTextA(edit, buf, 0x64) > 0) {
                    g_curMulti->BroadcastChatLine(buf, 1, 1, GetDlgItem(hWnd, 0x4b6));
                    SetWindowTextA(edit, g_emptyString);
                }
            }
        }
    }

    RVA(0x000be490, 0x84)
    void NetDlgSessionStop(HWND hWnd, CMulti* session) {
        if (hWnd && session) {
            g_sessionFlag = 0;
            session->PollSession();
            if (session->m_584) {
                KillTimer(hWnd, 1);
                EndDialog(hWnd, session->m_lastSenderId);
            } else if (g_curMulti->m_sessionTerminated) {
                KillTimer(hWnd, 1);
                session->ReportVersionMsg("The game session has been terminated.", 0);
                EndDialog(hWnd, 0x4ce);
            } else {
                g_sessionFlag = 0;
            }
        }
    }

    RVA(0x000be550, 0x193)
    i32 CALLBACK DropInDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg = hWnd;
                g_curMulti = static_cast<CMulti*>(g_gameReg->m_curState);
                InitializeDropInDialog(hWnd, g_curMulti);
                return 1;
            case 0x111:
                if (wParam == 0x4d0) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(0x402, wParam, 1);
                    }
                    EndDialog(hWnd, 0x4d0);
                    return 1;
                }
                if (wParam == 0x4d1) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(0x402, wParam, 1);
                    }
                    EndDialog(hWnd, 0x4d1);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    if (g_curMulti->m_isHost) {
                        g_curMulti->SendNetStat(0x402, wParam, 1);
                    }
                    EndDialog(hWnd, 0x4ce);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                NetDlgSessionStop(hWnd, g_curMulti);
                UpdateDropInDialog(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    RVA(0x000be760, 0x82)
    void InitializeDropInDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];

            const char* pn = g_str649618;
            if (g_str649618.GetLength()) {
                sprintf(buf, "New Player Drop-In Request: %s", pn);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            UpdateDropInDialog(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_sharedFlag = GetDlgItem(hWnd, 0x4b6);
        }
    }

    RVA(0x000be820, 0x49)
    void UpdateDropInDialog(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, 0x4d0), (static_cast<CMulti*>(ctx))->m_isHost);
            EnableWindow(GetDlgItem(hWnd, 0x4d1), (static_cast<CMulti*>(ctx))->m_isHost);
        }
    }
} // namespace NetLobby

RVA(0x000beb60, 0x1e)
CMultiHelpDlg::CMultiHelpDlg(CWnd* pParent) : CDialog(0xcb, pParent) {}

RVA_COMPGEN(0x000beb90, 0x1e, ??_GCMultiHelpDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x000bebc0, 0x5, ??1CMultiHelpDlg@@UAE@XZ)

RVA(0x000bebe0, 0x3)
void CMultiHelpDlg::DoDataExchange(CDataExchange*) {}

RVA(0x000bec00, 0x6)
const AFX_MSGMAP* CMultiHelpDlg::GetMessageMap() const {
    return &messageMap;
}
