#ifndef GRUNTZ_GRUNTZ_PLAYERCOMMANDKIND_H
#define GRUNTZ_GRUNTZ_PLAYERCOMMANDKIND_H

#include <Enums.h>

// The verb of a replicated player order - CGruntzCommand::m_commandKind, sent
// over the wire and dispatched by CPlay::ExecCommand.
//
// PARTIAL BY DESIGN. Five of the eleven values are named here because two
// independent things agree on each; the other six (2, 3, 4, 9, 10 and the pairs
// they form) are deliberately left as bare labels, because naming them would
// mean naming CTriggerMgr::ApplyTriggerA / ApplyTriggerB / ReportRecordsA /
// ReportRecordsB first - and those are themselves placeholder names. A guess
// there would be a guess citing itself as evidence.
//
//   0  PLACE_GRUNT
//      Its arm's whole body is m_cmdGrid->PlaceObject(...). Both producers
//      agree: CPlay::PlaceStartGruntz sends it, and so does the start-marker
//      branch of OnLButtonDblClk, which snaps the click to a tile centre
//      ((px & 0xffe0) + 0x10) and requires FindReadySlot() first.
//
//   5  STOP
//      Its arm resets the grunt and nothing else - clears the reroll window,
//      m_tileClaimed = 0, m_arrivalState = 0, SetEntrancePos(1, 1). Its one
//      producer is EnqueueGroupCells, i.e. it goes out to every selected grunt.
//
//   6  GUARD_BEGIN   7  GUARD_END
//      A proven pair, because their flag edits are exact complements:
//      6 does m_arrivalFlags |= 0x18040402, 7 does m_arrivalFlags &= 0xe7fbfbfd.
//      6 additionally sets m_arrivalState = 4, m_defenderState = AISTATE_SEEK
//      and a m_defenderRadius (per-tool, defaulting to the bute key
//      "PlayerDefenderRadius" + 1), and parks m_defenderPx at the grunt's own
//      tile; 7 sets m_arrivalState = 0. The "defender" members are what name it,
//      and they were named independently of this switch.
//
//   8  GIVE_TOOL
//      Its arm is the only one that touches the toolbox: it clears
//      m_playerCommandPending, calls LoadPickupSprites, EnterHlRow(sel,
//      m_cursorFrame) and then SetCursorFrame(0) - i.e. it consumes the cursor's
//      held item. Its producer in OnLButtonDown passes m_cursorFrame as the
//      hit-test token, so the item under the cursor is what gets handed over.
GZ_ENUM_BEGIN(PlayerCommandKind)
    PLAYERCMD_PLACE_GRUNT = 0,
    PLAYERCMD_STOP = 5,
    PLAYERCMD_GUARD_BEGIN = 6,
    PLAYERCMD_GUARD_END = 7,
    PLAYERCMD_GIVE_TOOL = 8
GZ_ENUM_END(PlayerCommandKind)

#endif // GRUNTZ_GRUNTZ_PLAYERCOMMANDKIND_H
