#ifndef GAME_MISC_H
#define GAME_MISC_H

/*
 * Miscellaneous game objects that don't fit a larger theme: no-op behaviors and a
 * single-frame message object.
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only / @todo.
 * CDoNothing / CDoNothingNormal are likely null-behavior placeholders used by the
 * trigger/logic system (a "do nothing" action). CSingleFrameMessage is probably a
 * transient one-frame on-screen message/notification.
 */

class CDoNothing          { /* .?AVCDoNothing@@ */ };
class CDoNothingNormal    { /* .?AVCDoNothingNormal@@ */ };
class CSingleFrameMessage { /* .?AVCSingleFrameMessage@@ */ };

#endif /* GAME_MISC_H */
