// GameRegistry.h - the one canonical shape of the global game-manager singleton
// (?g_gameReg@@3PAUWwdGameReg@@A, the WwdGameReg* at RVA 0x24556c / VA 0x64556c).
//
// SINGLETON IDENTITY (verified): the object at *0x24556c is the RTTI-confirmed
// CGruntzMgr (vftable ??_7CGruntzMgr@@6B@ @0x5e9b64), `new`'d by CGruntzApp::
// InitializeGameManager (@0x080a20, push 0xa30). CGameRegistry (this struct) and
// CGruntzMgr (<Gruntz/GruntzMgr.h>) are TWO VIEWS OF ONE OBJECT, proven by shared
// method RVAs (CGameRegistry::Ack == CGruntzMgr::ReportError, both @0x08dc60) and
// coincident slot meanings (+0x48 m_sound, m_2c m_curState, m_8c m_modeW: the
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
// object TYPES defined in <Gruntz/ResMgr.h>/<Gruntz/Viewport.h> are forward-
// declared (not included) to keep this ~60-TU-wide header light.
//
// The slots at 0x54/0x58/0x68/0x6c/0x74/0x78/0x7c are SUSPECT / UN-RECOVERED, NOT
// confirmed-authentic. Today each TU downcasts the void* to a different concrete type
// (+0x68: a placement/cue grid in single-player, the goo-well mgr in battlez, a
// light-fx SURFACE in the fx TUs; +0x7c: a "draw object" in GameMode vs GruntPickupStats
// in the pickup loader). That per-TU divergence is a RED FLAG per no-sane-dev-test: a
// real CGruntzMgr field has ONE type, so "a different type per TU" almost always means
// the real single type - or the common base the mode objects derive - was never
// recovered, and the void*-plus-downcast is a reconstruction ARTIFACT, not a genuine
// per-mode union. These stay void* ONLY because the real type isn't recovered yet. The
// fix (do NOT defend the void*): trace what the retail CGruntzMgr ctor + per-mode init
// actually store at each offset - a single type, or a real base to derive - then type
// it here so consumers reach it cast-free, exactly like m_curState/m_world/m_tileGrid.
#ifndef GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
#define GRUNTZ_GRUNTZ_CGAMEREGISTRY_H

#include <Ints.h>

struct CSpriteFactory; // +0x30 -> +0x08 factory (CreateSprite); Grunt.h completes it
class CGruntCueSink;   // +0x60 on-screen cue receiver; Grunt.h completes it (or, in
                       // the pure-Win32 grunt-step TUs that can't pull Grunt.h,
                       // completed locally with just the 0x4039f4 6-arg cue - a
                       // data-less method handle, so layout-neutral, no cross-cast)
class CState;          // +0x2c current game-state; CState.h completes it
// Sub-objects of the +0x30 resource manager, defined in <Gruntz/ResMgr.h> /
// <Gruntz/Viewport.h>; forward-declared here so consumers reach them typed
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
// <Gruntz/TileGrid.h>. CGrunt::LoadEntranceConfig stamps the grunt's footprint
// into the cell occupied by (m_10->m_5c>>5, m_10->m_60>>5): sets/clears bit
// 0x20 in cell byte+3 and writes a packed (m_1ec<<8)|m_1f0 owner word into cell[1].
#include <Gruntz/TileGrid.h>

// One per-player focus/registry slot: the element of the +0x150 array (4 records
// of 0x238 bytes; == the retail CGruntzMgr m_options[4]). It is ONE real record
// whose fields are REUSED per game-mode - a same-struct field overlay, not a set
// of distinct types (unlike the +0x68/+0x7c object slots which hold genuinely
// different classes). The fields named here are the offsets the game-mode
// consumers read: the two arrival/active gates (+0x14, +0x20), the join/done/
// cleared round-state trio (+0x24/+0x28/+0x2c), the per-mode id/sound/key word at
// +0x0c, and the snapped focus position (+0x220/+0x224). The role of m_0c varies
// by mode (sound id in battlez, entity id in the exit trigger, a key pointer in
// the sprite loader) so it is a plain i32 the pointer-consumer reinterprets.
struct CFocusSlot {
    char m_pad0[0x0c];
    i32 m_0c; // +0x0c  per-mode id / sound id / key word
    char m_pad10[0x14 - 0x10];
    i32 m_14; // +0x14  arrival/load gate (grunt step; not-yet-loaded test)
    char m_pad18[0x20 - 0x18];
    i32 m_20; // +0x20  live/active gate
    i32 m_24; // +0x24  "already cleared this round" mark / timer-expiry flag
    i32 m_28; // +0x28  joined
    i32 m_2c; // +0x2c  done
    char m_pad30[0x220 - 0x30];
    i32 m_220; // +0x220  snapped focus X
    i32 m_224; // +0x224  snapped focus Y
    char m_pad228[0x238 - 0x228];
};

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
    void StopBankIfActive();  // 0x92000 (== CGruntzMgr::StopBankIfActive; sound-bank stop)
    void StopBank0IfActive(); // 0x92030 (== CGruntzMgr::StopBank0IfActive; bank-0 stop)
    // The status-bar HUD reaches these score/level methods on the singleton (== the
    // MFC-side CGruntzMgr view of *0x24556c; reloc-masked, the same-object dual-view).
    // Re-homed here from the former per-TU SBI CGameReg facet's Fn29aa/HiPump/SetToggle.
    void UpdateScoreHud();             // 0x0860b0 (== CGruntzMgr::UpdateScoreHud)
    void AccrueScoreTime();            // 0x0861e0 (== CGruntzMgr::AccrueScoreTime)
    i32 FinishLevel(i32 a, i32 b);     // 0x08e980 (== CGruntzMgr::FinishLevel)
    void ReportError(i32 id, i32 tag); // status-bar activation-fail report (i32,i32 overload)

    // Well-understood slots are named (the base CGameMgr region m_gameWnd/m_owner/
    // m_frameGate/m_soundEnabled, the four single-type pointers m_curState/m_world/
    // m_cueSink/m_tileGrid, m_settings/m_sound/m_numRuns + the scalar config block);
    // the genuinely reused per-mode void* slots (0x54/58/68/6c/74/78/7c) and the per-TU
    // outcome discriminator (0x134) keep m_<off> - their concrete role differs per
    // game-mode/TU, so one name would mislead. m_11c/m_120 and m_14 also stay m_<off>:
    // their manager-side name (GruntzMgr input flags / m_musicEnabled) conflicts with
    // the consumer-side use (sound volume / level-loaded gate) - flagged, not guessed.
    char m_pad0[0x4];   // +0x00  CGameMgr vptr slot (base ??_7CGameMgr@@6B@)
    void* m_gameWnd;    // +0x04  bound game window (base CGameMgr::m_gameWnd; window/host)
    void* m_owner;      // +0x08  owning app (base CGameMgr::m_owner)
    i32 m_frameGate;    // +0x0c  nonzero suppresses per-frame advance / busy-pause gate
                        //         (base CGameMgr::m_frameGate; toggled, CanQuickSave gate)
    i32 m_soundEnabled; // +0x10  sound-on flag (base CGameMgr::m_soundEnabled "Sound"; every
                        //         ambient/goo consumer guards its sound work on it)
    i32 m_14;           // +0x14  base names it m_musicEnabled ("Music"); GruntzMgr uses it as a
                        //         level-loaded gate (StopBankIfActive) - role unresolved, kept m_14
    char m_pad18[0x2c - 0x18];
    CState* m_curState;            // +0x2c  current game-state (concrete states
                                   //         downcast to their play/level view)
    CSpriteFactoryHolder* m_world; // +0x30  world/map resource holder (grunt reaches
                                   //         the sprite factory via m_world->m_8)
    char m_pad34[0x38 - 0x34];
    void* m_settings; // +0x38  settings/registry writer (== GruntzMgr m_settings; consumers cast
    //         to RegistryHelper: SetValueDword/LogPos/QueryPos). void* -> cast at use.
    char m_pad3c[0x48 - 0x3c];
    void* m_sound; // +0x48  sound/bank object (== GruntzMgr m_sound, CGruntzSoundZ*)
    char m_pad4c[0x54 - 0x4c];
    void* m_54; // +0x54  == m_inputState (grunt: active-world view)
    void* m_58; // +0x58  == m_saveSink (grunt: progress / notifier)
    char m_pad5c[0x60 - 0x5c];
    CGruntCueSink* m_cueSink; // +0x60  on-screen cue receiver (Cue/CueA/CueSpawn;
                              //         GruntzMgr m_timer per-frame poll view)
    char m_pad64[0x68 - 0x64];
    void* m_68;            // +0x68  == m_cmdGrid. REUSED slot (authentic per-mode downcast):
                           //         placement/cue grid (CGruntRec**) in single-player, the
                           //         goo-well mgr in battlez, a light-fx target in the fx TUs.
    void* m_6c;            // +0x6c  == m_cmdSubMgr (secondary grid/cmd sub-object)
    CTileGrid* m_tileGrid; // +0x70  tile occupancy grid + tile-system notifier
                           //         (GruntzMgr m_cmdNotify: cmd sink writes cell heights)
    void* m_74;            // +0x74  sprite factory / ref-table (BeginGridWalk retry path)
    void* m_78;            // +0x78  sub-object (per-TU view)
    void* m_7c;            // +0x7c  == m_scoreHud (HUD/score accumulator + cmd sink);
                           //         battlez views it as the CBzData score tracker facet.
    i32 m_numRuns;         // +0x80  launch counter "Num_Runs" (== GruntzMgr m_numRuns; CMulti
                           //         varies the attract title screen by m_numRuns % N + 1)
    char m_pad84[0x8c - 0x84];
    i32 m_modeW;      // +0x8c  live video-mode width (cmp ...,0x280==640)
    i32 m_modeH;      // +0x90  live video-mode height (==480)
    i32 m_savedModeW; // +0x94  last-good mode width
    i32 m_savedModeH; // +0x98  last-good mode height
    char m_pad9c[0x100 - 0x9c];
    i32 m_isVoiceEnabled; // +0x100  "Voice"
    char m_pad104[0x10c - 0x104];
    i32 m_isHighDetail;     // +0x10c  "High_Detail"
    i32 m_isEffectsEnabled; // +0x110  "Effects"
    char m_pad114[0x118 - 0x114];
    i32 m_isEasyMode;  // +0x118  "Easy_Mode" (hazard gate: m_isEasyMode && m_134==1)
    i32 m_11c;         // +0x11c  == m_inputFlag
    i32 m_120;         // +0x120  == m_inputStateVal
    i32 m_scrollSpeed; // +0x124  "Scroll_Speed"
    char m_pad128[0x130 - 0x128];
    i32 m_130; // +0x130  (m_128..m_134 game-outcome block; m_134==3 -> "won")
    i32 m_134; // +0x134  gate/outcome discriminator (grunt: ==1 visible-bounds,
               //         ==2 alt path; GruntzMgr: ==3 "won")
    char m_pad138[0x13c - 0x138];
    i32 m_viewOriginL; // +0x13c  view min X
    i32 m_viewOriginT; // +0x140  view min Y
    i32 m_viewOriginR; // +0x144  view max X
    i32 m_viewOriginB; // +0x148  view max Y
    char m_pad14c[0x150 - 0x14c];
    // The per-player focus/registry slot array (== CGruntzMgr m_options[4]): 4
    // records of 0x238 bytes -> 0xa30 total, making sizeof(CGameRegistry) exactly
    // the retail `new` size 0xa30 (CGruntzApp::InitializeGameManager push 0xa30).
    // Consumers index it cast-free via m_focusSlots[k].
    CFocusSlot m_focusSlots[4]; // +0x150  stride 0x238
};

#endif // GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
