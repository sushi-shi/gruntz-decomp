#ifndef GAME_COMMANDS_H
#define GAME_COMMANDS_H

/*
 * Command pattern — player input/orders to gruntz, in single- and multi-player
 * variants. In MP these are presumably serialized over DirectPlay (lockstep).
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only / @todo.
 */

class CGruntzCommand       { /* .?AVCGruntzCommand@@ */ };       // base command
class CGruntzSingleCommand { /* .?AVCGruntzSingleCommand@@ */ }; // single-player
class CGruntzMultiCommand  { /* .?AVCGruntzMultiCommand@@ */ };  // multiplayer

#endif /* GAME_COMMANDS_H */
