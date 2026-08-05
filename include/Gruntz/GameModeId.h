#ifndef GRUNTZ_GRUNTZ_GAMEMODEID_H
#define GRUNTZ_GRUNTZ_GAMEMODEID_H

#include <Enums.h>

// What kind of session CGruntzMgr::m_gameMode says is running.
//
// Every value is pinned by its producer in CGruntzMgr::HandleCommand:
//
//   0  the constructor's seed, before any session exists
//   1  CMD_NEW_GAME / CMD_NEW_GAME_ALT / CMD_LOAD_WORLD /
//      CMD_CONTINUE_AT_MAX_LEVEL, and a custom level that has NOT been won yet
//   2  CMD_MULTI_JOIN, and CMulti::LoadGameAssetNamespaces on entering the
//      multiplayer session
//   3  CMD_NEW_GAME_REPLAY, and re-entering a level whose save record says
//      `si->m_isWon` - which is why CGruntzMgr writes the flag straight back out
//      by comparing m_gameMode with GAMEMODE_BATTLEZ.
//
// This is NOT the Questz/Battlez axis: m_isBattlezLevel and m_isCustomLevel are
// separate flags, set alongside this one at the same sites.
GZ_ENUM_BEGIN(GameModeId)
    GAMEMODE_NONE = 0,
    GAMEMODE_SINGLE = 1,
    GAMEMODE_MULTIPLAYER = 2,
    GAMEMODE_REPLAY = 3
GZ_ENUM_END(GameModeId)

#endif // GRUNTZ_GRUNTZ_GAMEMODEID_H
