#ifndef GRUNTZ_GRUNTZ_STATUSBARTAB_H
#define GRUNTZ_GRUNTZ_STATUSBARTAB_H

#include <Enums.h>

// The item groups of the in-game status bar. Values 1..5 are selectable tabs;
// 0 holds the permanent bar controls and 6 holds the modal result dialog.
//
// Named by RETAIL'S OWN resource strings, not by guesswork:
// CStatusBarMgr::SetTabState(tab, state) gives exactly one of five sprites
// SetState and the other four ProbeState, and each sprite is the one built from
// its own GAME_STATUSBAR_TABZ_* image:
//
//   1 -> m_statzTabButton  GAME_STATUSBAR_TABZ_STATZTAB
//   2 -> m_gruntzTabButton  GAME_STATUSBAR_TABZ_GRUNTZTAB
//   3 -> m_resourceTabButton  GAME_STATUSBAR_TABZ_RESOURCETAB
//   4 -> m_multiTabButton  GAME_STATUSBAR_TABZ_MULTIPLAYERTAB
//   5 -> m_gameTabButton  GAME_STATUSBAR_TABZ_GAMETAB
//
// The sprite ORDER is not the tab order (tab 2 drives sprite 2 but tab 3 drives
// sprite 1), which is why the mapping has to be read off the code rather than
// assumed from the field names.
GZ_ENUM_BEGIN(StatusBarTab)
    TAB_ALL = -1,
    // The same zero is the permanent-controls group for CStatusBarItem::m_tab
    // and the no-active-tab sentinel for CStatusBarMgr::m_activeTab.
    TAB_CONTROLS = 0,
    TAB_NONE = 0,
    TAB_STATZ = 1,
    TAB_GRUNTZ = 2,
    TAB_RESOURCE = 3,
    TAB_MULTIPLAYER = 4,
    TAB_GAME = 5,
    TAB_DIALOG = 6,
    // The command handler bounds its tab argument with `cmd <= 0 || cmd > 5`, so
    // this is that bound, spelled inclusively the way retail spells it.
    TAB_LAST = TAB_GAME
GZ_ENUM_END(StatusBarTab)

#endif // GRUNTZ_GRUNTZ_STATUSBARTAB_H
