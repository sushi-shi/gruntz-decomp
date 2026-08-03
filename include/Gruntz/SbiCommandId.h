#ifndef GRUNTZ_GRUNTZ_SBICOMMANDID_H
#define GRUNTZ_GRUNTZ_SBICOMMANDID_H

#include <Enums.h>

// The status bar's own widget command ids, as dispatched by CStatusBarMgr's
// `cmd` switches.
//
// Each one is named by what its arm DOES, and most of them do it by posting a
// GruntzCommandId that is already named - so the widget takes the name of the
// command it raises:
//
//   0x1f4 -> CMD_PAUSE_TOGGLE        0x1f8 -> CMD_SHOW_BOOTY_STATE
//   0x1f5 -> CMD_LOAD_GAME_DIALOG    0x324 -> CMD_RELOAD_LEVEL
//   0x1f6 -> CMD_QUICK_SAVE_PROMPT   0x325 -> CMD_MAIN_MENU
//   0x1f7 -> CMD_CONFIG_SETTINGS     0x327 -> CMD_MAIN_MENU
//
// The three dock commands name themselves - their arms call RefreshA(),
// DockStatusBarRight() and HideRect(), which are the three StatusBarDock states.
//
// SBICMD_DESTRUCT is retail's own word: its arm reads the bute key
// "StatusBar" / "DestructButtonWarningDelay".
//
// NOTE this space also holds the five tabs. CStatusBarMgr bounds `cmd` with
// `cmd <= 0 || cmd > 5` and hands it to SetTabState, so StatusBarTab's 1..5 are
// the low end of THIS domain - which is why those call sites convert rather
// than compare.
GZ_ENUM_BEGIN(SbiCommandId)
    SBICMD_PAUSE = 0x1f4,
    SBICMD_LOAD_GAME = 0x1f5,
    SBICMD_SAVE_GAME = 0x1f6,
    SBICMD_SETTINGS = 0x1f7,
    SBICMD_BOOTY_STATE = 0x1f8,
    SBICMD_GAME_TAB = 0x1fa,
    SBICMD_DESTRUCT = 0x1fc,
    // The dock trio is dispatched as a binary split - `cmd > SBICMD_DOCK_FIRST`
    // selects the other two - so the boundary gets its own name at the value the
    // test actually compares against.
    SBICMD_DOCK_LEFT = 0x259,
    SBICMD_DOCK_FIRST = SBICMD_DOCK_LEFT,
    SBICMD_DOCK_RIGHT = 0x25a,
    SBICMD_HIDE = 0x25b,
    SBICMD_RELOAD_LEVEL = 0x324,
    // Two widgets raise CMD_MAIN_MENU with identical bodies, differing only in which
    // cue they play (HiCueLookup vs HiCueTimed), so they are distinguished the same
    // way TileCollisionKind distinguishes its indistinguishable arrow pairs.
    SBICMD_MAIN_MENU_A = 0x325,
    SBICMD_MAIN_MENU_B = 0x327
GZ_ENUM_END(SbiCommandId)

// 0x1f9 and 0x328 are DELIBERATELY absent. 0x1f9 toggles m_frameGate and calls
// FinishLevel with it before entering an overlay drag, and 0x328 only plays a
// cue; neither arm says what the widget IS.

#endif // GRUNTZ_GRUNTZ_SBICOMMANDID_H
