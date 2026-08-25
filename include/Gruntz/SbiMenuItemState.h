#ifndef GRUNTZ_GRUNTZ_SBIMENUITEMSTATE_H
#define GRUNTZ_GRUNTZ_SBIMENUITEMSTATE_H

#include <Enums.h>

// The visual state of a status-bar menu item (a tab), as carried by
// CSBI_MenuItem::m_state and passed to SetState/ProbeState/SetTabState.
//
// Each value is named by what its arm does:
//
//   1  the constructor's seed, and the state everything falls BACK to -
//      ProbeState answers SetState(1) when a sibling takes 2 or 3, and Blit
//      drops 2 back to it
//   2  plays the hover cue; SetState refuses it while already 3, because a
//      selected tab does not also show hover
//   3  selects: ClearTabGroup, set m_activeTab, LoadTabSprites, Deactivate
//
// ProbeState is the mutual-exclusion half: "a sibling is entering this state,
// so drop me to NORMAL" - which is why probing with NORMAL itself does nothing.
GZ_ENUM_BEGIN(SbiMenuItemState)
// The constructor's seed, alongside m_record = 0. Every entry point bails on a
// null m_record, so this is "not built yet" rather than a drawable state.
    MENUITEM_UNSET = 0,
    MENUITEM_NORMAL = 1,
    MENUITEM_HIGHLIGHT = 2,
    MENUITEM_SELECTED = 3,
    // Greyed out. Set on the MULTIPLAYER tab when m_gameMode is GAMEMODE_QUESTZ,
    // and the very next thing the code does is check the sprite actually has this
    // frame (`m_minIndex <= 4 && m_maxIndex >= 4`) - which is also the clue that
    // the state IS the sprite's frame index, not a parallel enum.
    MENUITEM_DISABLED = 4
GZ_ENUM_END(SbiMenuItemState)

#endif // GRUNTZ_GRUNTZ_SBIMENUITEMSTATE_H
