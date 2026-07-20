#ifndef GRUNTZ_GRUNTZ_GAMESTATEID_H
#define GRUNTZ_GRUNTZ_GAMESTATEID_H

enum GameStateId {
    GAMESTATE_BASE = 1,         // CState        (base default)
    GAMESTATE_ATTRACT = 2,      // CAttract
    GAMESTATE_PLAY = 3,         // CPlay         (in-game / in-world)
    GAMESTATE_MENU = 5,         // CMenuState
    GAMESTATE_CREDITS = 8,      // CCreditsState
    GAMESTATE_BOOTY = 0xa,      // CBootyState
    GAMESTATE_NONE = 0x11,      // PerFrameTick sentinel: suppresses frame timing
    GAMESTATE_MULTIBOOTY = 0x12 // CMultiBootyState
};

#endif // GRUNTZ_GRUNTZ_GAMESTATEID_H
