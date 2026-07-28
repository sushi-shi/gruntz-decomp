// LobbyDialogs.h - the LobbyDialogs TU's external declarations.
#ifndef NET_LOBBYDIALOGS_H
#define NET_LOBBYDIALOGS_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

class CMulti;

i32 BlockScreenSaver(void*, UINT, WPARAM, LPARAM);
namespace NetLobby {
    // The four modal DlgProcs CMulti hands to CMulti::RunErrorDialog. Retail routes
    // each address-take through its /INCREMENTAL ILT jmp-thunk (0x113b / 0x27fc /
    // 0x301c / 0x3387 -> the bodies below), which is why they used to be modeled as
    // `extern "C" void MultiPauseCallback()` placeholder thunk symbols.
    i32 CALLBACK HostWaitDlgProc(HWND, UINT, WPARAM, LPARAM);    // 0xbd850  MULTI_PAUSE
    i32 CALLBACK JoinWaitDlgProc(HWND, UINT, WPARAM, LPARAM);    // 0xbda70  MULTI_OPTIONZ
    i32 CALLBACK SessionWaitDlgProc(HWND, UINT, WPARAM, LPARAM); // 0xbddd0  MULTI_OUTOFSYNC
    i32 CALLBACK NetGameDlgProc(HWND, UINT, WPARAM, LPARAM);     // 0xbe0a0  MULTI_DROPPLAYER
    void Init_bda50(HWND hWnd, void* ctx);       // 0xbda50 (host-wait init/timer hook)
    void Init_bdbe0(HWND hWnd, void* ctx);       // 0xbdbe0 (join-wait init/timer hook)
    void Init_bddb0(HWND hWnd, void* ctx);       // 0xbddb0 (lobby init/timer hook)
    void Init_be3e0(HWND hWnd, void* ctx);       // 0xbe3e0 (drop-wait init/timer hook)
    void Init_2522(HWND hWnd, void* ctx);        // 0xbe030 (session-wait button enable)
    void Init_2ed7(HWND hWnd, void* ctx);        // 0xbe820 (drop-in button enable)
    void NetDlgInit_bdd60(HWND, void*);          // 0xbdd60 (ex OnLobbyInit_2c66)
    void NetDlgInitDropWait(HWND, void*);        // 0xbe2f0 (ex OnLobbyInit_371f)
    void NetDlgSessionStop(HWND, CMulti*);       // 0xbe490 (ex OnLobbyTimerA_265d)
    void NetChatSubmit(HWND, void*);             // 0xbe400 (ex OnLobbyCancel_2ae0)
    void NetDlgInit_bda00(HWND hWnd, void* ctx); // 0xbda00
    void NetDlgInit_bdb90(HWND hWnd, void* ctx); // 0xbdb90
    void NetDlgInit_bdfe0(HWND hWnd, void* ctx); // 0xbdfe0
    void NetDlgInitDropIn(HWND hWnd, void* ctx); // 0xbe760
} // namespace NetLobby

#endif // NET_LOBBYDIALOGS_H
