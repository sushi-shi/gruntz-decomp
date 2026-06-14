#ifndef GAME_HAZARDS_H
#define GAME_HAZARDS_H

/*
 * World hazards & moving level objects — the dangerous/dynamic things in a level:
 * giant rocks, rolling balls, rain clouds, spotlights, UFOs, path/static hazards,
 * kitchen slime, toob spikez, brickz.
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only stubs (no layout recovered).
 * Corroborating strings: "No giant rock logic found {around|at}: x=%d, y=%d",
 * LEVEL_UFO / LEVEL_ROLLINGBALL sound keys (STRINGS_ANALYSIS.md §5, §11).
 */

class CGiantRock      { /* .?AVCGiantRock@@ */ };
class CGiantRockLogic { /* .?AVCGiantRockLogic@@ */ };   // plugs into CUserLogic dispatch
class CRollingBall    { /* .?AVCRollingBall@@ */ };
class CRainCloud      { /* .?AVCRainCloud@@ */ };
class CSpotLight      { /* .?AVCSpotLight@@ */ };
class CLightFx        { /* .?AVCLightFx@@ */ };
class CUFO            { /* .?AVCUFO@@ */ };
class CPathHazard     { /* .?AVCPathHazard@@ */ };
class CStaticHazard   { /* .?AVCStaticHazard@@ */ };
class CKitchenSlime   { /* .?AVCKitchenSlime@@ */ };
class CToobSpikez     { /* .?AVCToobSpikez@@ */ };
class CBrickz         { /* .?AVCBrickz@@ */ };

#endif /* GAME_HAZARDS_H */
