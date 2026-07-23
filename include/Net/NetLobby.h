#ifndef NET_NETLOBBY_H
#define NET_NETLOBBY_H

#include <Win32.h>

namespace NetLobby {
    // The active modeless-dialog HWND cache (0x64557c), cached on dialog entry/init.
    extern HWND g_curDlg;

    void WINAPI AppendEditLine(HWND edit, char* str);
} // namespace NetLobby

#endif // NET_NETLOBBY_H
