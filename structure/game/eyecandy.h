#ifndef GAME_EYECANDY_H
#define GAME_EYECANDY_H

/*
 * Eye-candy & animation decoration — non-interactive visual flourishes layered
 * in front of / behind the play area, menu sparkles, toy peeks, and the generic
 * animation helpers (cycle / simple / single-frame).
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only stubs (no layout recovered).
 */

/* --- layered decorative candy (front/behind, static + animated) --- */
class CEyeCandy       { /* .?AVCEyeCandy@@ */ };
class CEyeCandyAni    { /* .?AVCEyeCandyAni@@ */ };
class CFrontCandy     { /* .?AVCFrontCandy@@ */ };
class CFrontCandyAni  { /* .?AVCFrontCandyAni@@ */ };
class CBehindCandy    { /* .?AVCBehindCandy@@ */ };
class CBehindCandyAni { /* .?AVCBehindCandyAni@@ */ };

/* --- menu / toy flourishes --- */
class CMenuSparkle    { /* .?AVCMenuSparkle@@ */ };
class CToyPeek        { /* .?AVCToyPeek@@ */ };

/* --- generic animation helpers --- */
class CAniCycle        { /* .?AVCAniCycle@@ */ };
class CSimpleAnimation { /* .?AVCSimpleAnimation@@ */ };
class CSingleAnimation { /* .?AVCSingleAnimation@@ */ };

#endif /* GAME_EYECANDY_H */
