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
    SBICMD_MAIN_MENU_B = 0x327,

    // Three BANDS of per-slot widgets. Each band's base is proven by the
    // subtraction its own arm performs - the id is turned straight back into a
    // 0-based index - and each band's outer bounds by the guard that rejects
    // everything outside it before the split.
    //
    //   `cmd < 0x12c || cmd > 0x149` then
    //     cmd <= 0x13a -> ToggleStat(cmd - 0x12c)
    //     else            PlaceCursorTarget(cmd - 0x13b, 0)
    SBICMD_STAT_TOGGLE_FIRST = 0x12c,
    SBICMD_STAT_TOGGLE_LAST = 0x13a,
    SBICMD_CURSOR_TARGET_FIRST = 0x13b,
    SBICMD_CURSOR_TARGET_LAST = 0x149,

    //   `cmd < 0xd3 || cmd > 0xde` then a three-way split, each arm
    //   re-basing on its own group's first id
    SBICMD_HL_GROUP0_FIRST = 0xd3,
    SBICMD_HL_GROUP0_LAST = 0xd6,
    SBICMD_HL_GROUP1_FIRST = 0xd7,
    SBICMD_HL_GROUP1_LAST = 0xda,
    SBICMD_HL_GROUP2_FIRST = 0xdb,
    SBICMD_HL_GROUP2_LAST = 0xde
GZ_ENUM_END(SbiCommandId)

// 0x1f9 and 0x328 are DELIBERATELY absent. 0x1f9 toggles m_frameGate and calls
// FinishLevel with it before entering an overlay drag, and 0x328 only plays a
// cue; neither arm says what the widget IS.

#endif // GRUNTZ_GRUNTZ_SBICOMMANDID_H
