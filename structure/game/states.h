#ifndef GAME_STATES_H
#define GAME_STATES_H

/*
 * Game state machine — CState base + concrete states.
 *
 * The STATEZ_* ids (see ../enums.h, enum Statez) drive this hierarchy. Each
 * concrete state is a screen/mode of the game (splash, menu, gameplay, etc.).
 *
 * Provenance: ALL names from RTTI (mangled names kept). The only layout note we
 * have is a tomalla @todo for CPlay (see below). Otherwise name-only / @todo.
 */

/*
 * CState — base state class.
 * .?AVCState@@
 */
class CState { /* .?AVCState@@ */ };  //@todo layout

/* --- front-end / non-gameplay states --- */
class CSplashState  { /* .?AVCSplashState@@ */ };   // STATEZ_SPLASH
class CMenuState    { /* .?AVCMenuState@@ */ };     // STATEZ_MENU
class CHelpState    { /* .?AVCHelpState@@ */ };     // STATEZ_HELP
class CCreditsState { /* .?AVCCreditsState@@ */ };  // STATEZ_CREDITZ
class CAttract      { /* .?AVCAttract@@ */ };       // STATEZ_ATTRACT (attract loop)
class CDemo         { /* .?AVCDemo@@ */ };           // demo playback

/* --- gameplay states --- */
/*
 * CPlay — single-player gameplay state.
 * .?AVCPlay@@
 * tomalla @todo note (refs/tomalla-gruntz/gruntz/cstate.h):
 *   CPlay + 0x3f4 -> pointer to unknown struct
 *     + 0x38 -> int: starting game time to measure stats from (in ms)
 * Offsets are tomalla's (1.0.1.77), unverified for v1.0.
 */
class CPlay { /* .?AVCPlay@@ */ };  //@todo layout (see note above)

class CMulti           { /* .?AVCMulti@@ */ };            // STATEZ_MULTI (multiplayer)
class CBootyState      { /* .?AVCBootyState@@ */ };       // STATEZ_BOOTY (end-of-level booty)
class CMultiBootyState { /* .?AVCMultiBootyState@@ */ };  // MP booty screen

#endif /* GAME_STATES_H */
