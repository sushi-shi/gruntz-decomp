#ifndef GAME_SPRITES_H
#define GAME_SPRITES_H

/*
 * Grunt UI sprites + cursor sprite — the per-grunt overlay widgets that float on
 * or near a grunt (health bar, stamina bar, powerup icon, selection ring, held
 * toy + toy timer + wingz timer), plus the cursor-snap sprite.
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only / @todo.
 * (Note: HUD/status-bar widgets live in statusbar.h; these are the in-world
 * per-grunt overlays.)
 */

class CGruntHealthSprite    { /* .?AVCGruntHealthSprite@@ */ };
class CGruntStaminaSprite   { /* .?AVCGruntStaminaSprite@@ */ };
class CGruntPowerupSprite   { /* .?AVCGruntPowerupSprite@@ */ };
class CGruntSelectedSprite  { /* .?AVCGruntSelectedSprite@@ */ };
class CGruntToySprite       { /* .?AVCGruntToySprite@@ */ };
class CGruntToyTimeSprite   { /* .?AVCGruntToyTimeSprite@@ */ };
class CGruntWingzTimeSprite { /* .?AVCGruntWingzTimeSprite@@ */ };
class CCursorSnapSprite     { /* .?AVCCursorSnapSprite@@ */ };
class CStatusBarSprite      { /* .?AVCStatusBarSprite@@ */ };

#endif /* GAME_SPRITES_H */
