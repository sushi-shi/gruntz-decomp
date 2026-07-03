// GameStateId.h - the per-state "which state am I" id returned by
// CState::Update() (vtable slot 4 / +0x10). Each concrete game state overrides
// Update() to `return` its own id constant; the per-frame tick (RezMgr::
// PerFrameTick) and CGruntzMgr's state predicates gate on these ids.
//
// GAMESTATE_NONE (0x11) is the PerFrameTick sentinel: a state that suppresses
// timing (the "paused/hold" id) - PerFrameTick skips the frame-clock advance
// while `Update() == 0x11`, and the sound/cursor predicates treat {3,0x11} as
// the playable/in-world pair. No reconstructed state returns 0x11 itself.
//
// MSVC 5.0 enums are int-width, so typing Update() `GameStateId` instead of
// `i32` is matching-neutral (same 6-byte `mov eax,<id>; ret` body).
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
