#ifndef GRUNTZ_GRUNTZ_STATUSBARTAB_H
#define GRUNTZ_GRUNTZ_STATUSBARTAB_H

#include <Enums.h>

// The five tabs of the in-game status bar.
//
// Named by RETAIL'S OWN resource strings, not by guesswork:
// CStatusBarMgr::SetTabState(tab, state) gives exactly one of five sprites
// SetState and the other four ProbeState, and each sprite is the one built from
// its own GAME_STATUSBAR_TABZ_* image:
//
//   1 -> m_tabSprite0  GAME_STATUSBAR_TABZ_STATZTAB
//   2 -> m_tabSprite2  GAME_STATUSBAR_TABZ_GRUNTZTAB
//   3 -> m_tabSprite1  GAME_STATUSBAR_TABZ_RESOURCETAB
//   4 -> m_tabSprite3  GAME_STATUSBAR_TABZ_MULTIPLAYERTAB
//   5 -> m_tabSprite4  GAME_STATUSBAR_TABZ_GAMETAB
//
// The sprite ORDER is not the tab order (tab 2 drives sprite 2 but tab 3 drives
// sprite 1), which is why the mapping has to be read off the code rather than
// assumed from the field names.
GZ_ENUM_BEGIN(StatusBarTab)
    TAB_STATZ = 1,
    TAB_GRUNTZ = 2,
    TAB_RESOURCE = 3,
    TAB_MULTIPLAYER = 4,
    TAB_GAME = 5,
    // The command handler bounds its tab argument with `cmd <= 0 || cmd > 5`, so
    // this is that bound, spelled inclusively the way retail spells it.
    TAB_LAST = TAB_GAME
GZ_ENUM_END(StatusBarTab)

// Values 0 and 6 are DELIBERATELY absent. CStatusBarItem::m_tab is dispatched
// over 0..6, so it is a WIDER space than SetTabState's five: 0 is the arm that
// handles the bar's own commands (dock/hide/refresh plus the 1..5 tab picks) and
// 6 has a single site under m_toggleActive. Neither has evidence for a name, so
// both stay literal at the switches that read them.

#endif // GRUNTZ_GRUNTZ_STATUSBARTAB_H
