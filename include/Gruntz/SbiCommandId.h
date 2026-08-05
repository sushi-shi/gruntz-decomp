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
    SBICMD_NONE = 0,
    SBICMD_TAB_STATZ = 1,
    SBICMD_TAB_FIRST = SBICMD_TAB_STATZ,
    SBICMD_TAB_GRUNTZ = 2,
    SBICMD_TAB_RESOURCE = 3,
    SBICMD_TAB_MULTIPLAYER = 4,
    SBICMD_TAB_GAME = 5,
    SBICMD_TAB_LAST = SBICMD_TAB_GAME,

    SBICMD_SIDE_TAB_FIRST = 0xb,
    SBICMD_SIDE_TAB_LAST = 0x19,

    SBICMD_GRUNT_SLOT_FIRST = 0x64,
    SBICMD_GRUNT_SLOT_LAST = 0x68,
    SBICMD_GRUNT_WELL = 0x69,
    SBICMD_GRUNT_WELL_GOO = 0x6a,
    SBICMD_GRUNT_OVENS_TEXT = 0x6b,
    SBICMD_GRUNT_WELL_TEXT = 0x6c,

    SBICMD_RESOURCE_MAIN_BACKGROUND = 0xc8,
    SBICMD_RESOURCE_UPPER_BACKGROUND = 0xc9,
    SBICMD_RESOURCE_WINDOW_BACKGROUND = 0xca,
    SBICMD_RESOURCE_BELT_GROUP0 = 0xcb,
    SBICMD_RESOURCE_BELT_GROUP1 = 0xcc,
    SBICMD_RESOURCE_BELT_GROUP2 = 0xcd,

    // Resource-tab conveyor widgets. SetFallRect accepts either belt segment.
    SBICMD_CONVEYOR_TOP = 0xce,
    SBICMD_CONVEYOR_BOTTOM = 0xd0,
    SBICMD_RESOURCE_MACHINE_BACKGROUND = 0xd1,
    SBICMD_RESOURCE_MACHINE_FOREGROUND = 0xd2,
    SBICMD_RESOURCE_CURRENT_ITEM = 0xdf,
    SBICMD_RESOURCE_FALLING_ITEM = 0xe0,

    SBICMD_MULTIPLAYER_HEAD1 = 0x190,
    SBICMD_MULTIPLAYER_HEAD_FIRST = SBICMD_MULTIPLAYER_HEAD1,
    SBICMD_MULTIPLAYER_HEAD2 = 0x191,
    SBICMD_MULTIPLAYER_HEAD3 = 0x192,
    SBICMD_MULTIPLAYER_HEAD4 = 0x193,
    SBICMD_MULTIPLAYER_HEAD_LAST = SBICMD_MULTIPLAYER_HEAD4,

    SBICMD_PAUSE = 0x1f4,
    SBICMD_LOAD_GAME = 0x1f5,
    SBICMD_SAVE_GAME = 0x1f6,
    SBICMD_SETTINGS = 0x1f7,
    SBICMD_BOOTY_STATE = 0x1f8,
    SBICMD_QUIT = 0x1f9,
    SBICMD_GAME_TAB = 0x1fa,
    SBICMD_MISSION_STATUS = 0x1fb,
    SBICMD_DESTRUCT = 0x1fc,

    // The dock trio is dispatched as a binary split - `cmd > SBICMD_DOCK_FIRST`
    // selects the other two - so the boundary gets its own name at the value the
    // test actually compares against.
    SBICMD_DOCK_LEFT = 0x259,
    SBICMD_DOCK_FIRST = SBICMD_DOCK_LEFT,
    SBICMD_DOCK_RIGHT = 0x25a,
    SBICMD_HIDE = 0x25b,
    SBICMD_TAB_TITLE_TEXT = 0x25c,

    SBICMD_WARPSTONE_BASE = 0x2bc,
    SBICMD_WARPSTONE_FRAGMENT1 = 0x2bd,
    SBICMD_WARPSTONE_FRAGMENT2 = 0x2be,
    SBICMD_WARPSTONE_FRAGMENT3 = 0x2bf,
    SBICMD_WARPSTONE_FRAGMENT4 = 0x2c0,

    SBICMD_DIALOG_FRAME = 0x321,
    SBICMD_DIALOG_MISSION_STATUS = 0x322,
    // The displayed labels vary by level and game mode: next/replay/observe on
    // the primary button and main-menu/statz on the secondary button.
    SBICMD_DIALOG_PRIMARY = 0x324,
    SBICMD_DIALOG_SECONDARY = 0x325,
    SBICMD_DIALOG_REASON = 0x326,
    SBICMD_DIALOG_YES = 0x327,
    SBICMD_DIALOG_NO = 0x328,

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
    //   re-basing on its own group's first id.
    //
    //   The three groups are the HUD's three inventory rows, and four separate
    //   sites in CStatusBarMgr say which is which by tiering a PickupType the
    //   same way every time: `>= PICKUP_BRICKZ_FIRST` picks 2, else
    //   `>= PICKUP_TOYZ_FIRST` picks 1, else 0. So group 0 is toolz, 1 is toyz,
    //   2 is brickz - which is also the order they are drawn in, at
    //   m_itemBaseX 0x1d, 0x45 and 0x6d.
    SBICMD_HL_GROUP0_FIRST = 0xd3,
    SBICMD_HL_GROUP0_LAST = 0xd6,
    SBICMD_HL_GROUP1_FIRST = 0xd7,
    SBICMD_HL_GROUP1_LAST = 0xda,
    SBICMD_HL_GROUP2_FIRST = 0xdb,
    SBICMD_HL_GROUP2_LAST = 0xde
GZ_ENUM_END(SbiCommandId)

#endif // GRUNTZ_GRUNTZ_SBICOMMANDID_H
