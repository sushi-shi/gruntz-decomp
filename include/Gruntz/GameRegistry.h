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
// object TYPES defined in <Gruntz/ResMgr.h>/<Wwd/WwdFile.h> are forward-
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
class CWorldSoundSet;  // +0x54 active-level input/spatial-sound object (WorldSoundSet.h)
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
class CBattlezData; // +0x7c the HUD/score accumulator (BattlezData.h completes it)
// Sub-objects of the +0x30 resource manager, defined in <Gruntz/ResMgr.h> /
// <Wwd/WwdFile.h> (CPlaneRender); forward-declared here so consumers reach them typed
// (no per-site cast) without pulling those headers into this ~60-TU-wide view.
struct CDrawTarget; // +0x30->+0x04 active draw surface (m_drawContext at +0x14)
// The image/name registry IS the canonical CDDrawWorkerRegistry
// (<DDrawMgr/DDrawWorkerRegistry.h>, real polymorphic; ex CWorkerVtableView).
class CDDrawWorkerRegistry;
typedef CDDrawWorkerRegistry CImageRegistry;
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
// +0x0c renderer B IS the real CDDrawWorkerList (<DDrawMgr/DDrawWorkerList.h>,
// vtable 0x1efd88): CDDrawSurfaceMgr::Init (0x155900) news it (0x2c B) and stores
// it at +0x0c; the play states' per-frame "Present" is its slot 13, PruneWorkers.
// (The former View.h `CRenderer` view - and its 12 fabricated filler slots - is
// dissolved 2026-07-14; renderer A at +0x08 was a SECOND class all along, the
// already-canonical CDDrawChildGroup.)
class CDDrawWorkerList;
struct CAnimRegistry; // +0x2c  anim/third registry (ResMgr.h, real class)

// The level/view object reached as g->m_30->m_24 (== CState::m_c holder's +0x24) IS
// the canonical CGameLevel (<Gruntz/GameLevel.h>) - the former `CGameViewport` facet
// here was a FAKE NAME for it, proven by the retail call targets: "PushView" is
// ?VisitVisible@CGameLevel@@ @0x15dc90, "SetClipRect" is ?BuildAllPlanes@CGameLevel@@
// @0x15da80 (both direct rel32, GameLevel.cpp), the "+0x10 viewport rect" is
// CGameLevel::m_planeCtx (the LevelCoordRect the plane rebuild consumes) and the
// "+0x5c camera geom" is CGameLevel::m_mainPlane (a CLevelPlane: tile origin at
// +0x40/+0x44, integer scroll origin at +0x84/+0x88). The two fabricated
// PreStep/PostStep methods (phantoms - no retail address) are gone with it.
// Forward-declared: the deref TUs include <Gruntz/GameLevel.h>.
class CGameLevel;

// The +0x30 game resource/level manager (the retail CResMgr; ResMgr.h models the
// same object for the loaders and CPlay). Every view of *0x24556c reaches its
// resources through this one holder: the draw surface (+0x04), the sprite/object
// factory (+0x08, CreateSprite + key lookup), the image registry (+0x10), the
// level/view object (+0x24) and the sound/anim registry (+0x28).
class SoundStream; // +0x20 (Dsndmgr; the per-frame sound tick)
// [SETTLED IDENTITY (Fable lane, 2026-07-13): this class IS CDDrawSurfaceMgr
// (<DDrawMgr/DDrawSurfaceMgr.h>, ??_7 @0x1efc58, SIZE 0x40) - the two canonicals
// agree member-for-member: +0x20 both named m_soundStream, +0x24 both CGameLevel,
// +0x28 CSndHost==CDDrawSubMgrLeafScan (settled), and the pairs m_drawTarget==
// m_pages / m_8(CSpriteFactory)==m_childGroup / m_10(CImageRegistry)==m_surfaceDesc /
// m_animRegistry==m_leaf are per-slot identities of the same children. The CLASS
// merge (one definition) is deferred - it must not emit two vtables for the one
// retail ??_7; this game-side view stays vptr-padded (m_pad0) until then.]
struct CSpriteFactoryHolder {
    char m_pad0[0x4]; // the vptr (the real class is polymorphic - see the note above)
    // +0x04: one child, two established names (CDrawTarget the game-side reading;
    // CDDrawSubMgrPages the DDraw-side canonical - Method_158c70 pause, m_backPair).
    union {
        CDrawTarget* m_drawTarget;
        struct CDDrawSubMgrPages* m_pages;
    };
    // +0x08: the sprite/object factory == the DDraw child-group == CWwdObjMgr (the
    // three-way class identity is documented in SpriteFactory.h / WwdObjMgr.cpp).
    union {
        CSpriteFactory* m_8;
        struct CDDrawChildGroup* m_childGroup;
    };
    // +0x0c  renderer B: the real CDDrawWorkerList (per-frame worker pump; "Present"
    // == its slot-13 PruneWorkers; ClearWorkers is the leaf-state teardown).
    CDDrawWorkerList* m_rendererB;
    CImageRegistry* m_10; // +0x10  image/name registry (real ResMgr.h class: Install/Has/
                          //         Register/Release/LoadNamespace + the m_10map hash)
    // +0x14..+0x1c: the DDraw-side children (names from the CDDrawSurfaceMgr
    // canonical; the Init decode 0x155900 news each).
    struct CDDrawWorkerCache* m_workerCache; // +0x14  string-keyed worker cache
    class CLoadable* m_workerMap;            // +0x18  CDDrawWorkerMapSmall (CLoadable child)
    struct CDDrawPtrCollections* m_ptrColl;  // +0x1c  device/surface pool (m_surf0 =
                                             //        the IDirectDraw2; GetCapsChecked)
    // +0x20  the REAL SoundStream (<Dsndmgr/SoundStream.h>). Was "m_frameProfiler": the
    // two per-frame `(obj, timeGetTime())` calls on it are SoundDevice::PurgeVoiceList
    // (0x136e20) + SoundStream::TickSubManagers (0x137ac0), BOTH 100%% EXACT in the tree -
    // a sound tick, not a profiler.
    SoundStream* m_soundStream;
    CGameLevel* m_24; // +0x24  the level object (canonical CGameLevel: planeCtx rect,
                      //         main plane, VisitVisible/BuildAllPlanes render steps)
    CSndHost* m_28;   // +0x28  sound registry (CSndHost cue facet <Gruntz/SoundCue.h>; the
                      //         render/resource facet reaches it as CSoundRegistry, cast).
                      //         CSndFinder @+0x10 name->CSndEmitter map + the +0x30 emit gate.
    CAnimRegistry* m_animRegistry; // +0x2c  anim/third registry (real ResMgr.h class;
                                   //         == CDDrawSurfaceMgr::m_leaf)
    void* m_hWnd30;                // +0x30  bound window / device handle (CDDrawSurfaceMgr::m_hWnd)
    i32 m_flags34;                 // +0x34  caps flags
    // +0x38  last-error / world load-status code (ReportWorldStatus maps it to a
    // message id; Init stores 0x3e9..0x3f1). u32 alias = the old CWorldZ::m_38.
    union {
        i32 m_lastError;
        u32 m_38;
    };
    void* m_callback3c; // +0x3c  run/config callback (HP_Callback)
};
SIZE(CSpriteFactoryHolder, 0x40); // == SIZE(CDDrawSurfaceMgr, 0x40)

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
    char m_pad0[0x8];
    i32 m_08; // +0x08  per-owner sprite-selector config row (CWarlord ctor reads it,
              //         clamps to [0,0x11), and feeds CSpriteRefTable::GetSel)
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
    i32 m_22c; // +0x22c  roster: per-slot display value (Watchdog reads it into the "%d"
               //          status readout when the slot is active+present)
    char m_pad230[0x238 - 0x230];

    // Format the player's display name (multiplayer roster; __thiscall @0x3e54 ILT
    // thunk). Returns CString BY VALUE, declared via the elaborated-type-specifier
    // (NO file-scope `class CString;` fwd-decl: this Win32-wide header's fwd-decl
    // COUNT is /O2 type-table state - see header-fwd-decl-count-regalloc-butterfly);
    // CString is complete only in the MFC caller TUs, which is legal for a
    // declaration-only use.
    // FormatName_3e54 @0x3e54 IS GruntzPlayer::GetName; cast at the call.
};

// PHANTOM PURGE (this batch): BuildLevelRezPath / LogError / RunModalDialog / GetRect /
// EnterModalUI are GONE from this view. Every one was a mangled name (?X@CGameRegistry@@..)
// that no obj and no .LIB can ever define - while the RVA each call actually targets is a
// REAL CGruntzMgr method (0x93d40 / 0x8ef10 / 0x90260 / 0x8e3a0 / 0x8ef10, read off the
// call sites' rel32). LogError turned out to BE EnterModalUI (one function, two fake
// names). Their callers (savegame, loadgamemenu, customworlddialog, drawdebugstats,
// groupops) now declare the singleton at its REAL type, CGruntzMgr* - which is legal in
// any TU (GruntzMgr.h already includes THIS header, and extern "C" gives the pointer one
// C symbol whatever type a TU picks). The rest of this view's methods are the same defect
// and go the same way as their callers convert; see the matcher report for the blocker
// (the sub-object views - CWorldZ vs CSpriteFactoryHolder, CGruntzMapMgr vs CTileGrid -
// must be folded first for the field-heavy TUs).
struct CGameRegistry {
    // The entrance-reset cue-prep call (thunk_FUN_0040cd00, __thiscall ret 0): run
    // once before the focused-grunt cue test. External/no-body (reloc-masked).
    // KNOWN-UNDECIDABLE OWNER (do not force): the body @0x40cd00 never touches
    // `this`, so a no-arg __thiscall member and a __cdecl free fn are byte-identical
    // and the callers disagree; see the Rand note below. Still a PHANTOM name.
    void CuePrep();
    // (RETIRED PHANTOMS - resolved to the RTTI-true CGruntzMgr methods by reading
    //  each call site's rel32 through its ILT thunk:
    //    PerFrameCue            -> CGruntzMgr::AdvanceOptionsCycle @0x933e0 (ILT 0x2d33)
    //    QueryLevelName         -> CGruntzMgr::GetWorldFileName    @0x928c0 (ILT 0x2531)
    //    ReportError(const char*)-> CGruntzMgr::EnterModalUI       @0x8ef10 (ILT 0x417e)
    //  Their callers (play, playplanescan) now call the real names.)
    // Registry service methods some TUs call directly on the singleton
    // (external/no-body, reloc-masked rel32 callees).
    // Rand: KNOWN-UNDECIDABLE OWNER (do not force) - the LCG body reads only the
    // g_ seed globals, so __thiscall-member vs __cdecl-free is byte-undecidable.
    i32 Rand();                    // game-mgr RNG
    i32 RandRange(i32 lo, i32 hi); // game-mgr RNG range
    // (The FOUR names `Ack` / `EmitEvent` / `Report` / `ReportError(i32,i32)` that used
    //  to be declared on this view were ONE real function: CGruntzMgr::ReportError
    //  @0x8dc60 - PROVEN by disassembling each call site's rel32 (the tile-switch ack,
    //  the hazard event, the group-broadcast report and the status-bar fail report all
    //  land on 0x8dc60, whose src claim is ?ReportError@CGruntzMgr@@QAEXIJ@Z, 100% exact).
    //  Four fake names for one function is the Eng::Teardown defect in mirror image, so
    //  they are collapsed onto the single real name below.)
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
    //   +0x54 RESOLVED -> CWorldSoundSet (<Gruntz/WorldSoundSet.h>): ONE object (proven by
    //     the LoadWorldMode ctor + all "input" methods being CWorldSoundSet methods at the
    //     exact rvas: StoreFlag=Restart@0xbc30, Arm=Resume, Disarm=Stop, Flush=Deactivate,
    //     InitInput=Init). +0x24 m_active is the mgr's armed flag == the ambient's
    //     "playable"/object-count gate, +0x08 the shared spatial-sound voice CPtrList.
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
    CWorldSoundSet* m_inputState; // +0x54  active-level input/spatial-sound object (ONE
                                  //         object, proven: the mgr's input facet == the
                                  //         ambient TU's active-level facet, +0x24 m_active
                                  //         armed==playable gate, +0x08 CPtrList spatial voice
                                  //         list; see WorldSoundSet.h)
    // +0x58  the save-game record sink IS the CSaveGame (<Io/SaveGame.h>): the
    // main-menu builder gates the AREAS pages on its m_curLevel (+0x1c) and the
    // FINAL movie on CheckMagic (0xe5690, the +0x20 == 0x42a completed-game mark);
    // the 0x8174 restart command reads its m_maxLevel (+0x18). (Was void* +
    // the MenuProgress / SaveSink58 per-TU views.)
    class CSaveGame* m_saveSink;
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
    // +0x7c  the HUD/score accumulator + cmd sink. TYPED (was `void*`, which forced a cast
    // at every use). It is a CBattlezData, and the proof needed no new work: this class's
    // OWN twin view - CGruntzMgr, the same physical class at the same offsets - already
    // declared `CBattlezData* m_scoreHud; // +0x7c` (GruntzMgr.h). The two casts that used
    // to disagree were never in conflict: the `(CBattlezData*)` sites and Wormhole.cpp's
    // `(CTeleMgrSub*)` shim both address THIS object - CTeleMgrSub was a one-field view whose
    // m_28 is exactly CBattlezData::m_28 (+0x28), the wormhole/teleporter counter that
    // FormatHudText reads back as its case-7 stat (STAT(SumGroupField20, m_28)).
    CBattlezData* m_scoreHud;
    i32 m_numRuns; // +0x80  launch counter "Num_Runs" (== GruntzMgr m_numRuns; CMulti
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
    i32 m_128;           // +0x128  per-frame play word (CPlay::OnExit clears it on state exit)
    char m_pad12c[0x130 - 0x12c];
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
