// NetLobby.h - shared globals of the multiplayer LOBBY dialog cluster (DATA homes
// in src/Net/LobbyDialogs.cpp). Declared here so the many dialog/window consumers
// reference them from this owner header instead of per-TU externs. Kept light (a
// forward-declared HWND__ only) so pure-Win32 and MFC TUs alike can include it.
#ifndef NET_NETLOBBY_H
#define NET_NETLOBBY_H

struct HWND__;

namespace NetLobby {
    // The active modeless-dialog HWND cache (0x64557c), cached on dialog entry/init.
    extern HWND__* g_curDlg;
} // namespace NetLobby

#endif // NET_NETLOBBY_H
