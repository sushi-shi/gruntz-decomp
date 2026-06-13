#ifndef GAME_MAPMGR_H
#define GAME_MAPMGR_H

/*
 * Map managers — CMapMgr (engine base) and CGruntzMapMgr (game subclass), which
 * load/validate the WWD level data and place objects.
 *
 * Provenance: NAMES from RTTI (mangled names kept). Name-only / @todo.
 * The CGruntzMapMgr map-load VALIDATOR strings (STRINGS_ANALYSIS.md §5) reveal an
 * (x:int, y:int) tile signature and the placement checks, e.g.:
 *   "Bad {brickz|rock|trigger|switch|...} at: x=%d, y=%d"
 *   "Could not add Grunt: Player=%d, x=%d, y=%d"
 *   "Plane %s: Bad map {tile|image set} value (%i) at %i,%i"
 *   "Switch on an unknown tile at: x=%d, y=%d"
 */

/*
 * CMapMgr — engine map-manager base.
 * .?AVCMapMgr@@
 */
class CMapMgr { /* .?AVCMapMgr@@ */ };  //@todo layout

/*
 * CGruntzMapMgr — Gruntz map manager (loads/validates WWD; places gruntz/objects).
 * .?AVCGruntzMapMgr@@
 */
class CGruntzMapMgr /* : public CMapMgr @todo */
{
public:
    //@size: unknown @todo
    //@todo: load/validate pipeline. Validator messages imply (x,y) int tile coords
    //       and per-plane tile/image-set value checks.
};

#endif /* GAME_MAPMGR_H */
