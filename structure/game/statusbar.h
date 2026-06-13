#ifndef GAME_STATUSBAR_H
#define GAME_STATUSBAR_H

/*
 * HUD / status-bar widget hierarchy — the CSBI_* ("status-bar item") family plus
 * the in-game HUD elements (icons, text, level timer).
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only / @todo.
 * Corroborating: GAME_STATUSBAR_TABZ_* keys; the profiler line
 * "Input=…, …, StatusBar=…, Flip=…" (STRINGS_ANALYSIS.md §5, §10).
 *
 * CGruntPowerupSprite is grouped under sprites.h (per-grunt overlay), not here.
 */

/* --- status-bar item base & visual primitives --- */
class CStatusBarItem   { /* .?AVCStatusBarItem@@ */ };
class CSBI_Image       { /* .?AVCSBI_Image@@ */ };
class CSBI_ImageSet    { /* .?AVCSBI_ImageSet@@ */ };
class CSBI_ImageSetAni { /* .?AVCSBI_ImageSetAni@@ */ };
class CSBI_RectOnly    { /* .?AVCSBI_RectOnly@@ */ };
class CSBI_MenuItem    { /* .?AVCSBI_MenuItem@@ */ };
class CSBI_SideTab     { /* .?AVCSBI_SideTab@@ */ };

/* --- specialized status-bar items --- */
class CSBI_GruntMachine    { /* .?AVCSBI_GruntMachine@@ */ };
class CSBI_StatzTabArrow   { /* .?AVCSBI_StatzTabArrow@@ */ };
class CSBI_StatzTabGruntBar{ /* .?AVCSBI_StatzTabGruntBar@@ */ };
class CSBI_WarlordHead     { /* .?AVCSBI_WarlordHead@@ */ };
class CSBI_WellGoo         { /* .?AVCSBI_WellGoo@@ */ };

/* --- in-game HUD elements --- */
class CInGameIcon { /* .?AVCInGameIcon@@ */ };
class CInGameText { /* .?AVCInGameText@@ */ };
class CLevelTime  { /* .?AVCLevelTime@@ */ };

#endif /* GAME_STATUSBAR_H */
