// LobbyDialogs.cpp - the NetMgr multiplayer LOBBY / in-game network DialogProcs and
// their per-dialog WM_INITDIALOG init helpers, re-homed from src/Stub/ApiCallers.cpp.
//
// Two __stdcall DialogProc callbacks drive a network-session dialog: the lobby proc
// (0xbdc00) and its in-game sibling (0xbe0a0). WM_INITDIALOG binds the PeerSession
// singleton (the current game-state, g_gameReg->m_curState); WM_COMMAND ends the
// dialog on the lobby button IDs; WM_TIMER (0x113) polls the session deadline and
// re-posts the cancel. The remaining functions are the per-dialog init helpers (init
// + arm a 500/750 ms timer + cache the 0x4b6 child control) and the chat-edit
// append/submit helpers.
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
// (m_curState, +0x2c) which - while a network game is open - IS the PeerSession.
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
        void SetText(char* text, i32 arg); // RVA 0xb7e30 (thunk 0x1af0)
        void Load(i32 id, i32 dest);
    };

    // The chat/lobby PeerSession singleton at DAT_006496ac. Field names are
    // placeholders; offsets are load-bearing.
    struct TimerHost_148d {
        i32 Poll_148d(i32 elapsed); // __thiscall, RVA 0x148d (nonzero once the deadline passed)
    };
    struct PeerSession_0be490 {
        char m_pad0[0x520];
        TimerHost_148d* m_520; // +0x520
        char m_pad524[0x52c - 0x524];
        i32 m_52c; // +0x52c
        char m_pad530[0x564 - 0x530];
        i32 m_564;                                        // +0x564  abnormal-termination gate
        void Submit(char* text, i32 a, i32 b, HWND ctrl); // thiscall, RVA 0x2243
        void Notify_2955(i32 a, i32 wParam, i32 b);       // thiscall, RVA 0x2955
    };
    DATA(0x002496ac)
    extern PeerSession_0be490* g_peerSession; // DAT_006496ac

    // Session host (arg2 of 0xbe490). Stop() at RVA 0xb95f0; SetStatus(text, flag) at
    // 0xb7e30 (reached via thunk 0x1af0). m_584 marks a normal exit; m_5c4 carries its code.
    struct SessionHost_0be490 {
        char m_pad0[0x584];
        i32 m_584; // +0x584
        char m_pad588[0x5c4 - 0x588];
        i32 m_5c4;                         // +0x5c4
        void Stop();                       // RVA 0xb95f0
        void SetStatus(char* text, i32 f); // RVA 0xb7e30 (thunk 0x1af0)
    };
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
    i32 PreHandleLobbyMsg_38c3(HWND, u32, u32, i32);    // RVA 0x38c3 -> 0x1192d0
    void OnLobbyInit_2c66(HWND, PeerSession_0be490*);   // RVA 0x2c66
    void OnLobbyInit_371f(HWND, PeerSession_0be490*);   // RVA 0x371f
    void OnLobbyTimerA_265d(HWND, PeerSession_0be490*); // RVA 0x265d
    void OnLobbyTimerB_154b(HWND, PeerSession_0be490*); // RVA 0x154b
    void OnLobbyTimerC_2185(HWND, PeerSession_0be490*); // RVA 0x2185 -> 0xbe3e0
    void OnLobbyCancel_2ae0(HWND, PeerSession_0be490*); // RVA 0x2ae0

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
    void __stdcall winapi_0bb3e0_GetWindowTextLengthA(HWND edit, char* str) {
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

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bda00, 0x3e)
    void winapi_0bda00_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_42b4(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bdb90, 0x3e)
    void winapi_0bdb90_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_1924(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the network-lobby dialog proc.
    RVA(0x000bdc00, 0x10c)
    i32 CALLBACK
    winapi_0bdc00_EndDialog_KillTimer(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (PeerSession_0be490*)g_gameReg->m_curState;
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

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bdd60, 0x3e)
    void winapi_0bdd60_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bddb0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __cdecl(hWnd, ctx): init, arm a 750 ms timer, cache a child control handle.
    RVA(0x000bdfe0, 0x3e)
    void winapi_0bdfe0_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
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
    i32 CALLBACK winapi_0be0a0_EndDialog_KillTimer_PostMessageA(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    ) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (PeerSession_0be490*)g_gameReg->m_curState;
                OnLobbyInit_371f(hWnd, g_peerSession);
                return 1;
            case 0x111:
                if (wParam == 0x4ea) {
                    KillTimer(hWnd, 1);
                    g_peerSession->Notify_2955(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4cd) {
                    KillTimer(hWnd, 1);
                    g_peerSession->Notify_2955(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    g_peerSession->Notify_2955(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    OnLobbyCancel_2ae0(hWnd, g_peerSession);
                    return 1;
                }
                break;
            case 0x113:
                if (g_peerSession->m_564) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, 0x4cd);
                    return 1;
                }
                OnLobbyTimerA_265d(hWnd, g_peerSession);
                OnLobbyTimerC_2185(hWnd, g_peerSession);
                if (g_peerSession->m_520->Poll_148d(0x2710)) {
                    PostMessageA(hWnd, 0x111, 0x4cd, 0);
                }
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): show the "not receiving data" banner, init, arm a timer.
    RVA(0x000be2f0, 0xb9)
    void winapi_0be2f0_GetDlgItem_SetDlgItemTextA_SetTimer(HWND hWnd, void* ctx) {
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

    // __cdecl(hWnd, gate): read the chat-edit text and, if non-empty, submit it.
    RVA(0x000be400, 0x6c)
    void winapi_0be400_GetWindowTextA_SetWindowTextA(HWND hWnd, void* gate) {
        char buf[0x68];
        if (hWnd && gate) {
            HWND edit = GetDlgItem(hWnd, 0x4b7);
            if (edit) {
                if (GetWindowTextA(edit, buf, 0x64) > 0) {
                    g_peerSession->Submit(buf, 1, 1, GetDlgItem(hWnd, 0x4b6));
                    SetWindowTextA(edit, g_emptyString);
                }
            }
        }
    }

    // __cdecl(hWnd, session): stop the session and end the dialog appropriately.
    RVA(0x000be490, 0x84)
    void winapi_0be490_EndDialog_KillTimer(HWND hWnd, SessionHost_0be490* session) {
        if (hWnd && session) {
            g_sessionFlag = 0;
            session->Stop();
            if (session->m_584) {
                KillTimer(hWnd, 1);
                EndDialog(hWnd, session->m_5c4);
            } else if (g_peerSession->m_52c) {
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
    void winapi_0be760_GetDlgItem_SetDlgItemTextA_SetTimer(HWND hWnd, void* ctx) {
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
