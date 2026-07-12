// LobbyDialogs.cpp - the multiplayer LOBBY / in-game network DialogProcs and
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
// THE NETWORK OBJECT - RESOLVED (netmgr-vs-cmulti, full proof in <Net/NetMgr.h>):
// g_gameReg->m_curState during a network game is the multiplayer game-state CMulti
// (RTTI CMulti : CPlay : CState), and CMulti itself owns the +0x2d8..+0x60c network
// field block and the 0xb5xxx-0xbdxxx method cluster these DlgProcs drive
// (PollSession 0xb95f0 / SendNetStat 0xb9290 / BroadcastChatLine 0xbb190 /
// ReportVersionMsg 0xb7e30 / ReportStatusId 0xb7ec0, defined below in its RVA-order
// home). The former TU-local CNetMgrView/CNetSessionView/StrHost_0b7ec0 views are
// dissolved onto the canonical <Gruntz/Multi.h> CMulti / <Net/NetMgr.h> CNetSession. The real
// CNetMgr (the small DirectPlay wrapper, ??1 @0xb6000) lives at CMulti+0x524 and is
// not touched here.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS, control IDs, and code
// bytes are load-bearing (campaign doctrine).
#include <Mfc.h> // real MFC CString (status/drop banners) + windows.h (dialog API) via afx.h
#include <Ints.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h> // canonical CGameRegistry (g_gameReg->m_curState @ +0x2c)
#include <Gruntz/Multi.h>        // canonical CMulti (the network game-state; m_session is CNetSession)
#include <Net/NetMgr.h> // canonical CNetSession (m_session command-slot facet; CheckLatency @0xc04a0)
#include <Wap32/Wap32.h> // CGameApp (m_logic->m_owner->m_hInstance, ReportStatusId)
#include <string.h>      // strcpy/strcat (inline CRT, reloc-masked)
#include <stdio.h>       // sprintf (the drop-in banner)

// --- shared globals (canonical home elsewhere; extern-only pins here) ---
// The CGameRegistry singleton: the lobby DlgProcs read its current game-state
// (m_curState, +0x2c) which - while a network game is open - IS the CMulti.
extern "C" CGameRegistry* g_gameReg;
// GetDlgItem(hWnd, 0x4b6) cache (0x248ce0; the modeless dialog's cached child HWND,
// stored raw as i32), shared by the timer wrappers. Bound to the DATA home name
// (extern "C" g_sharedFlag, DATA @0x248ce0 in Multi.cpp) so the store relocs.
extern "C" i32 g_sharedFlag;
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];
// The DirectPlay session/client-status CString global (0x6473d8; canonical home
// g_6473d8 in Multi.cpp). Referenced by its home name so the read relocs.
extern CString g_6473d8;

// BlockScreenSaver (0x1192d0, GLOBAL scope; home Utils/TimeSplit.cpp): the shared
// WM_SYSCOMMAND screensaver/monitor-power swallow each lobby DlgProc runs first. 4-arg
// (lParam unused) so every DlgProc's push count matches. Declared at file scope (NOT
// inside NetLobby) so it mangles to the global ?BlockScreenSaver@@ TimeSplit binds. First
// param is void* (not HWND): TimeSplit compiles under the non-STRICT <Win32.h> where HWND
// is void* (PAX), so the mangled name is ?BlockScreenSaver@@YAHPAXIIJ@Z - this TU's STRICT
// <Mfc.h> HWND (HWND__*, PAU) would mis-mangle. The HWND args convert to void* implicitly.
i32 BlockScreenSaver(void*, UINT, WPARAM, LPARAM);
// DAT_00648cec: the "connection established / abort" latch the join-wait timer polls
// (nonzero = keep waiting, skip the timeout EndDialog). Home elsewhere; extern-only pin.
extern "C" i32 g_648cec;

namespace NetLobby {
    // --- cluster-local globals (DATA home is HERE) ---
    // DAT_0064557c: the active modeless dialog HWND, cached on entry/init.
    DATA(0x0024557c)
    extern HWND g_curDlg_64557c; // DAT_0064557c (shared; homed elsewhere - not TU-private)

    // Owner-TU DEFINITIONS (private to this dialog TU), ascending by RVA (.bss/zero).
    DATA(0x002487e0)
    char g_sessionFlag; // 0x2487e0
    DATA(0x002496ac)
    CMulti* g_curMulti; // 0x2496ac  the current multiplayer game-state

    // Per-dialog init/timer hooks: empty 1-byte `ret` helpers defined below in this
    // TU in RVA order (were the unbound Init_42b4/Init_1924/InitDropPrompt/OnLobbyTimerB/
    // OnLobbyTimerC fake decls that reloc-masked instead of binding). Each RVA is
    // reached by both a WM_INITDIALOG and a WM_TIMER path, so both call sites bind to
    // the one empty function at that RVA.
    void Init_bda50(HWND hWnd, void* ctx); // 0xbda50 (host-wait init/timer hook)
    void Init_bdbe0(HWND hWnd, void* ctx); // 0xbdbe0 (join-wait init/timer hook)
    void Init_bddb0(HWND hWnd, void* ctx); // 0xbddb0 (lobby init/timer hook)
    void Init_be3e0(HWND hWnd, void* ctx); // 0xbe3e0 (drop-wait init/timer hook)
    void Init_2522(HWND hWnd, void* ctx);  // 0xbe030 (session-wait button enable)
    void Init_2ed7(HWND hWnd, void* ctx);  // 0xbe820 (drop-in button enable)
    // These four ARE the real dialog helpers defined later in this TU (reloc-masked
    // calls now bind to the correct RVAs); forward-declared so the early DlgProcs
    // above their definitions can call them.
    void NetDlgInit_bdd60(HWND, void*);    // 0xbdd60 (ex OnLobbyInit_2c66)
    void NetDlgInitDropWait(HWND, void*);  // 0xbe2f0 (ex OnLobbyInit_371f)
    void NetDlgSessionStop(HWND, CMulti*); // 0xbe490 (ex OnLobbyTimerA_265d)
    void NetChatSubmit(HWND, void*);       // 0xbe400 (ex OnLobbyCancel_2ae0)
    // WM_INITDIALOG init helpers defined later in this TU (forward-declared so the
    // sibling wait/drop DlgProcs below can call them before their definitions).
    void NetDlgInit_bda00(HWND hWnd, void* ctx); // 0xbda00
    void NetDlgInit_bdb90(HWND hWnd, void* ctx); // 0xbdb90
    void NetDlgInit_bdfe0(HWND hWnd, void* ctx); // 0xbdfe0
    void NetDlgInitDropIn(HWND hWnd, void* ctx); // 0xbe760
} // namespace NetLobby
// CMulti::ReportStatusId (0xb7ec0) + NetLobby::AppendEditLine (0xbb3e0) live in
// their home TU per the interval dossier (#4b): src/Gruntz/Multi.cpp.

namespace NetLobby {

    // ---------------------------------------------------------------------------
    // 0x0bd7f0 (RVA-homed from src/Stub/BoundaryLowerThunks.cpp) - the compiler-
    // generated dynamic initializer for the pending drop-in player-name CString
    // g_str649618: tail-construct it empty in place (CString::CString @0x1b9b93, the
    // NAFXCW default ctor - reloc-masked). NetDlgInitDropIn below reads the pending
    // drop-in name through this CString's LPCTSTR (m_pszData, the char* at 0x249618) -
    // one DATA home, no separate g_playerName_* char* alias.
    DATA(0x00249618)
    extern CString g_str649618;
    RVA(0x000bd7f0, 0xa)
    void InitPlayerNameStr() {
        g_str649618.CString::CString();
    }

    // __stdcall DlgProc: the host-wait dialog. WM_TIMER polls the PAUSE key
    // (GetAsyncKeyState(VK_PAUSE) & down|pressed) and re-posts the 0x4d2 abort;
    // WM_COMMAND ends on 0x4d2 / 2 (pushing the abort stat) or cancels on 0x4c6.
    RVA(0x000bd850, 0x141)
    i32 CALLBACK HostWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_curMulti = (CMulti*)g_gameReg->m_curState;
                NetDlgInit_bda00(hWnd, g_curMulti);
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
                Init_bda50(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache the 0x4b6 child control.
    RVA(0x000bda00, 0x3e)
    void NetDlgInit_bda00(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bda50(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_sharedFlag = (i32)GetDlgItem(hWnd, 0x4b6);
        }
    }

    // 0xbda50 - empty per-dialog init/timer hook (host-wait), reached from both the
    // WM_INITDIALOG (NetDlgInit_bda00) and WM_TIMER (HostWaitDlgProc) paths. Compiles
    // to a bare `ret`; kept so both call sites bind their reloc to this RVA.
    RVA(0x000bda50, 0x1)
    void Init_bda50(HWND, void*) {}

    // __stdcall DlgProc: the join-wait dialog. WM_TIMER services the session then, if
    // the connection latch (g_648cec) has cleared, kills the timer and ends with 0x4d2;
    // WM_COMMAND cancels on 0x4c6.
    RVA(0x000bda70, 0xda)
    i32 CALLBACK JoinWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_curMulti = (CMulti*)g_gameReg->m_curState;
                NetDlgInit_bdb90(hWnd, g_curMulti);
                return 1;
            case 0x111:
                if (wParam == 0x4c6) {
                    NetChatSubmit(hWnd, g_curMulti);
                    return 1;
                }
                break;
            case 0x113:
                NetDlgSessionStop(hWnd, g_curMulti);
                Init_bdbe0(hWnd, g_curMulti);
                if (g_648cec) {
                    return 1;
                }
                KillTimer(hWnd, 1);
                EndDialog(hWnd, 0x4d2);
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache the 0x4b6 child control.
    RVA(0x000bdb90, 0x3e)
    void NetDlgInit_bdb90(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bdbe0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_sharedFlag = (i32)GetDlgItem(hWnd, 0x4b6);
        }
    }

    // 0xbdbe0 - empty per-dialog init/timer hook (join-wait), reached from both
    // NetDlgInit_bdb90 (init) and JoinWaitDlgProc (timer). Bare `ret`.
    RVA(0x000bdbe0, 0x1)
    void Init_bdbe0(HWND, void*) {}

    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the network-lobby dialog proc.
    RVA(0x000bdc00, 0x10c)
    i32 CALLBACK LobbyDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_curMulti = (CMulti*)g_gameReg->m_curState;
                NetDlgInit_bdd60(hWnd, g_curMulti);
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
                Init_bddb0(hWnd, g_curMulti);
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
            g_sharedFlag = (i32)GetDlgItem(hWnd, 0x4b6);
        }
    }

    // 0xbddb0 - empty per-dialog init/timer hook (lobby), reached from both
    // NetDlgInit_bdd60 (init) and LobbyDlgProc (timer). Bare `ret`.
    RVA(0x000bddb0, 0x1)
    void Init_bddb0(HWND, void*) {}

    // __stdcall DlgProc: an in-game session-wait dialog (sibling of NetGameDlgProc).
    // WM_COMMAND ends on 0x4cc/0x4cd/0x4ce - each kills the timer, and (when host)
    // ships the command as a 0x402 stat before EndDialog - or cancels on 0x4c6.
    RVA(0x000bddd0, 0x193)
    i32 CALLBACK SessionWaitDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_curMulti = (CMulti*)g_gameReg->m_curState;
                NetDlgInit_bdfe0(hWnd, g_curMulti);
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
                Init_2522(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): init, arm a 750 ms timer, cache the 0x4b6 child control.
    RVA(0x000bdfe0, 0x3e)
    void NetDlgInit_bdfe0(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_2522(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_sharedFlag = (i32)GetDlgItem(hWnd, 0x4b6);
        }
    }

    // Init_2522 (0xbe030, reached via ILT thunk 0x2522 - the NetDlgInit_bdfe0 call
    // above): enable/disable the accept (0x4cc) and reject (0x4cd) buttons per the
    // game-state's host flag. ctx is the current CMulti. __cdecl. Re-homed from the
    // AppHelpers.cpp holding TU (was the Unmatched_be030 / DlgData view; the +0x528
    // enable flag IS CMulti::m_isHost - same access as the Init_2ed7 sibling below).
    RVA(0x000be030, 0x49)
    void Init_2522(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, 0x4cc), ((CMulti*)ctx)->m_isHost);
            EnableWindow(GetDlgItem(hWnd, 0x4cd), ((CMulti*)ctx)->m_isHost);
        }
    }

    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the in-game network dialog proc
    // (sibling of the lobby proc at 0xbdc00). WM_COMMAND ends the dialog on a set of
    // button IDs; WM_TIMER (0x113) polls the abort deadline and re-posts the cancel.
    RVA(0x000be0a0, 0x1c7)
    i32 CALLBACK NetGameDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_curMulti = (CMulti*)g_gameReg->m_curState;
                NetDlgInitDropWait(hWnd, g_curMulti);
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
                Init_be3e0(hWnd, g_curMulti);
                if (g_curMulti->Session()->CheckLatency(
                        0x2710
                    )) { // CNetSession::CheckLatency @0xc04a0
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
            if (g_6473d8.GetLength() != 0) {
                banner.Format("Not Receiving Data From Client: %s", (LPCTSTR)g_6473d8);
                SetDlgItemTextA(hWnd, 0x44b, (LPCTSTR)banner);
            }
            Init_be3e0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_sharedFlag = (i32)GetDlgItem(hWnd, 0x4b6);
        }
    }

    // 0xbe3e0 - empty per-dialog init/timer hook (drop-wait/in-game), reached from both
    // NetDlgInitDropWait (init) and NetGameDlgProc (timer). Bare `ret`.
    RVA(0x000be3e0, 0x1)
    void Init_be3e0(HWND, void*) {}

    // __cdecl(hWnd, gate): read the chat-edit text and, if non-empty, broadcast it.
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

    // __cdecl(hWnd, session): poll/stop the session and end the dialog appropriately.
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

    // __stdcall DlgProc: the drop-in-request dialog (WM_INITDIALOG shows the drop-in
    // prompt). WM_COMMAND ends on 0x4d0/0x4d1/0x4ce - each kills the timer, ships the
    // command as a 0x402 stat when host, then EndDialog - or cancels on 0x4c6.
    RVA(0x000be550, 0x193)
    i32 CALLBACK DropInDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (BlockScreenSaver(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_curMulti = (CMulti*)g_gameReg->m_curState;
                NetDlgInitDropIn(hWnd, g_curMulti);
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
                Init_2ed7(hWnd, g_curMulti);
                return 1;
        }
        return 0;
    }

    // __cdecl(hWnd, ctx): show a drop-in prompt, init, arm a timer, cache a child.
    RVA(0x000be760, 0x82)
    void NetDlgInitDropIn(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];
            // The pending drop-in player's name = g_str649618's CString data (m_pszData,
            // the char* stored at 0x249618); its length lives 8 bytes before the data.
            const char* pn = g_str649618;
            if (*(i32*)(pn - 8)) {
                sprintf(buf, "New Player Drop-In Request: %s", pn);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            Init_2ed7(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_sharedFlag = (i32)GetDlgItem(hWnd, 0x4b6);
        }
    }

    // Init_2ed7 (0xbe820, reached via ILT thunk 0x2ed7): enable/disable the accept
    // (0x4d0) and reject (0x4d1) drop-in buttons per the game-state's host flag. ctx
    // is the current CMulti (g_curMulti). __cdecl. Re-homed from src/Stub/ReconBatch2.cpp
    // (was the EnableButtons_be820 / DlgData_be820 view; obj->m_528 == CMulti::m_isHost).
    RVA(0x000be820, 0x49)
    void Init_2ed7(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            EnableWindow(GetDlgItem(hWnd, 0x4d0), ((CMulti*)ctx)->m_isHost);
            EnableWindow(GetDlgItem(hWnd, 0x4d1), ((CMulti*)ctx)->m_isHost);
        }
    }
} // namespace NetLobby
