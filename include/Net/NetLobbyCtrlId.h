#ifndef NET_NETLOBBYCTRLID_H
#define NET_NETLOBBYCTRLID_H

#include <Enums.h>

// Dialog control ids shared by the multiplayer lobby dialogs.
//
// Only one of the bank can be named, and it is named by unanimity rather than
// by a resource script: SIX independent lobby dialog procedures test for
// IDC_NETCHAT_SEND in their WM_COMMAND arm, and in every one of them the arm's
// entire body is `NetChatSubmit(hWnd, g_curMulti); return 1;`. Nothing else in
// the tree does anything else with it, and CMultiStartDlg::EnableControls
// re-enables it beside IDCANCEL - which is what a chat Send button next to a
// Cancel button looks like.
//
// The dialogs' OTHER ids (0x4cc, 0x4cd, 0x4ce, 0x4d0, 0x4d1, 0x4d2, 0x4ea,
// 0x4f7) stay bare on purpose. Their arms are byte-for-byte the same shape -
// KillTimer, optionally SendNetStat, then EndDialog with the id itself - so
// nothing in the code distinguishes one button from another, and only the
// resource script could.
GZ_ENUM_BEGIN(NetLobbyCtrlId)
    IDC_NETCHAT_SEND = 0x4c6
GZ_ENUM_END(NetLobbyCtrlId)

#endif // NET_NETLOBBYCTRLID_H
