#ifndef NET_LOBBYDIALOGS_H
#define NET_LOBBYDIALOGS_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

class CMulti;

// HWND is void* here on purpose: TimeSplit.cpp is a <Win32.h> TU, where
// windows.h leaves STRICT off and HWND *is* void*, so the definition mangles
// PAX.  Spelling HWND in this MFC (STRICT) header would resolve to nothing.
i32 BlockScreenSaver(void*, UINT, WPARAM, LPARAM);
namespace NetLobby {

    BOOL CALLBACK HostWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    BOOL CALLBACK JoinWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    BOOL CALLBACK SessionWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    BOOL CALLBACK NetGameDlgProc(HWND, UINT, WPARAM, LPARAM);
    void UpdateHostWaitDialog(HWND hWnd, CMulti* ctx);
    void UpdateJoinWaitDialog(HWND hWnd, CMulti* ctx);
    void UpdateLobbyDialog(HWND hWnd, CMulti* ctx);
    void UpdateDropWaitDialog(HWND hWnd, CMulti* ctx);
    void UpdateSessionWaitDialog(HWND hWnd, CMulti* ctx);
    void UpdateDropInDialog(HWND hWnd, CMulti* ctx);
    void InitializeLobbyDialog(HWND, CMulti*);
    void InitializeDropWaitDialog(HWND, CMulti*);
    void NetDlgSessionStop(HWND, CMulti*);
    void NetChatSubmit(HWND, CMulti*);
    void InitializeHostWaitDialog(HWND hWnd, CMulti* ctx);
    void InitializeJoinWaitDialog(HWND hWnd, CMulti* ctx);
    void InitializeSessionWaitDialog(HWND hWnd, CMulti* ctx);
    void InitializeDropInDialog(HWND hWnd, CMulti* ctx);
} // namespace NetLobby

#endif // NET_LOBBYDIALOGS_H
