#ifndef GRUNTZ_GRUNTZ_PLAYERCOMMANDKIND_H
#define GRUNTZ_GRUNTZ_PLAYERCOMMANDKIND_H

#include <Enums.h>

// The verb of a replicated player order - CGruntzCommand::m_commandKind, sent
// over the wire and dispatched by CPlay::ExecuteCommand.
//
//   0  PLACE_GRUNT
//      Its arm's whole body is m_triggerMgr->PlaceObject(...). Both producers
//      agree: CPlay::PlaceStartGruntz sends it, and so does the start-marker
//      branch of OnLButtonDblClk, which snaps the click to a tile centre
//      ((px & 0xffe0) + 0x10) and requires FindReadySlot() first.
//
//   5  STOP
//      Its arm resets the grunt and nothing else - clears the reroll window,
//      m_tileClaimed = 0, m_arrivalState = AI_NONE, SetEntrancePos(1, 1). Its one
//      producer is EnqueueGroupCells, i.e. it goes out to every selected grunt.
//
//   6  GUARD_BEGIN   7  GUARD_END
//      A proven pair, because their flag edits are exact complements:
//      6 does m_arrivalFlags |= 0x18040402, 7 does m_arrivalFlags &= 0xe7fbfbfd.
//      6 additionally sets m_arrivalState = AI_DEFENDER, m_defenderState = AISTATE_SEEK
//      and a m_defenderRadius (per-tool, defaulting to the bute key
//      "PlayerDefenderRadius" + 1), and parks m_defenderPx at the grunt's own
//      tile; 7 sets m_arrivalState = AI_NONE. The "defender" members are what name it,
//      and they were named independently of this switch.
//
//   2  MOVE
//      The arm delegates to ClearCell, whose only terminal action is
//      StepArrivalDrop at the requested point. EnqueueSelectedMove emits this value
//      for both single and grouped selections.
//
//   3  USE_TOOL_AT_POINT   9  USE_TOOL_ON_GRUNT
//      Both arms call UseEquippedToolAt, which dispatches on the grunt's equipped
//      tool. The second form resolves a grid-addressed grunt to its screen
//      position before applying the same operation.
//
//   4  USE_TOY_AT_POINT   10  USE_TOY_ON_GRUNT
//      Both arms call UseToyAt, which dispatches on m_vehiclePickupType.
//      The second form likewise carries a grid-addressed grunt target.
//
//   8  GIVE_TOOL
//      Its arm is the only one that touches the toolbox: it clears
//      m_playerCommandPending, calls LoadPickupSprites, EnterHlRow(sel,
//      m_cursorFrame) and then SetCursorFrame(0) - i.e. it consumes the cursor's
//      held item. Its producer in OnLButtonDown passes m_cursorFrame as the
//      hit-test token, so the item under the cursor is what gets handed over.
GZ_ENUM_BEGIN(PlayerCommandKind)
    PLAYERCMD_PLACE_GRUNT = 0,
    PLAYERCMD_MOVE = 2,
    PLAYERCMD_USE_TOOL_AT_POINT = 3,
    PLAYERCMD_USE_TOY_AT_POINT = 4,
    PLAYERCMD_STOP = 5,
    PLAYERCMD_GUARD_BEGIN = 6,
    PLAYERCMD_GUARD_END = 7,
    PLAYERCMD_GIVE_TOOL = 8,
    PLAYERCMD_USE_TOOL_ON_GRUNT = 9,
    PLAYERCMD_USE_TOY_ON_GRUNT = 10
GZ_ENUM_END(PlayerCommandKind)

#endif // GRUNTZ_GRUNTZ_PLAYERCOMMANDKIND_H
