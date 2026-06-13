#ifndef GAME_TRIGGERS_H
#define GAME_TRIGGERS_H

/*
 * Trigger / switch / logic system — the puzzle-mechanism family.
 *
 * This is the large CTile*Trigger*/CTile*Logic family plus the standalone
 * triggers and the action/guard/waypoint markers. They are the puzzle backbone:
 * switches, pressure plates, toggle bridges, secret triggers, checkpoints, exits,
 * teleporters, voice triggers, etc.
 *
 * Provenance: ALL names from RTTI (verbatim mangled names kept on each class).
 * The map-load validator strings (STRINGS_ANALYSIS.md §5) corroborate the roles:
 *   "Bad {trigger|switch|secret switch|toggle switch|toggle-bridge trigger|
 *    up-down switch|hold switch|multi switch|time switch|once-only switch|
 *    pressure plate|covered powerup} at: x=%d, y=%d"
 *   "No {switch|trigger} logic found for {plate|switch} at: x=%d, y=%d"
 * NO field layouts recovered — all name-only / @todo.
 *
 * The *Logic classes plug into the CUserLogic dispatch system (see userlogic.h).
 */

/* --- tile trigger / switch base + logic --- */
class CTileTrigger                       { /* .?AVCTileTrigger@@ */ };
class CTileTriggerLogic                  { /* .?AVCTileTriggerLogic@@ */ };
class CTileTriggerSwitch                 { /* .?AVCTileTriggerSwitch@@ */ };
class CTileTriggerSwitchLogic            { /* .?AVCTileTriggerSwitchLogic@@ */ };
class CTileTriggerTransition             { /* .?AVCTileTriggerTransition@@ */ };

/* --- multi / exclusive switch logic --- */
class CTileMultiTriggerSwitchLogic       { /* .?AVCTileMultiTriggerSwitchLogic@@ */ };
class CTileExclusiveTriggerSwitchLogic   { /* .?AVCTileExclusiveTriggerSwitchLogic@@ */ };

/* --- time-gated switch logic --- */
class CTileTimeTriggerLogic              { /* .?AVCTileTimeTriggerLogic@@ */ };
class CTileTimeTriggerSwitchLogic        { /* .?AVCTileTimeTriggerSwitchLogic@@ */ };

/* --- secret triggers/switches --- */
class CTileSecretTrigger                 { /* .?AVCTileSecretTrigger@@ */ };
class CTileSecretTriggerLogic            { /* .?AVCTileSecretTriggerLogic@@ */ };
class CTileSecretTriggerSwitchLogic      { /* .?AVCTileSecretTriggerSwitchLogic@@ */ };

/* --- checkpoint --- */
class CCheckpointTrigger                 { /* .?AVCCheckpointTrigger@@ */ };
class CCheckpointTriggerSwitchLogic      { /* .?AVCCheckpointTriggerSwitchLogic@@ */ };

/* --- secret level / teleporter / exit / voice triggers --- */
class CSecretLevelTrigger                { /* .?AVCSecretLevelTrigger@@ */ };
class CSecretTeleporterTrigger           { /* .?AVCSecretTeleporterTrigger@@ */ };
class CExitTrigger                       { /* .?AVCExitTrigger@@ */ };
class CVoiceTrigger                      { /* .?AVCVoiceTrigger@@ */ };

/* --- area / guard / waypoint markers --- */
class CActionArea                        { /* .?AVCActionArea@@ */ };
class CGuardPoint                        { /* .?AVCGuardPoint@@ */ };
class CWayPoint                          { /* .?AVCWayPoint@@ */ };

/* --- covered powerup (covered by a switch/plate) + its logic --- */
class CCoveredPowerup                    { /* .?AVCCoveredPowerup@@ */ };
class CCoveredPowerupLogic               { /* .?AVCCoveredPowerupLogic@@ */ };

/* --- generic moving-object logic (used by rolling ball / giant rock paths?) --- */
class CMovingLogic                       { /* .?AVCMovingLogic@@ */ };

#endif /* GAME_TRIGGERS_H */
