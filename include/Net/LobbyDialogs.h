#ifndef NET_LOBBYDIALOGS_H
#define NET_LOBBYDIALOGS_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

class CMulti;

i32 BlockScreenSaver(void*, UINT, WPARAM, LPARAM);
namespace NetLobby {

    i32 CALLBACK HostWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    i32 CALLBACK JoinWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    i32 CALLBACK SessionWaitDlgProc(HWND, UINT, WPARAM, LPARAM);
    i32 CALLBACK NetGameDlgProc(HWND, UINT, WPARAM, LPARAM);
    void Init_bda50(HWND hWnd, void* ctx);
    void Init_bdbe0(HWND hWnd, void* ctx);
    void Init_bddb0(HWND hWnd, void* ctx);
    void Init_be3e0(HWND hWnd, void* ctx);
    void Init_2522(HWND hWnd, void* ctx);
    void Init_2ed7(HWND hWnd, void* ctx);
    void NetDlgInit_bdd60(HWND, void*);
    void NetDlgInitDropWait(HWND, void*);
    void NetDlgSessionStop(HWND, CMulti*);
    void NetChatSubmit(HWND, void*);
    void NetDlgInit_bda00(HWND hWnd, void* ctx);
    void NetDlgInit_bdb90(HWND hWnd, void* ctx);
    void NetDlgInit_bdfe0(HWND hWnd, void* ctx);
    void NetDlgInitDropIn(HWND hWnd, void* ctx);
} // namespace NetLobby

#endif // NET_LOBBYDIALOGS_H
