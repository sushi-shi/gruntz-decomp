#ifndef NET_NETLOBBY_H
#define NET_NETLOBBY_H

struct HWND__;

namespace NetLobby {
    // The active modeless-dialog HWND cache (0x64557c), cached on dialog entry/init.
    extern HWND__* g_curDlg;
} // namespace NetLobby

#endif // NET_NETLOBBY_H
