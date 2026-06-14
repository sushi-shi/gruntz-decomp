#ifndef GAME_WORLD_OBJECTS_H
#define GAME_WORLD_OBJECTS_H

/*
 * Misc. interactive world objects — teleport/warp network, the fortress flag and
 * Warlord (enemy boss), and the dropped-object system (objects a grunt drops, with
 * shadows, managed by an object dropper).
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only stubs (no layout recovered).
 * Corroborating: GAME_TELEPORTER sound key; WARLORDZ_* (KING/NAPOLEAN/PATTON/
 * VIKING) sprite namespace (STRINGS_ANALYSIS.md §11); fortress-capture MP objective
 * text (§10).
 */

/* --- teleport / warp network --- */
class CTeleporter    { /* .?AVCTeleporter@@ */ };
class CWormhole      { /* .?AVCWormhole@@ */ };
class CWarpStonePad  { /* .?AVCWarpStonePad@@ */ };

/* --- fortress / boss --- */
class CFortressFlag  { /* .?AVCFortressFlag@@ */ };
class CWarlord       { /* .?AVCWarlord@@ */ };   // see enum Warlord in ../enums.h

/* --- dropped-object system --- */
class CDroppedObject       { /* .?AVCDroppedObject@@ */ };
class CDroppedObjectShadow { /* .?AVCDroppedObjectShadow@@ */ };
class CObjectDropper        { /* .?AVCObjectDropper@@ */ };

#endif /* GAME_WORLD_OBJECTS_H */
