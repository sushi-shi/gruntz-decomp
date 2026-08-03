#ifndef GRUNTZ_GRUNTZ_STATUSBARDOCK_H
#define GRUNTZ_GRUNTZ_STATUSBARDOCK_H

#include <Enums.h>

// Where the in-game status bar currently sits, as carried by
// CStatusBarMgr::m_position (and remembered in m_restorePosition).
//
// The RECT each state sets is what names it - the value is never described
// anywhere, but the geometry is unambiguous:
//
//   DockStatusBarRight()  SetRect(w - 0xa0, 0, w, 0x1e0)   then SetState(0)
//   RefreshA()            SetRect(0, 0, 0xa0, 0x1e0)       then SetState(1)
//   HideRect()            SetRect(-1, -1, -1, -1)          then SetState(2)
//
// 0xa0 is the bar's width, so 0 pins it to the right screen edge and 1 to the
// left; the all -1 rect is offscreen.
//
// Corroborated twice more. CSBI_SideTab builds its tabs at
// `m_rect10.left - 0x1c` when m_position is 0 and at `m_rect10.right` otherwise,
// i.e. always on the side facing the screen centre. And RefreshState only acts
// while HIDDEN, restoring to RefreshA or DockStatusBarRight according to
// m_restorePosition - which SetState saved on the way into hiding.
GZ_ENUM_BEGIN(StatusBarDock)
    STATUSBAR_DOCK_RIGHT = 0,
    STATUSBAR_DOCK_LEFT = 1,
    STATUSBAR_HIDDEN = 2
GZ_ENUM_END(StatusBarDock)

#endif // GRUNTZ_GRUNTZ_STATUSBARDOCK_H
