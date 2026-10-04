#ifndef GRUNTZ_STATECHANGE_H
#define GRUNTZ_STATECHANGE_H
#include <string>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzCommandId.h>

struct StateChangeOptions {
    StateChangeOptions(i32 errorSite = 0x435, ErrorStringId errorId = IDS_SET_GAME_STATE,
        GameStateId fallbackState = GAMESTATE_NONE)
        : error(errorId), site(errorSite), fallback(fallbackState),
          afterCommand(CMD_MAIN_MENU), postCommand(false), connectRound(false), restoreSave(false) {}
    StateChangeOptions then(GruntzCommandId command) const {
        StateChangeOptions result = *this;
        result.afterCommand = command;
        result.postCommand = true;
        return result;
    }
    ErrorStringId error;
    i32 site;
    GameStateId fallback;
    GruntzCommandId afterCommand;
    bool postCommand;
    bool connectRound;
    bool restoreSave;
    std::string snapshot;
};

enum StateChangeKind { ReplaceState, ResumeStackedState, ReloadState };
struct StateChange {
    StateChange() : kind(ReplaceState), target(GAMESTATE_NONE), previous(GAMESTATE_NONE),
        level(0), loadMode(0), keepCurrent(false) {}
    StateChangeKind kind;
    GameStateId target, previous;
    i32 level, loadMode;
    bool keepCurrent;
    StateChangeOptions options;
};
#endif
