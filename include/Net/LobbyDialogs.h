#ifndef NET_LOBBYDIALOGS_H
#define NET_LOBBYDIALOGS_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

class CMulti;

i32 BlockScreenSaver(void*, UINT, WPARAM, LPARAM);
namespace NetLobby {

    i32 CALLBACK HostWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    i32 CALLBACK JoinWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    i32 CALLBACK SessionWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    i32 CALLBACK NetGameDlgProc(HWND, UINT, WPARAM, LPARAM);
    void UpdateHostWaitDialog(HWND hWnd, void* ctx);
    void UpdateJoinWaitDialog(HWND hWnd, void* ctx);
    void UpdateLobbyDialog(HWND hWnd, void* ctx);
    void UpdateDropWaitDialog(HWND hWnd, void* ctx);
    void UpdateSessionWaitDialog(HWND hWnd, void* ctx);
    void UpdateDropInDialog(HWND hWnd, void* ctx);
    void InitializeLobbyDialog(HWND, void*);
    void InitializeDropWaitDialog(HWND, void*);
    void NetDlgSessionStop(HWND, CMulti*);
    void NetChatSubmit(HWND, void*);
    void InitializeHostWaitDialog(HWND hWnd, void* ctx);
    void InitializeJoinWaitDialog(HWND hWnd, void* ctx);
    void InitializeSessionWaitDialog(HWND hWnd, void* ctx);
    void InitializeDropInDialog(HWND hWnd, void* ctx);
} // namespace NetLobby

#endif // NET_LOBBYDIALOGS_H
