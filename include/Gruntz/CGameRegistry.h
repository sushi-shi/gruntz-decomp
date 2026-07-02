// CGameRegistry.h - the one canonical shape of the global game-manager singleton
// (?g_gameReg@@3PAUWwdGameReg@@A, the WwdGameReg* at RVA 0x24556c / VA 0x64556c).
//
// SINGLETON IDENTITY (verified): the object at *0x24556c is the RTTI-confirmed
// CGruntzMgr (vftable ??_7CGruntzMgr@@6B@ @0x5e9b64), `new`'d by CGruntzApp::
// InitializeGameManager (@0x080a20, push 0xa30). CGameRegistry (this struct) and
// CGruntzMgr (<Gruntz/GruntzMgr.h>) are TWO VIEWS OF ONE OBJECT, proven by shared
// method RVAs (CGameRegistry::Ack == CGruntzMgr::ReportError, both @0x08dc60) and
// coincident slot meanings (m_48==m_sound, m_2c==m_curState, m_8c==m_modeW: the
// mode-width cmp [reg+0x8c],0x280 (640) in RestoreVideoMode 0x08ddd0).
//
// WHY TWO HEADERS (a NECESSARY split, not a mistake): CGruntzMgr is an MFC class
// (`: public WAP32::CGameMgr`, CString/CByteArray members) so <Gruntz/GruntzMgr.h>
// pulls <Mfc.h>/afx. THIS header is included by ~60 TUs, many of which are pure-
// Win32 (they `#include <Win32.h>` -> windows.h). afx forbids a prior windows.h
// (`C1189: MFC apps must not #include <windows.h>`), so this canonical view MUST
// stay MFC-free (a plain struct over <Ints.h>+<CTileGrid.h>). The two views cannot
// live in one header without a build break; the field DESCRIPTIONS below are kept
// reconciled with GruntzMgr's descriptive names so there is one agreed layout.
// See docs/vtable-conversion-log.md ("0x24556c dual-view: MFC/Win32 wall").
//
// This object was previously modeled ~20 different ways across the tree (CGameReg,
// WwdGameReg, WwdGameRegZ, CObjDropReg, CGmGameReg, TgcGameReg, ... one bespoke
// partial "view" struct per TU). The USER PRINCIPLE is: different layouts = a
// mistake; there is ONE real object. The leaf SCALAR fields (the ints below) are
// provably consistent across every TU's disasm and are named here.
//
// The SINGLE-TYPE sub-object pointers are typed here so their consumers reach them
// WITHOUT a per-site cast: m_2c (CState* current game-state), m_30 (the resource
// manager - CSpriteFactoryHolder, the retail CResMgr: draw target + sprite factory
// + image registry + view + sound/anim), m_60 (cue sink), m_70 (tile grid). Sub-
// object TYPES defined in <Gruntz/ResMgr.h>/<Gruntz/CViewport.h> are forward-
// declared (not included) to keep this ~60-TU-wide header light.
//
// The REUSED slots at 0x38/0x48/0x58/0x68/0x6c/0x74/0x78/0x7c genuinely hold a
// *different concrete object per game-mode* (e.g. +0x68 is a placement/cue grid in
// single-player, the goo-well mgr in battlez, a light-fx target in the fx TUs; +0x7c
// is the score/HUD sink under several battlez/teleporter facets). Fabricating one
// fake mega-type would be an artifact, not the devs' shape (and a wrong unifying
// type is worse than a documented base + downcast), so those stay void* and each TU
// casts to its own concrete view - a legitimate, AUTHENTIC downcast, not a squeeze
// hack.
#ifndef GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
#define GRUNTZ_GRUNTZ_CGAMEREGISTRY_H

#include <Ints.h>

struct CSpriteFactory; // +0x30 -> +0x08 factory (CreateSprite); Grunt.h completes it
class CGruntCueSink;   // +0x60 on-screen cue receiver; Grunt.h completes it
class CState;          // +0x2c current game-state; CState.h completes it
// Sub-objects of the +0x30 resource manager, defined in <Gruntz/ResMgr.h> /
// <Gruntz/CViewport.h>; forward-declared here so consumers reach them typed
// (no per-site cast) without pulling those headers into this ~60-TU-wide view.
struct CDrawTarget;    // +0x30->+0x04 active draw surface (m_drawContext at +0x14)
struct CImageRegistry; // +0x30->+0x10 image/tile registry (name->sprite map)

// The level/view object reached as g->m_30->m_24: +0x10 the on-screen bar RECT
// (the action/option menu bar), +0x5c the world->screen viewport (WrapCoord;
// its +0x40 is the rect base the on-screen cue gate's visibility helper reads).
// Used by the menu bar and the entrance-reset focused-grunt cue path.
struct CGameViewport {
    char m_pad0[0x10];
    i32 m_barRect[4]; // +0x10  on-screen bar RECT (left,top,right,bottom)
    char m_pad20[0x5c - 0x20];
    // A CViewport* (world->screen transform; base +0x40 = clip rect). Modeled i32
    // because the CGrunt visibility helpers take it as a raw address (int) arg;
    // the menu bar casts it to CViewport at the two sites it calls methods on.
    i32 m_5c; // +0x5c
};

// The +0x30 game resource/level manager (the retail CResMgr; ResMgr.h models the
// same object for the loaders and CPlay). Every view of *0x24556c reaches its
// resources through this one holder: the draw surface (+0x04), the sprite/object
// factory (+0x08, CreateSprite + key lookup), the image registry (+0x10), the
// level/view object (+0x24) and the sound/anim registry (+0x28).
struct CSpriteFactoryHolder {
    char m_pad0[0x4];
    CDrawTarget* m_drawTarget; // +0x04  active draw surface
    CSpriteFactory* m_8;       // +0x08  sprite/object factory (CreateSprite / key lookup)
    char m_pad0c[0x10 - 0xc];
    CImageRegistry* m_10; // +0x10  image/tile registry (name->sprite map)
    char m_pad14[0x24 - 0x14];
    CGameViewport* m_24; // +0x24  level/view object (bar RECT + viewport)
    void* m_28;          // +0x28  sound/anim registry (per-TU view)
};

// The tile occupancy grid (*g_pGameRegistry+0x70) is CTileGrid, in
// <Gruntz/CTileGrid.h>. CGrunt::LoadEntranceConfig stamps the grunt's footprint
// into the cell occupied by (m_10->m_5c>>5, m_10->m_60>>5): sets/clears bit
// 0x20 in cell byte+3 and writes a packed (m_1ec<<8)|m_1f0 owner word into cell[1].
#include <Gruntz/CTileGrid.h>

struct CGameRegistry {
    // The entrance-reset cue-prep call (thunk_FUN_0040cd00, __thiscall ret 0): run
    // once before the focused-grunt cue test. External/no-body (reloc-masked).
    void CuePrep();
    // The mode-3 per-frame cue step (thunk_FUN_004933e0, __thiscall): run each
    // world-draw frame when m_134==3. External/no-body (reloc-masked).
    void PerFrameCue();
    // Registry service methods some TUs call directly on the singleton
    // (external/no-body, reloc-masked rel32 callees).
    void Ack(i32 line, i32 code);                               // 0x8dc60 switch-logic ack
    i32 BuildLevelRezPath(i32 isEmpty, i32 hi, i32 lo, i32 id); // save-game rez-path builder
    void LogError(const char* msg);                             // 0x404178 save-game error notifier
    void EmitEvent(i32 a, i32 b);                               // hazard event emitter
    i32 Rand();                                                 // game-mgr RNG
    i32 RandRange(i32 lo, i32 hi);                              // game-mgr RNG range
    i32 Report(i32 a, i32 b);          // diagnostic reporter (return often discarded)
    void ReportError(const char* msg); // plane/scan error notifier
    i32 RunModalDialog(const char* tmpl, void* proc, i32 flag); // modal dialog runner
    void* GetRect(void* buf); // dev-stats bounds query (RECT* buf/ret)

    // Field comments carry the GruntzMgr descriptive name (== <name>) after the
    // offset - the reconciled "real" names; see <Gruntz/GruntzMgr.h> (CGruntzMgr)
    // for the fully-typed MFC view. Terse m_<off> names are kept because ~60 TUs
    // (incl. pure-Win32 ones) reference them and stay untouched.
    char m_pad0[0x4]; // +0x00  CGameMgr vptr slot (base ??_7CGameMgr@@6B@)
    void* m_4;        // +0x04  window/host sub-object
    void* m_8;        // +0x08
    void* m_c;        // +0x0c  active-selection / busy gate (CanQuickSave)
    i32 m_10;         // +0x10  base m_10 run-state (level/state discriminator)
    i32 m_14;         // +0x14  base m_14 run-state (has-window / level-loaded gate)
    char m_pad18[0x2c - 0x18];
    CState* m_2c;               // +0x2c  == m_curState (current game-state; concrete
                                //         states downcast to their play/level view)
    CSpriteFactoryHolder* m_30; // +0x30  == m_world (world/map; grunt reaches the
                                //         sprite factory via m_30->m_8)
    char m_pad34[0x38 - 0x34];
    void* m_38; // +0x38  registry-helper / logger sub-object (grunt view)
    char m_pad3c[0x48 - 0x3c];
    void* m_48; // +0x48  == m_sound (sound/bank object)
    char m_pad4c[0x54 - 0x4c];
    void* m_54; // +0x54  == m_inputState (grunt: active-world view)
    void* m_58; // +0x58  == m_saveSink (grunt: progress / notifier)
    char m_pad5c[0x60 - 0x5c];
    CGruntCueSink* m_60; // +0x60  == m_timer (grunt: cue sink / ->Cue receiver)
    char m_pad64[0x68 - 0x64];
    void* m_68;      // +0x68  == m_cmdGrid. REUSED slot (authentic per-mode downcast):
                     //         placement/cue grid (CGruntRec**) in single-player, the
                     //         goo-well mgr in battlez, a light-fx target in the fx TUs.
    void* m_6c;      // +0x6c  == m_cmdSubMgr (secondary grid/cmd sub-object)
    CTileGrid* m_70; // +0x70  == m_cmdNotify (tile occupancy grid + tile-system
                     //         notifier; the cmd sink writes cell heights into it)
    void* m_74;      // +0x74  sprite factory / ref-table (BeginGridWalk retry path)
    void* m_78;      // +0x78  sub-object (per-TU view)
    void* m_7c;      // +0x7c  == m_scoreHud (HUD/score accumulator + cmd sink);
                     //         battlez views it as the CBzData score tracker facet.
    char m_pad80[0x8c - 0x80];
    i32 m_8c; // +0x8c  == m_modeW (live video-mode width; cmp ...,0x280==640)
    i32 m_90; // +0x90  == m_modeH (live video-mode height; ==480)
    i32 m_94; // +0x94  == m_savedModeW (last-good mode width)
    i32 m_98; // +0x98  == m_savedModeH (last-good mode height)
    char m_pad9c[0x100 - 0x9c];
    i32 m_100; // +0x100  == m_isVoiceEnabled ("Voice")
    char m_pad104[0x10c - 0x104];
    i32 m_10c; // +0x10c  == m_isHighDetail ("High_Detail")
    i32 m_110; // +0x110  == m_isEffectsEnabled ("Effects")
    char m_pad114[0x118 - 0x114];
    i32 m_118; // +0x118  == m_isEasyMode ("Easy_Mode")
    i32 m_11c; // +0x11c  == m_inputFlag
    i32 m_120; // +0x120  == m_inputStateVal
    i32 m_124; // +0x124  == m_scrollSpeed ("Scroll_Speed")
    char m_pad128[0x130 - 0x128];
    i32 m_130; // +0x130  (m_128..m_134 game-outcome block; m_134==3 -> "won")
    i32 m_134; // +0x134  gate/outcome discriminator (grunt: ==1 visible-bounds,
               //         ==2 alt path; GruntzMgr: ==3 "won")
    char m_pad138[0x13c - 0x138];
    i32 m_13c; // +0x13c  == m_viewOriginL (view min X)
    i32 m_140; // +0x140  == m_viewOriginT (view min Y)
    i32 m_144; // +0x144  == m_viewOriginR (view max X)
    i32 m_148; // +0x148  == m_viewOriginB (view max Y)
    char m_pad14c[0x158 - 0x14c];
    // 0x150.. is the CGruntzMgr m_options[4] (4 x 0x238 options/registry slots ->
    // 0xa30 total); grunt TUs view individual scalars inside it:
    i32 m_158;   // +0x158  base of a per-player / world slot table (in m_options)
    void* m_15c; // +0x15c  level/entity-tree holder (in m_options[0])
    char m_pad160[0x164 - 0x160];
    i32 m_164; // +0x164
    char m_pad168[0x170 - 0x168];
    i32 m_170; // +0x170
};

#endif // GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
