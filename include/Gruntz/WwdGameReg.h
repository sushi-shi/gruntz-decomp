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
struct GruntBoard;      // +0x70  level tile board
class CSpriteRefTable;  // +0x74  sprite/animation ref table (GetSel)
class CBattlezData;     // +0x7c  HUD/score + pickup-stat accumulator (<Gruntz/BattlezData.h>)
struct tagRECT;         // GetMessageBounds in/out (== Win32 RECT)

SIZE(WwdGameReg, 0xa30);
struct WwdGameReg {
    // Registry service methods (all external / no-body, reloc-masked rel32 callees).
    i32 Rand();                              // FUN_0040cd00  signed game RNG
    i32 RandRange(i32 lo, i32 hi);           // FUN_00419f50  uniform [lo,hi) (ret 8)
    void LogTileError(const char* msg);      // 0x48ef10 (via 0x417e thunk)
    tagRECT* GetMessageBounds(tagRECT* out); // 0x2cb1 thunk, on-screen message bounds

    char m_pad0[0x10];
    i32 m_soundEnabled; // +0x10  sound-on flag (== the CGameMgr base's m_soundEnabled;
                        //         ex "presence gate" - the combat cue paths gate on it)
    char m_pad14[0x2c - 0x14];
    CState* m_2c;              // +0x2c  current game state (curState; the MEGAPHONE unit-count path
                               //         reads its +0x2dc sub-object; == GameRegistry m_curState)
    CDDrawSurfaceMgr* m_world; // +0x30  the ONE world/resource holder (canonical;
                               //         ex the GruntSoundCat/CTeleResHolder facet views)
    char m_pad34[0x60 - 0x34];
    CGruntCueSink* m_cueSink; // +0x60  on-screen cue receiver (grunt facet; UserLogic
                              //         downcasts to CTeleCueSink)
    char m_pad64[0x68 - 0x64];
    i32 m_68;   // +0x68  reused slot (cast: CGruntTileMgr* / PlaceGridMgr* / CTriggerProbe*)
    class CGruntzCmdMgr* m_6c; // +0x6c  the cmd sub-mgr (twin of CGameRegistry
                               //        m_cmdSubMgr; the StartCmdMgr casts are its facet)
    GruntBoard* m_tileGrid;   // +0x70  level tile board (grunt facet; cast: WwdGameGrid*)
    CSpriteRefTable* m_74;    // +0x74  sprite/animation ref table (GetSel)
    class CLightFxMgr* m_78;  // +0x78  the logic pump (twin of CGruntzMgr m_logicPump)
    CBattlezData* m_scoreHud; // +0x7c  (same slot/name as the CGameRegistry sibling view;
                              //  the former GruntPickupStats/WwdGameRegInner/Aux views of it
                              //  are dissolved - the pickup-stat bands are its +0xd8.. arrays)
    char m_pad80[0x118 - 0x80];
    i32 m_isEasyMode; // +0x118  "Easy_Mode"
    i32 m_11c;        // +0x11c  sound-channel / configure-item param
    char m_pad120[0x130 - 0x120];
    i32 m_130; // +0x130
    i32 m_134; // +0x134  view-cull / place-mode / game-outcome gate
    char m_pad138[0x13c - 0x138];
    // +0x13c..+0x148: the visible view-edge origins (same names/offsets as the
    // CGameRegistry / CGruntzMgr sibling views; the combat on-screen gate reads them).
    i32 m_viewOriginL; // +0x13c  view min X
    i32 m_viewOriginT; // +0x140  view min Y
    i32 m_viewOriginR; // +0x144  view max X
    i32 m_viewOriginB; // +0x148  view max Y
    // 0x150.. is the CGruntzMgr m_options[4] block; reach it by raw offset.
};

// One ref-index array slot of the +0x158 array inside that m_options block (an
// 8-byte entry whose first dword is the sprite ref-row index passed to GetSel;
// the FortressFlag tag-8 fixup indexes it by selector-row * 71).
struct WwdRefSlot {
    i32 m_idx; // +0x00  ref-row index (passed to GetSel)
    i32 m_04;  // +0x04
};

#endif // GRUNTZ_GRUNTZ_WWDGAMEREG_H
