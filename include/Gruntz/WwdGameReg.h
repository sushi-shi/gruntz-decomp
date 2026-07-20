// WwdGameReg.h - the ONE canonical shape of the WwdGameReg-typed view of the
// global game-manager singleton (?g_gameReg@@3PAUWwdGameReg@@A, VA 0x64556c /
// RVA 0x24556c). This object is the RTTI-true CGruntzMgr: 0xa30 bytes, new'd with
// `push 0xa30; call operator new` by CGruntzApp::InitializeGameManager (@0x080a20).
//
// This layout was previously fractured into SEVEN per-TU reduced "views" (one
// bespoke `struct WwdGameReg` each in Grunt.h + GameMode / CFortressFlag /
// LevelTileValidation / UserLogic / GruntzCommand, plus a forward decl in
// BacklogStateLoaders). No sane dev writes one class seven ways: they are all the
// same object, unified here.
//
// The old claim that the 0x30 slot holds "a different concrete object per
// game-mode/facet" is CONTRADICTED and dead (2026-07-16): +0x30 is the ONE
// canonical CDDrawSurfaceMgr (the world/resource holder) - the grunt facet's
// "GruntSoundCat" (+0x08 factory == m_childGroup, +0x24 == m_level the CGameLevel,
// +0x28 == m_soundRegistry the CDDrawSubMgrLeafScan) and UserLogic's ex
// "CTeleResHolder" were per-TU views of it, all dissolved onto the canonical.
// The genuinely-reused slots (0x60/0x68/0x6c/0x70/0x7c) keep their facet notes
// below. The 0x150.. region is the CGruntzMgr m_options[4] block (4 x 0x238 ->
// 0xa30); the TUs that reach it (per-player start records @0x150, ref-index array
// @0x158, ...) index it by raw (char*)g_gameReg + offset - the established idiom
// (see Grunt.cpp / GameMode). The reconciled MFC/Win32 sibling view is
// <Gruntz/GameRegistry.h> (CGruntzMgr / g_gameReg).
#ifndef GRUNTZ_GRUNTZ_WWDGAMEREG_H
#define GRUNTZ_GRUNTZ_WWDGAMEREG_H

#include <Ints.h>
#include <rva.h> // SIZE class-metadata macro

// Pointer-member facet types (each consumer completes the ones it dereferences).
class CState;           // +0x2c  current game state (== GameRegistry m_curState; State.h)
class CDDrawSurfaceMgr; // +0x30  the ONE world/resource holder (<DDrawMgr/DDrawSurfaceMgr.h>)
class CGruntSpawnConfig;
typedef CGruntSpawnConfig CGruntCueSink;    // +0x60  on-screen cue receiver
class CGruntzMapMgr;    // +0x70  level tile board (the RTTI-real CMapMgr-derived map mgr)
class CSpriteRefTable;  // +0x74  sprite/animation ref table (GetSel)
class CBattlezData;     // +0x7c  HUD/score + pickup-stat accumulator (<Gruntz/BattlezData.h>)
struct tagRECT;         // GetMessageBounds in/out (== Win32 RECT)

// THE STRUCT IS GONE (2026-07-20): the WwdGameReg mirror's last consumer flipped
// to the real CGruntzMgr. The NAME survives below as retail's own decl spelling
// (?g_gameReg@@3PAUWwdGameReg@@A - retail declared the global through this tag).
struct WwdGameReg; // retail's opaque decl tag; no layout


// One ref-index array slot of the +0x158 array inside that m_options block (an
// 8-byte entry whose first dword is the sprite ref-row index passed to GetSel;
// the FortressFlag tag-8 fixup indexes it by selector-row * 71).
struct WwdRefSlot {
    i32 m_idx; // +0x00  ref-row index (passed to GetSel)
    i32 m_04;  // +0x04
};

#endif // GRUNTZ_GRUNTZ_WWDGAMEREG_H
