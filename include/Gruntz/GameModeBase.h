// GameModeBase.h - CGameModeBase, the WAP32 base the game-state classes chain
// their cleanup to (CState's dtor casts `(CGameModeBase *)this` and calls it).
// BaseCleanup has no body here: it is a reloc-masked external call.
#ifndef GRUNTZ_GRUNTZ_GAMEMODEBASE_H
#define GRUNTZ_GRUNTZ_GAMEMODEBASE_H

struct CGameModeBase {
    void BaseCleanup(); // thiscall, no-body -> reloc-masked external call
};

#endif // GRUNTZ_GRUNTZ_GAMEMODEBASE_H
