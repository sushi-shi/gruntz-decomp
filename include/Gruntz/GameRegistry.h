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
// RESOLVED: +0x74 is CSpriteRefTable* (<Gruntz/SpriteRefTable.h>) - one object,
// RTTI/teardown-proven (Close tears it down via CSpriteRefTable::Reset @0xe2290; the
// GetByIndex/LoadSprite consumer facets are its GetSel/LoadSprite methods, all cast-free).
// RESOLVED: +0x68 is CTriggerMgr* (see the m_cmdGrid fwd-decl block below) - the ~10
// per-TU downcasts were all views of the ONE non-polymorphic CTriggerMgr, proven by
// shared method RVAs (HitTestCell 0x75af0, CellDispatch 0x6bcb0) + the +0x1c cell grid.
// The slots at 0x54/0x58/0x6c/0x78/0x7c are SUSPECT / UN-RECOVERED, NOT
// confirmed-authentic. Today each TU downcasts the void* to a different concrete type
// (+0x7c: a "draw object" in GameMode vs GruntPickupStats
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
// +0x30->+0x28 sound/anim cue registry (CSndHost). Included (not fwd-declared) as the
// lightweight Ints.h-only SoundCue.h: a full def does NOT count toward this ~60-TU
// header's file-scope forward-decl budget, so typing m_28 CSndHost* costs 0 fwd decls
// (see docs/patterns/header-fwd-decl-count-regalloc-butterfly.md - the include-as-lever).
#include <Gruntz/SoundCue.h>

struct CSpriteFactory; // +0x30 -> +0x08 factory (CreateSprite); Grunt.h completes it
class CGruntCueSink;   // +0x60 on-screen cue receiver; Grunt.h completes it (or, in
                       // the pure-Win32 grunt-step TUs that can't pull Grunt.h,
                       // completed locally with just the 0x4039f4 6-arg cue - a
                       // data-less method handle, so layout-neutral, no cross-cast)
class CState;          // +0x2c current game-state; CState.h completes it
struct CInput54;       // +0x54 active-level input/spatial-sound object (InputState.h)
// +0x68 world command/trigger grid. RESOLVED -> CTriggerMgr (<Gruntz/TriggerMgr.h>):
// the ~10 per-TU downcasts (CTeleIconTable/CTriggerSink/CSbIconSet/CSlimeCueGate/
// CPathCueGate/TgcRegion/MgrObj/RbCmdGrid/...) are all VIEWS of the ONE non-polymorphic
// CTriggerMgr. Proven by shared non-virtual method RVAs: the 5-arg Probe/HitTestCell
// == CTriggerMgr::HitTestCell @0x75af0, the 4-arg ScrollTo/Strike/RbMarkRect ==
// CTriggerMgr::CellDispatch @0x6bcb0 (both FID-tagged ?...@CTriggerMgr@@), and the
// +0x1c 15-column grid every array-consumer indexes == CTriggerMgr::m_grid. Forward-
// declared (not included) to keep this ~60-TU header MFC-free (TriggerMgr.h pulls
// <Mfc.h>); consumers include TriggerMgr.h to reach methods cast-free.
class CTriggerMgr;
// Sub-objects of the +0x30 resource manager, defined in <Gruntz/ResMgr.h> /
// <Gruntz/Viewport.h>; forward-declared here so consumers reach them typed
// (no per-site cast) without pulling those headers into this ~60-TU-wide view.
struct CDrawTarget;    // +0x30->+0x04 active draw surface (m_drawContext at +0x14)
struct CImageRegistry; // +0x30->+0x10 image/tile registry (name->sprite map)
// +0x74 sprite/animation reference table (<Gruntz/SpriteRefTable.h>): GetSel(i,bAlt)
// resolves a kind slot to its sprite/frame pointer; LoadSprite(desc,flag) loads by
// descriptor. `new`'d in the game bootstrap (0x83450), torn down by CGruntzMgr::Close
// (CSpriteRefTable::Reset @0xe2290). Forward-declared to keep this ~60-TU view light.
class CSpriteRefTable;
// +0x78 per-frame light-FX / shade-table pump (<Gruntz/LightFxMgr.h>): Push(imgSet,
// anchor, slot) applies a chosen shade table; m_tables[10] (+0x14) is the effect->table
// array consumers index. `new`'d in the bootstrap (0x83450), torn down by Close
// (CLightFxMgr::Reset @0x9dc80). Forward-declared to keep this ~60-TU view light.
class CLightFxMgr;
// Render-facet sub-object types (defined in <Gruntz/View.h>, MFC). This holder is ALSO
// CState::m_c (verified same object; the former `CView`/`CSpriteFactoryHolder` render view is now
// folded here). Its 3 render-only slots below point to these; the MFC state TUs reach the
// 5 SHARED slots (+0x04/+0x08/+0x10/+0x24/+0x28) by casting the canonical members to the
// other View.h facet types (StateMgrBZ/CGameImageRegistry/CDrawSurface/CViewSoundRegistry)
// - the deferred int-vs-pointer sub-object reconciliation (see View.h). Forward-declared so
// this ~60-TU MFC-free header never pulls View.h; the ~60 pure-Win32 m_world consumers
// never touch these render-only slots.
struct CRenderer;     // +0x0c  renderer B (present) / resource worker holder (View.h)
struct CAnimRegistry; // +0x2c  anim/third registry (ResMgr.h, real class)

// The level/view object reached as g->m_30->m_24 (== CState::m_c holder's +0x24): the
// on-screen bar RECT / viewport at +0x10, the world->screen camera geom at +0x5c
// (WrapCoord; its +0x40 is the rect base the on-screen cue gate's visibility helper
// reads). This is the ONE real +0x24 draw-surface/viewport class: the afx-neutral
// menu bar / CGrunt cue path AND the MFC render TUs (CPlay Render draw chain) share it
// - the former per-TU `CDrawSurface` render-facet view (View.h) is folded away here.
struct CGameViewport {
    // Render sub-steps the MFC state TUs drive (external/reloc-masked __thiscall):
    void PushView(void* view, void* renderer); // 0x15dc90
    void PreStep();                            // per-frame view pre-step
    void PostStep();                           // per-frame view post-step
    void SetClipRect(void* r);                 // 0x15da80 (ClampViewport apply-tail; RECT*)

    // +0x10 viewport rect. Named two ways over the same 16 bytes: the afx-neutral menu
    // bar reads the raw i32[4] (m_barRect); the MFC render TUs read the named
    // {left,top,right,bottom} viewport fields.
    struct SViewRect {
        i32 left, top, right, bottom;
    };
    // +0x5c world->screen camera geom (reached as a raw int base by the CGrunt cue
    // helpers via (m_5c + 0x40), cast to CViewport by the menu bar, and to this by the
    // render TUs). +0x40 is the on-screen visible rect; +0x84/+0x88 the world blit src.
    struct CameraGeom {
        char p0[0x40];
        i32 m_originX; // +0x40
        i32 m_originY; // +0x44
        char p48[0x84 - 0x48];
        i32 m_84; // +0x84
        i32 m_88; // +0x88
    };

    char m_pad0[0x10];
    union {
        i32 m_barRect[4]; // +0x10  on-screen bar RECT (left,top,right,bottom)
        SViewRect m_viewport;
    };
    char m_pad20[0x5c - 0x20];
    i32 m_5c; // +0x5c  camera-geom base (int; cast to CameraGeom*/CViewport* at deref)
};

// The +0x30 game resource/level manager (the retail CResMgr; ResMgr.h models the
// same object for the loaders and CPlay). Every view of *0x24556c reaches its
// resources through this one holder: the draw surface (+0x04), the sprite/object
// factory (+0x08, CreateSprite + key lookup), the image registry (+0x10), the
// level/view object (+0x24) and the sound/anim registry (+0x28).
struct CSpriteFactoryHolder {
    char m_pad0[0x4];
    CDrawTarget* m_drawTarget; // +0x04  active draw surface / render-flip pump (CDrawTarget:
                               //         Flush + the frame/draw surface pages; render TUs use it)
    CSpriteFactory* m_8;       // +0x08  sprite/object factory (CreateSprite / key lookup); the
                               //         render facet reaches it as renderer A (cast to CRenderer)
    CRenderer* m_rendererB;    // +0x0c  renderer B (present) / resource worker holder (View.h)
    CImageRegistry* m_10;      // +0x10  image/name registry (real ResMgr.h class: Install/Has/
                               //         Register/Release/LoadNamespace + the m_10map hash)
    char m_pad14[0x20 - 0x14];
    void* m_frameProfiler; // +0x20  frame profiler timer (timeGetTime x2)
    CGameViewport* m_24;   // +0x24  level/view object (bar RECT + viewport); the render facet
                           //         reaches it as the draw surface (cast to CDrawSurface)
    CSndHost* m_28;        // +0x28  sound registry (CSndHost cue facet <Gruntz/SoundCue.h>; the
                           //         render/resource facet reaches it as CSoundRegistry, cast).
                           //         CSndFinder @+0x10 name->CSndEmitter map + the +0x30 emit gate.
    CAnimRegistry* m_animRegistry; // +0x2c  anim/third registry (real ResMgr.h class)
};

// The tile occupancy grid (*g_gameReg+0x70) is CTileGrid, in
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
    i32 m_10; // +0x10  multiplayer roster: combo index base (slot-kind selection;
              //         UpdatePlayers seeds the kind combo with m_10+1)
    i32 m_14; // +0x14  arrival/load gate (grunt step; not-yet-loaded test) / roster:
              //         human-vs-computer flag
    i32 m_18; // +0x18  roster: colour id (compared against CMulti::m_hostIndex)
    i32 m_1c; // +0x1c  roster: ready flag (ToggleReady checkbox state)
    i32 m_20; // +0x20  live/active gate / roster: slot-in-use flag
    i32 m_24; // +0x24  "already cleared this round" mark / timer-expiry flag
    i32 m_28; // +0x28  joined
    i32 m_2c; // +0x2c  done
    char m_pad30[0x164 - 0x30];
    i32 m_164; // +0x164  roster: colour-pick gate (CMultiStartDlg::OnColorSlotN skips the
               //          m_1c/m_18 check when the host set this)
    i32 m_168; // +0x168  roster: colour owner index (compared against CMulti::m_hostIndex)
    i32 m_16c; // +0x16c  roster: colour/lock value (UpdatePlayers reads the LOCAL
               //          slot's m_16c as its per-refresh colour gate)
    char m_pad170[0x220 - 0x170];
    i32 m_220; // +0x220  snapped focus X
    i32 m_224; // +0x224  snapped focus Y
    i32 m_228; // +0x228  roster: combo/selection value (OnSlotSelectN caches sel+1;
               //          UpdatePlayers relays it to SyncColour)
    char m_pad22c[0x238 - 0x22c];

    // Format the player's display name (multiplayer roster; __thiscall @0x3e54 ILT
    // thunk). Returns CString BY VALUE, declared via the elaborated-type-specifier
    // (NO file-scope `class CString;` fwd-decl: this Win32-wide header's fwd-decl
    // COUNT is /O2 type-table state - see header-fwd-decl-count-regalloc-butterfly);
    // CString is complete only in the MFC caller TUs, which is legal for a
    // declaration-only use.
    // FormatName_3e54 @0x3e54 IS GruntzPlayer::GetName; cast at the call.
};

struct CGameRegistry {
    // The entrance-reset cue-prep call (thunk_FUN_0040cd00, __thiscall ret 0): run
    // once before the focused-grunt cue test. External/no-body (reloc-masked).
    void CuePrep();
    // The mode-3 per-frame cue step (thunk_FUN_004933e0, __thiscall): run each
    // world-draw frame when m_134==3. External/no-body (reloc-masked).
    void PerFrameCue();
    class CString
    QueryLevelName(); // 0x928c0 via ILT 0x2531 (level rez path; == CGruntzMgr::GetWorldFileName, same object)
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
    // Same-object methods (== CGruntzMgr's, reloc-masked to the shared RVAs): declared on
    // the canonical view so a TU whose local singleton-view is folded here calls
    // g_gameReg->M() directly instead of cross-casting to an unrelated CGruntzMgr* (the
    // no-sane-dev cross-cast; 0x24556c convergence). Consumers still carrying a per-TU
    // g_gameReg/g_gameReg view (CTmGameReg/RockMgr/LevelSettings/...) must fold onto
    // this canonical first - see the matcher report's CGameRegistry-side worklist.
    CState* PickPausedThenPlayState();        // 0x0929b0
    void EnterModalUI(i32 arg);               // 0x08ef10
    i32 IsBattlezMapFile(class CString path); // (== CGruntzMgr::IsBattlezMapFile)
    i32 ChangeState_8fab0(i32 arg);           // 0x08fab0
    // Win32-safe options/run-state setters (== CGruntzMgr's, reloc-masked to the shared
    // RVAs) - declared on the canonical view so an options/save TU whose singleton-view
    // is folded here calls g_gameReg->M() directly (no cross-cast to CGruntzMgr*).
    void SetRunState(i32 v);              // 0x092340
    void StoreInputFlag(i32 v);           // 0x0919d0
    void StoreInputState(i32 v);          // 0x0919f0
    void SetSoundLevelState(i32 v);       // sound-level state setter
    class CPlay* PickPlayOrPausedState(); // 0x092990 ((CPlay*)FindStateById(3))

    // Slot NAMES agree with <Gruntz/GruntzMgr.h> (the RTTI-true MFC owner) - one field,
    // one name across the dual view (0x24556c convergence, step 1). Named: the base
    // CGameMgr region (m_gameWnd/m_owner/m_frameGate/m_soundEnabled), the manager-owned
    // sub-object pointers (m_curState/m_world/m_settings/m_sound/m_cueSink/m_inputState/
    // m_saveSink/m_cmdGrid/m_cmdSubMgr/m_tileGrid/m_scoreHud/m_spriteFactory/m_logicPump),
    // m_numRuns + the scalar config block. Still m_<off> (role unrecovered): the
    // m_128..m_134 outcome block.
    //   The reused-slot names (m_cmdGrid/m_spriteFactory/m_logicPump/...) are ONE object
    //   each (per no-sane-dev-test): the per-TU downcasts to different concrete types are
    //   the unrecovered-single-type ARTIFACT, not a genuine per-mode union - named by the
    //   common role, still void* until the real class is modeled.
    //   SUBSTANCE-DIVERGENCE FLAGS (one physical field, but the two views disagree on
    //   what it IS - the name unifies to the manager's, endgame to resolve the real
    //   single type):
    //   +0x54 RESOLVED -> CInput54 (<Gruntz/InputState.h>): ONE object (proven by the
    //     LoadWorldMode ctor). The mgr's input facet and the ambient TU's active-level
    //     facet are the same record; +0x24 is the mgr's armed flag == the ambient's
    //     "playable"/object-count gate, +0x08 the shared spatial-sound voice CObList.
    //   +0x60 m_cueSink (cue
    //   receiver, 60+ grunt sites) vs GruntzMgr m_timer (per-frame tick + Voice_Volume);
    //   +0x70 m_tileGrid (tile board) vs GruntzMgr m_cmdNotify (cmd sink + cell-height);
    //   +0x11c/+0x120 (mgr input flags vs consumer sound-volume); +0x14 (base
    //   m_musicEnabled vs GruntzMgr level-loaded gate); +0x150 m_focusSlots (per-player
    //   focus/round state) vs GruntzMgr m_options[4] (registry config records).
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
                                   //         the sprite factory via m_world->m_8). ONE object,
                                   //         two type-views (verified): the retail CResMgr resource
                                   //         facet (ResMgr.h) here, the CWorldZ world facet in
    //         GruntzMgr.h, and CState::m_c's CSpriteFactoryHolder world-holder (View.h)
    char m_pad34[0x38 - 0x34];
    void* m_settings; // +0x38  settings/registry writer (== GruntzMgr m_settings; consumers cast
    //         to RegistryHelper: SetValueDword/LogPos/QueryPos). void* -> cast at use.
    char m_pad3c[0x48 - 0x3c];
    void* m_sound; // +0x48  sound/bank object (== GruntzMgr m_sound, CGruntzSoundZ*)
    char m_pad4c[0x54 - 0x4c];
    CInput54* m_inputState; // +0x54  active-level input/spatial-sound object (ONE object,
                            //         proven: the mgr's input facet == the ambient TU's
                            //         active-level facet, +0x24 armed==playable gate,
                            //         +0x08 CObList spatial voice list; see InputState.h)
    void* m_saveSink;       // +0x58  save-record sink (consumers read save-game progress:
                            //         MenuProgress->m_1c / final-movie availability)
    char m_pad5c[0x60 - 0x5c];
    CGruntCueSink* m_cueSink; // +0x60  on-screen cue receiver (Cue/CueA/CueSpawn;
                              //         GruntzMgr m_timer per-frame poll view)
    char m_pad64[0x68 - 0x64];
    CTriggerMgr*
        m_cmdGrid;     // +0x68  world command/trigger grid (CTriggerMgr, <Gruntz/TriggerMgr.h>).
                       //         RESOLVED (see fwd-decl above): the per-TU downcasts to
                       //         CTeleIconTable/CTriggerSink/CSbIconSet/etc. are all views of the
                       //         ONE CTriggerMgr; its 5-arg HitTestCell @0x75af0 + 4-arg
                       //         CellDispatch @0x6bcb0 are FID-tagged CTriggerMgr methods and its
                       //         +0x1c m_grid is the 15-column cell grid every consumer indexes.
    void* m_cmdSubMgr; // +0x6c  secondary grid/cmd sub-object
    CTileGrid* m_tileGrid; // +0x70  tile occupancy grid + tile-system notifier
                           //         (GruntzMgr m_cmdNotify: cmd sink writes cell heights)
    CSpriteRefTable* m_spriteFactory; // +0x74  sprite/animation ref table (ONE object,
                                      //         RTTI/teardown-proven: LoadSprite in CPlay,
                                      //         GetByIndex==GetSel in InGameIcon; Grunt.h GetSel)
    CLightFxMgr* m_logicPump; // +0x78  light-FX / shade-table pump (ONE object, teardown-proven:
    //         Push@0x9dcb0; m_tables[10]@+0x14 is the effect->table array)
    void* m_scoreHud; // +0x7c  HUD/score accumulator + cmd sink;
                      //         battlez views it as the CBzData score tracker facet.
    i32 m_numRuns;    // +0x80  launch counter "Num_Runs" (== GruntzMgr m_numRuns; CMulti
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
    i32 m_isEasyMode; // +0x118  "Easy_Mode" (hazard gate: m_isEasyMode && m_134==1)
    i32 m_inputFlag; // +0x11c  StoreInputFlag target (FLAG: some consumers read it as sound volume)
    i32 m_inputStateVal; // +0x120  StoreInputState target (FLAG: consumer-side role diverges)
    i32 m_scrollSpeed;   // +0x124  "Scroll_Speed"
    char m_pad128[0x130 - 0x128];
    // +0x130  play-sub-mode gate within active play (m_134==1). Proven behavior: when 0,
    // the RNG runs deterministic/replay-style (CoinFlip), secret-level triggers initialize
    // (CSecretLevelTrigger), and per-level info text shows; when nonzero, a fixed HUD banner
    // (str 0x81a0) shows and RNG goes live. Persisted (SaveState streams it; FillSaveInfo
    // -> record m_f8). No nonzero write survives in the reconstructed code, so its exact
    // identity (replay / recording / special mode) is UNPROVEN - left m_130, not guessed.
    i32 m_130;
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
