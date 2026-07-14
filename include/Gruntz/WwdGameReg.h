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
// The slots at 0x30/0x60/0x68/0x6c/0x70/0x74/0x7c genuinely hold a *different
// concrete object per game-mode/facet*. The grunt facet (Grunt.cpp, by far the
// heaviest consumer) types them (m_world=GruntSoundCat, m_cueSink=CGruntCueSink,
// m_tileGrid=GruntBoard, m_74=CSpriteRefTable, m_68=int-cast-to-CGruntTileMgr), so
// those types are kept here; the other TUs reach their own facet with an authentic
// per-mode downcast (e.g. UserLogic: (CTeleResHolder*)m_world, (CTeleCueSink*)
// m_cueSink, (CTriggerProbe*)m_68). The 0x150.. region is the CGruntzMgr
// m_options[4] block (4 x 0x238 -> 0xa30); the TUs that reach it (per-player start
// records @0x150, ref-index array @0x158, ...) index it by raw (char*)g_gameReg +
// offset - the established idiom (see Grunt.cpp / GameMode). The reconciled MFC/
// Win32 sibling view is <Gruntz/GameRegistry.h> (CGruntzMgr / g_gameReg).
#ifndef GRUNTZ_GRUNTZ_WWDGAMEREG_H
#define GRUNTZ_GRUNTZ_WWDGAMEREG_H

#include <Ints.h>
#include <rva.h> // SIZE class-metadata macro

// Pointer-member facet types (each consumer completes the ones it dereferences).
struct GruntSoundCat;  // +0x30  grunt sound-category / resource holder facet
class CGruntCueSink;   // +0x60  on-screen cue receiver
struct GruntBoard;     // +0x70  level tile board
class CSpriteRefTable; // +0x74  sprite/animation ref table (GetSel)
struct tagRECT;        // GetMessageBounds in/out (== Win32 RECT)

SIZE(WwdGameReg, 0xa30);
struct WwdGameReg {
    // Registry service methods (all external / no-body, reloc-masked rel32 callees).
    i32 Rand();                              // FUN_0040cd00  signed game RNG
    i32 RandRange(i32 lo, i32 hi);           // FUN_00419f50  uniform [lo,hi) (ret 8)
    void LogTileError(const char* msg);      // 0x48ef10 (via 0x417e thunk)
    tagRECT* GetMessageBounds(tagRECT* out); // 0x2cb1 thunk, on-screen message bounds

    char m_pad0[0x10];
    i32 m_10; // +0x10  presence gate
    char m_pad14[0x30 - 0x14];
    GruntSoundCat* m_world; // +0x30  resource/sound-category holder (grunt facet;
                            //         other TUs downcast: CTeleResHolder / active gate)
    char m_pad34[0x60 - 0x34];
    CGruntCueSink* m_cueSink; // +0x60  on-screen cue receiver (grunt facet; UserLogic
                              //         downcasts to CTeleCueSink)
    char m_pad64[0x68 - 0x64];
    i32 m_68;   // +0x68  reused slot (cast: CGruntTileMgr* / PlaceGridMgr* / CTriggerProbe*)
    void* m_6c; // +0x6c  reused slot (cast: StartCmdMgr*)
    GruntBoard* m_tileGrid; // +0x70  level tile board (grunt facet; cast: WwdGameGrid*)
    CSpriteRefTable* m_74;  // +0x74  sprite/animation ref table (GetSel)
    void* m_78;             // +0x78
    void* m_7c; // +0x7c  reused slot (cast: WwdGameRegInner* / WwdGameRegAux* / CBzData*)
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

#endif // GRUNTZ_GRUNTZ_WWDGAMEREG_H
