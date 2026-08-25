#ifndef GRUNTZ_GRUNTZ_GAMEMODEID_H
#define GRUNTZ_GRUNTZ_GAMEMODEID_H

#include <Enums.h>

// What kind of session CGruntzMgr::m_gameMode says is running.
//
// Every value is pinned by its producer in CGruntzMgr::HandleCommand:
//
//   0  the constructor's seed, before any session exists
//   1  CMD_NEW_GAME / CMD_NEW_GAME_ALT / CMD_LOAD_WORLD /
//      CMD_CONTINUE_AT_MAX_LEVEL, and Questz save records
//   2  CMD_MULTI_JOIN, and CMulti::LoadGameAssetNamespaces on entering the
//      multiplayer session
//   3  CMD_START_BATTLEZ_GAME, and re-entering a level whose save record says
//      `si->m_isBattlez`.
//
// The built-in-level flags separately choose the GAME_BATTLEZ / GAME_MULTI
// resource banks; m_isCustomLevel identifies an external WWD.
GZ_ENUM_BEGIN(GameModeId)
    GAMEMODE_NONE = 0,
    GAMEMODE_QUESTZ = 1,
    GAMEMODE_MULTIPLAYER = 2,
    GAMEMODE_BATTLEZ = 3
GZ_ENUM_END(GameModeId)

#endif // GRUNTZ_GRUNTZ_GAMEMODEID_H
