// LobbyDialogs.cpp - the NetMgr multiplayer LOBBY / in-game network DialogProcs and
// their per-dialog WM_INITDIALOG init helpers, re-homed from src/Stub/ApiCallers.cpp.
//
// Two __stdcall DialogProc callbacks drive a network-session dialog: the lobby proc
// (LobbyDlgProc, 0xbdc00) and its in-game sibling (NetGameDlgProc, 0xbe0a0).
// WM_INITDIALOG binds the network object (the current game-state, g_gameReg->m_curState);
// WM_COMMAND ends the dialog on the lobby button IDs; WM_TIMER (0x113) polls the session
// deadline and re-posts the cancel. The remaining functions are the per-dialog init
// helpers (init + arm a 500/750 ms timer + cache the 0x4b6 child control) and the
// chat-edit append/submit helpers.
//
// THE NETWORK OBJECT: g_gameReg->m_curState during a network game is the multiplayer
// game-state CMulti (RTTI CMulti : CPlay : CState, so (view*)m_curState is a proper
// DOWNCAST, not a cross-cast). It exposes the DirectPlay network manager's methods and
// fields (proven by method RVA, sema rva) - matching CNetMgr (<Net/NetMgr.h>) exactly:
//     BroadcastChatLine  0xbb190 (thunk 0x2243)   ?BroadcastChatLine@CNetMgr@@ [netmgr]
//     SendNetStat        0xb9290 (thunk 0x2955)   ?SendNetStat@CNetMgr@@       [netmgr]
//     PollSession        0xb95f0 (thunk 0x2c39)   ?PollSession@CNetMgr@@       [netmgr]
//     m_session          +0x520  == CNetMgr::m_session (CNetSession*)
//       CheckLatency     0xc04a0 (thunk 0x148d)   ?CheckLatency@CNetSession@@  [netcmdsession]
//     m_sessionTerminated +0x52c / m_pollAbort +0x564 / m_584 / m_lastSenderId +0x5c4
// The two ApiCallers view structs (PeerSession/SessionHost) were ONE object - folded to a
// single CNetMgrView here. FULL dissolution onto the real class is deferred (a Net/Gruntz-
// owned follow-up): whether the network methods/fields belong to CNetMgr directly or to
// the CMulti that derives/embeds it is a genuine RE ambiguity - 0xb7e30 (SetStatus, thunk
// 0x1af0) reconstructs as ?ReportVersionMsg@CMulti@@ [multi] while bb190/b9290/b95f0 are
// attributed to CNetMgr - and no TU yet combines <Net/NetMgr.h> or <Gruntz/Multi.h> with
// <Gruntz/GameRegistry.h> (untested header combo).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS, control IDs, and code
// bytes are load-bearing (campaign doctrine).
#include <Mfc.h> // real MFC CString (status/drop banners) + windows.h (dialog API) via afx.h
#include <Ints.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h> // canonical CGameRegistry (g_gameReg->m_curState @ +0x2c)
#include <string.h>              // strcpy/strcat (inline CRT, reloc-masked)
#include <stdio.h>               // sprintf (the drop-in banner)

// --- shared globals (canonical home elsewhere; extern-only pins here) ---
// The CGameRegistry singleton: the lobby DlgProcs read its current game-state
// (m_curState, +0x2c) which - while a network game is open - IS the network manager.
extern CGameRegistry* g_gameReg;
// GetDlgItem(hWnd, 0x4b6) cache (DAT_00648ce0; homed in Globals.cpp), shared by the
// timer wrappers.
extern HWND g_dlgItem_648ce0;
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];
// The "client status" CString global (?g_6473d8@@3VCString@@A; homed in Multi.cpp).
extern CString g_clientStatus;
// CString-data ptr (DAT_00649618): the pending drop-in player's name; its CString
// length lives 8 bytes before the data. Homed in BoundaryLowerThunks.cpp.
extern char* g_playerName_649618;

namespace NetLobby {
    // --- cluster-local globals (DATA home is HERE) ---
    // DAT_0064557c: the active modeless dialog HWND, cached on entry/init.
    DATA(0x0024557c)
    extern HWND g_curDlg_64557c;

    // App-instance chain: this->m_4->m_8->m_c is the HINSTANCE for LoadString.
    struct AppRes_0b7ec0 {
        char m_pad0[0xc];
        HINSTANCE m_c; // +0x0c
    };
    struct AppHolder_0b7ec0 {
        char m_pad0[8];
        AppRes_0b7ec0* m_8; // +0x08
    };
    struct StrHost_0b7ec0 {
        char m_pad0[4];
        AppHolder_0b7ec0* m_4;             // +0x04
        void SetText(char* text, i32 arg); // RVA 0xb7e30 (thunk 0x1af0; owner ambiguous, see top)
        void Load(i32 id, i32 dest);
    };

    // The DirectPlay session sub-object at CNetMgr+0x520 (== CNetMgr::m_session, a
    // CNetSession). CheckLatency (0xc04a0, thunk 0x148d) reports whether any active
    // command slot's latency exceeds the cap.
    struct CNetSessionView {
        i32 CheckLatency(i32 cap); // __thiscall, RVA 0xc04a0 (thunk 0x148d)
    };
    // The network manager the lobby DlgProcs drive (proven CNetMgr, see file header;
    // <Net/NetMgr.h> models this same object as the real class). Field names/offsets
    // adopt the CNetMgr model. Folds the old PeerSession/SessionHost split into one.
    struct CNetMgrView {
        char m_pad0[0x520];
        CNetSessionView* m_session; // +0x520  CNetMgr::m_session (DirectPlay session sub-object)
        char m_pad524[0x52c - 0x524];
        i32 m_sessionTerminated; // +0x52c  "the game session has been terminated" gate
        char m_pad530[0x564 - 0x530];
        i32 m_pollAbort; // +0x564  set => PollSession stops pumping (abnormal-termination gate)
        char m_pad568[0x584 - 0x568];
        i32 m_584; // +0x584  state word cleared on dispatch-handler entry (== normal-exit mark)
        char m_pad588[0x5c4 - 0x588];
        i32 m_lastSenderId; // +0x5c4  sender-id latch (dispatch id 0x402)
        i32
        BroadcastChatLine(char* text, i32 toChat, i32 showWnd, HWND hWnd); // 0xbb190 (thunk 0x2243)
        void SendNetStat(i32 id, u32 value, i32 flag);                     // 0xb9290 (thunk 0x2955)
        i32 PollSession();                                                 // 0xb95f0 (thunk 0x2c39)
        void SetStatus(char* text, i32 flag); // 0xb7e30 (thunk 0x1af0; owner ambiguous, see top)
    };
    DATA(0x002496ac)
    extern CNetMgrView* g_peerSession; // DAT_006496ac (the current network manager)
    DATA(0x002487e0)
    extern char g_sessionFlag; // DAT_006487e0

    // Per-dialog init helpers (cdecl, reached through ILT jmp-thunks).
    void Init_42b4(HWND hWnd, void* ctx);            // RVA 0x42b4
    void Init_1924(HWND hWnd, void* ctx);            // RVA 0x1924
    void Init_bddb0(HWND hWnd, void* ctx);           // RVA 0xbddb0
    void Init_2522(HWND hWnd, void* ctx);            // RVA 0x2522
    void Init_2ed7(HWND hWnd, void* ctx);            // RVA 0x2ed7
    void InitDropPrompt_be3e0(HWND hWnd, void* ctx); // thunk 0x2185 -> 0xbe3e0
    // Lobby DlgProc message helpers (cdecl, reached through ILT jmp-thunks).
    i32 PreHandleLobbyMsg_38c3(HWND, u32, u32, i32); // RVA 0x38c3 -> 0x1192d0
    void OnLobbyInit_2c66(HWND, CNetMgrView*);       // RVA 0x2c66
    void OnLobbyInit_371f(HWND, CNetMgrView*);       // RVA 0x371f
    void OnLobbyTimerA_265d(HWND, CNetMgrView*);     // RVA 0x265d
    void OnLobbyTimerB_154b(HWND, CNetMgrView*);     // RVA 0x154b
    void OnLobbyTimerC_2185(HWND, CNetMgrView*);     // RVA 0x2185 -> 0xbe3e0
    void OnLobbyCancel_2ae0(HWND, CNetMgrView*);     // RVA 0x2ae0

    // __thiscall(id, dest): load string `id`, defaulting to "Error", then push it.
    RVA(0x000b7ec0, 0x7d)
    void StrHost_0b7ec0::Load(i32 id, i32 dest) {
        char buf[0x12a];
        if (m_4 && m_4->m_8->m_c) {
            if (!LoadStringA(m_4->m_8->m_c, id, buf, 0xfa)) {
                strcpy(buf, "Error");
            }
            SetText(buf, dest);
        }
    }

    // __stdcall(edit, str): append `str` to an edit control, prefixing a CRLF when
    // the control is non-empty, then scroll to keep the caret in view.
    RVA(0x000bb3e0, 0xe5)
    void __stdcall AppendEditLine(HWND edit, char* str) {
        if (!edit || !str || !str[0]) {
            return;
        }
        i32 len = GetWindowTextLengthA(edit);
        if (len == 0) {
            SendMessageA(edit, 0xb1, len, -1);
        } else {
            SendMessageA(edit, 0xb1, len, len);
        }
        char buf[0x80];
        buf[0] = 0;
        if (len > 0) {
            strcat(buf, "\r\n");
        }
        strcat(buf, str);
        SendMessageA(edit, 0xc2, 0, (LPARAM)buf);
        SendMessageA(edit, 0xb6, 0, 0x270f);
    }

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache the 0x4b6 child control.
    RVA(0x000bda00, 0x3e)
    void NetDlgInit_bda00(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_42b4(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache the 0x4b6 child control.
    RVA(0x000bdb90, 0x3e)
    void NetDlgInit_bdb90(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_1924(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the network-lobby dialog proc.
    RVA(0x000bdc00, 0x10c)
    i32 CALLBACK LobbyDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (CNetMgrView*)g_gameReg->m_curState;
                OnLobbyInit_2c66(hWnd, g_peerSession);
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
                    OnLobbyCancel_2ae0(hWnd, g_peerSession);
                    return 1;
                }
                break;
            case 0x113:
                OnLobbyTimerA_265d(hWnd, g_peerSession);
                OnLobbyTimerB_154b(hWnd, g_peerSession);
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache the 0x4b6 child control.
    RVA(0x000bdd60, 0x3e)
    void NetDlgInit_bdd60(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bddb0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __cdecl(hWnd, ctx): init, arm a 750 ms timer, cache the 0x4b6 child control.
    RVA(0x000bdfe0, 0x3e)
    void NetDlgInit_bdfe0(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_2522(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the in-game network dialog proc
    // (sibling of the lobby proc at 0xbdc00). WM_COMMAND ends the dialog on a set of
    // button IDs; WM_TIMER (0x113) polls the abort deadline and re-posts the cancel.
    RVA(0x000be0a0, 0x1c7)
    i32 CALLBACK NetGameDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (CNetMgrView*)g_gameReg->m_curState;
                OnLobbyInit_371f(hWnd, g_peerSession);
                return 1;
            case 0x111:
                if (wParam == 0x4ea) {
                    KillTimer(hWnd, 1);
                    g_peerSession->SendNetStat(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4cd) {
                    KillTimer(hWnd, 1);
                    g_peerSession->SendNetStat(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    g_peerSession->SendNetStat(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    OnLobbyCancel_2ae0(hWnd, g_peerSession);
                    return 1;
                }
                break;
            case 0x113:
                if (g_peerSession->m_pollAbort) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, 0x4cd);
                    return 1;
                }
                OnLobbyTimerA_265d(hWnd, g_peerSession);
                OnLobbyTimerC_2185(hWnd, g_peerSession);
                if (g_peerSession->m_session->CheckLatency(0x2710)) {
                    PostMessageA(hWnd, 0x111, 0x4cd, 0);
                }
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): show the "not receiving data" banner, init, arm a timer.
    RVA(0x000be2f0, 0xb9)
    void NetDlgInitDropWait(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            CString banner;
            if (g_clientStatus.GetLength() != 0) {
                banner.Format("Not Receiving Data From Client: %s", (LPCTSTR)g_clientStatus);
                SetDlgItemTextA(hWnd, 0x44b, (LPCTSTR)banner);
            }
            InitDropPrompt_be3e0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __cdecl(hWnd, gate): read the chat-edit text and, if non-empty, broadcast it.
    RVA(0x000be400, 0x6c)
    void NetChatSubmit(HWND hWnd, void* gate) {
        char buf[0x68];
        if (hWnd && gate) {
            HWND edit = GetDlgItem(hWnd, 0x4b7);
            if (edit) {
                if (GetWindowTextA(edit, buf, 0x64) > 0) {
                    g_peerSession->BroadcastChatLine(buf, 1, 1, GetDlgItem(hWnd, 0x4b6));
                    SetWindowTextA(edit, g_emptyString);
                }
            }
        }
    }

    // __cdecl(hWnd, session): poll/stop the session and end the dialog appropriately.
    RVA(0x000be490, 0x84)
    void NetDlgSessionStop(HWND hWnd, CNetMgrView* session) {
        if (hWnd && session) {
            g_sessionFlag = 0;
            session->PollSession();
            if (session->m_584) {
                KillTimer(hWnd, 1);
                EndDialog(hWnd, session->m_lastSenderId);
            } else if (g_peerSession->m_sessionTerminated) {
                KillTimer(hWnd, 1);
                session->SetStatus("The game session has been terminated.", 0);
                EndDialog(hWnd, 0x4ce);
            } else {
                g_sessionFlag = 0;
            }
        }
    }

    // __cdecl(hWnd, ctx): show a drop-in prompt, init, arm a timer, cache a child.
    RVA(0x000be760, 0x82)
    void NetDlgInitDropIn(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];
            if (*(i32*)(g_playerName_649618 - 8)) {
                sprintf(buf, "New Player Drop-In Request: %s", g_playerName_649618);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            Init_2ed7(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }
} // namespace NetLobby
