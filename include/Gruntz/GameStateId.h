#ifndef GRUNTZ_GRUNTZ_GAMESTATEID_H
#define GRUNTZ_GRUNTZ_GAMESTATEID_H

#include <Enums.h>

GZ_ENUM_BEGIN(GameStateId)
    GAMESTATE_BASE = 1,
    GAMESTATE_ATTRACT = 2,
    GAMESTATE_PLAY = 3,
    GAMESTATE_MENU = 5,
    GAMESTATE_CREDITS = 8,
    GAMESTATE_BOOTY = 0xa,
    // 0xb and 0xd are NOT in the state factory's switch, so they build no new
    // state object; both are invoked with keepCurrent=1, which leaves the
    // running state in place. They show the screen over whatever is current.
    GAMESTATE_CREDITS_OVER_CURRENT = 0xb,
    GAMESTATE_BOOTY_OVER_CURRENT = 0xd,
    GAMESTATE_DEMO = 7,
    GAMESTATE_HELP = 9,
    GAMESTATE_SPLASH = 0xe,
    // 0x11 is the MULTIPLAYER play state, not an absence: CGruntzMgr's state
    // factory maps 17 -> `new CMulti`, and CMulti::Update() returns it. Every
    // reader pairs it with GAMESTATE_PLAY to ask "are we in gameplay?" - one
    // even assigns it to a local called isPlay.
    GAMESTATE_MULTI = 0x11,
    GAMESTATE_MULTIBOOTY = 0x12
GZ_ENUM_END(GameStateId)

#endif // GRUNTZ_GRUNTZ_GAMESTATEID_H
