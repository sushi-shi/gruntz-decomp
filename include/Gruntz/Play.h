// Play.h - the in-game PLAY state, the
// concrete CState subclass whose Render() (vtable slot +0x14) is the
// per-frame heart of the running game: input -> per-entity step -> draw -> the
// HUD/scroll/FX overlays. See GameMode.{h,cpp} for the state hierarchy this
// extends (CState base) and RezMgr::PerFrameTick (the
// caller of m_mode->Render(), matched in `rezmgr`).
//
// CARCASS doctrine: only the member OFFSETS and the per-frame call/branch
// STRUCTURE are load-bearing. Field names are placeholders (m_<hexoffset>);
// unmatched engine callees are external no-body fns (reloc-masked `call rel32`);
// CPlay's own helper methods + high vtable slots are modeled so the indirect
// call shapes (`mov ecx,esi; call rel32`, `call [eax+0xNN]`) fall out. Globals
// (the frame clock, the game-manager singleton) are file-scope and reloc-mask.
//
// To avoid perturbing the matched-and-shared GameMode.h (entropy), CPlay is
// modeled here as a SELF-CONTAINED class with its own padded virtual interface
// (CState's slots 0..5 reproduced + the high slots Render dispatches to:
// +0x7c BeginFrameClear, +0x9c/+0xa0 the per-frame "slow/fast" virtuals).
#ifndef SRC_GRUNTZ_CPLAY_H
#define SRC_GRUNTZ_CPLAY_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// <Mfc.h> brings <windows.h> (RECT, SetRect / CopyRect / wsprintfA) and the central
// WINMM timeGetTime decl (timeGetTime is not in <windows.h> itself).
#include <Mfc.h>

// CGameRegistry - the global game-manager singleton (*g_64556c), shared via
// <Gruntz/GameRegistry.h> with the CGrunt resolvers in Grunt.h.
#include <Gruntz/GameRegistry.h>

// The zoned sound-bank manager (CWorld::m_48) + its currently-playing inner sound
// (CPlay::m_savedZonedSound). Full defs live in <Dsndmgr/GruntzSoundZ.h> (included by
// the TUs that dispatch on them); forward-declared here so the members can be typed.
class CGruntzSoundZ;
class CGruntzSoundInnerZ;

// The per-namespace load-notify sink passed to the GRUNTZ_* installers; its
// OnLoaded() (0x4bc420 thiscall) posts a load-progress tick. Full def in CPlay.cpp.
struct CLoadNotify;

// ===========================================================================
// Sub-object layouts CPlay::Render walks through (only the offsets it reads).
// ===========================================================================

struct CHitMarker; // +0x374 start-point marker element {x,y}; defined in CPlay.cpp

// The CState +0x0c view/render/resource context (the canonical CSpriteFactoryHolder) and
// its render sub-objects (CRenderer, CDrawSurface, the placed-object warlord list)
// now live in the shared <Gruntz/View.h> so the leaf-state TUs share the one shape.
#include <Gruntz/View.h>
#include <Gruntz/ResMgr.h> // the real CState::m_c sub-object classes (CDrawTarget / CImageRegistry / CSoundRegistry / CAnimRegistry)
// The world's per-kind warlord counter block (CWorld::m_7c). LoadWarlordSprites
// bumps m_30/m_34/m_38/m_40 by object-type range.
SIZE_UNKNOWN(CWarlordCounters);
struct CWarlordCounters {
    char p0[0x30];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    i32 m_38; // +0x38
    char p3c[0x40 - 0x3c];
    i32 m_40; // +0x40
};

// The world/level draw object at m_4->m_54 (the camera blit thiscall).
struct CWorldDraw {
    void Blit(i32 a, i32 b);
};

// The sub-object at m_4->m_60 (a no-arg reset; ResetForMode third teardown).
struct CWorldSub60 {};

// The plane/render geom block reached as m_4->m_30->m_24->m_5c (ResetGoals'
// float target). +0x8 flags bit0 gates the scale-multiply; +0x10/+0x14 receive
// the recomputed coords; +0x18/+0x1c hold the scale factors.
struct CPlayPlaneGeom {
    char p0[0x8];
    i32 m_8; // +0x8  flags
    char p0c[0x10 - 0xc];
    float m_10, m_14; // +0x10  recomputed plane coords (out)
    float m_18, m_1c; // +0x18  scale factors
};

// The world input dispatcher reached as m_4->m_4 (RegisterInputBindings binds
// the nine keyboard controls on it). 0x53d4e0 is a thiscall(code, flag).
struct CInputDispatch {
    void Bind(i32 code, i32 flag); // 0x53d4e0
    char p0[0x4];
    // +0x4 -> a window host whose +0x4 is the top-level HWND (PostMessageA target).
    struct WndHost {
        char p0[0x4];
        HWND m_4; // +0x04
    }* m_4;       // +0x04
};

// The "2nd world layer" reached as m_4->m_5c (OnKeyCommand's overlay forwarder).
struct CWorldLayer {
    void Forward3508(i32 a, i32 b); // 0x521e20 (thiscall) reloc-masked
};

// m_4 (the CState owner back-ptr -> the world/level object).
struct CWorld {
    // ClampApply @0x8f7f0 IS CGruntzMgr::RecomputeViewScale; cast at the call.
    // The per-frame manager tick + the restore-display-mode helper the play
    // draw/present sub-steps (DrawWorldPresent / PresentAndFlush) call on m_4.
    // ManagerTick @0x8f620 IS CGruntzMgr::PerFrameTick; cast at the call.
    // RestoreVideoMode @0x8df00 IS CGruntzMgr::SetVideoMode; cast at the call.
    // ReportError @0x346d IS CGruntzMgr::ReportError; cast at the call.
    char p0[0x4];
    CInputDispatch* m_4; // +0x04  the input dispatcher (RegisterInputBindings)
    char p8[0xc - 0x8];
    void* m_c; // +0x0c  a "active grunt"/selection ptr (==0 selects overlay path)
    char p10[0x30 - 0x10];
    struct RenderStateHolder {
        char p0[0x24];
        struct PlaneGeomHolder {
            char p0[0x10];
            RECT m_rect10; // +0x10  tile-click bounds (HandleTileClick)
            char p20[0x5c - 0x20];
            CPlayPlaneGeom* m_5c; // +0x5c
        }* m_24;                  // +0x24
    }* m_30;                      // +0x30  the render-state holder (ResetGoals)
    char p34[0x48 - 0x34];
    CGruntzSoundZ* m_48; // +0x48  the zoned sound-bank manager (PlaySound/FindSound/StopSound)
    char p4c[0x54 - 0x4c];
    CWorldDraw* m_54; // +0x54  the world/level draw object (camera blit)
    char p58[0x5c - 0x58];
    void* m_5c;        // +0x5c  a 2nd world layer
    CWorldSub60* m_60; // +0x60  reset sub-object (ResetForMode third teardown)
    char p64[0x68 - 0x64];
    // +0x68: the per-frame world timeline/substep object (the frame-timer step,
    // the fixed-sub-step variant, the HUD-rect post, the drag WorldPost).
    struct WorldTimeline {
        // 0x3017: a per-frame frame-timer step (thiscall(now)). reloc-masked.
        void Step(i32 now);
        // 0x48f7b0: the fixed-sub-step variant (thiscall(now, delta, accum)).
        void StepFull(i32 now, i32 delta, i32 accum);
        // 0x78060: post a by-value HUD rect + flag (DispatchHudClick). reloc-masked.
        void HudRect(RECT r, i32 flag);
        // 0x478a50: a world post (thiscall(a, b)) -> HandleDragMove out-of-box drag.
        void WorldPost(i32 a, i32 b);
        void Reset(); // 0x15c3 thunk (reloc-masked) per-frame/teardown reset
        // HandleTileClick marker place/cancel (thiscall, reloc-masked):
        void PlaceMarker(i32 sx, i32 sy, i32 rx, i32 ry, i32 a, i32 b, i32 c);
        void CancelMarker();
        char p0[0x230];
        i32 m_230; // +0x230  substep gate (cleared by ResetGoals)
        char p234[0x23c - 0x234];
        struct GoalObject {
            char p0[0x8];
            i32 m_flags; // +0x8  flags (ResetGoals ORs 0x10000)
        }* m_23c;        // +0x23c  goal object (ResetGoals)
        char p240[0x24c - 0x240];
        i32 m_24c; // +0x24c  marker-place gate (HandleTileClick)
        char p250[0x25c - 0x250];
        struct SelSub {
            char p0[0x2c];
            i32 m_2c; // +0x2c
        }* m_25c;     // +0x25c  active-selection sub-object (HandleTileClick)
        char p260[0x2a8 - 0x260];
        i32 m_2a8; // +0x2a8  drag-end suppress flag (HandleDragMove tail)
        char p2ac[0x400 - 0x2ac];
        i32 m_400; // +0x400  active-region gate (HandleTileClick)
    }* m_68;       // +0x68  -> +0x230 substep gate
    void* m_6c;    // +0x6c  a frame-timer object (Eng_FrameTimerStep)
    void* m_70;    // +0x70  an input sub-object
    // +0x74: the sprite/animation ref table (== g_gameReg->m_spriteFactory; this
    // CWorld view is the CGruntzMgr singleton). BeginGridWalk loads the grid's frame
    // sprite via its LoadSprite(desc, flag) facet. Full class in <Gruntz/SpriteRefTable.h>.
    CSpriteRefTable* m_74;
    char p78[0x7c - 0x78];
    CWarlordCounters* m_7c; // +0x7c  per-kind warlord counter block (LoadWarlordSprites)
    char p80[0x8c - 0x80];
    i32 m_8c; // +0x8c  viewport-clamp horizontal limit (ClampViewport2) / live mode W
    i32 m_90; // +0x90  viewport-clamp vertical limit (ClampViewport2) / live mode H
    i32 m_94; // +0x94  saved/last-good mode W (PresentAndFlush restore test)
    i32 m_98; // +0x98  saved/last-good mode H
    char p9c[0x134 - 0x9c];
    i32 m_134; // +0x134  mode/clear word (ResetForMode EnterMode gate)
    char p138[0x158 - 0x138];
    // +0x158: a flat config-array (stride 71*8 = 0x238 bytes); entry [id].m_0 is
    // the per-grunt-type sprite descriptor BeginGridWalk feeds to LoadSprite.
    char m_158[1]; // base of the config array (indexed by id*0x238)
};

// ===========================================================================
// CState (base) - the shared canonical definition (full 41-slot vftable + the
// ctor-pinned scalar layout). CPlay's Render drives the high slots
// (BeginFrameClear/RenderSlow/RenderFast) and reads m_c/m_4 as typed pointers.
// ===========================================================================
#include <Gruntz/State.h>

// A {x,y} edge pair StepInputA overlays on the CState scroll/input block
// (the flat ints at +0x188 first half, +0x198 second half).
struct Edge {
    i32 m_0;
    i32 m_4;
};

// The two serialize sub-objects SyncState (0x0d7520) round-trips through the
// archive via a reloc-masked 4-arg serialize entry. m_frameMarker is the real
// CTimer (below); m_beginMarker = CTileTriggerContainer (Serialize 0x117280),
// still unified as this serialize-entry interface because the call is
// reloc-masked and the archive arg is passed through unchanged.
SIZE_UNKNOWN(CPlaySerialChild);
struct CPlaySerialChild {
    i32 Sync(struct CSerialArchive* ar, i32 mode, i32 a2, i32 a3);
};

// The per-frame level timer (m_frameMarker's real class): the full canonical
// CTimer lives in <Gruntz/Timer.h> (extracted from SpriteLoaders.cpp). Its
// serialize entry is HandleEvent (0x9c1c0) - the reloc-masked call SyncState
// drives; the 0x8107 timer cheat (HandleCommand) zeroes its accum/expiry/
// running/current block ("Ah, who needed that stupid timer anyway?").
#include <Gruntz/Timer.h>
// The level/tile frame grid (CPlay::m_grid @+0x4cc) GrabTile/AdvanceTile walk. Top-level
// so the CState::m_c->m_10 image-registry map can return it typed (CSpriteHashTable::Lookup
// frame-grid overload), cast-free.
SIZE_UNKNOWN(CFrameGrid);
struct CFrameGrid {
    // SetDelay @0x152480 IS CImageSet::SetAllTypes; cast at each call.
    // SetSprite @0x152520 IS CImageSet::SetAllFormats; cast at each call.
    char p0[0x14];
    i32* m_rowTable; // +0x14  row/frame table
    char p18[0x64 - 0x18];
    i32 m_firstRow; // +0x64  first frame index
    i32 m_lastRow;  // +0x68  last frame index
};

// ===========================================================================
// CPlay - the in-game PLAY state. Extends CState from +0x1a8. The per-frame
// Render reads a large block of CPlay-specific members (camera/scroll rects,
// message latches, per-region one-shot FX gates). Offsets pinned by Render.
// ===========================================================================
class CPlay : public CState {
public:
    // Construction is inlined into CGruntzMgr::TransitionState (no standalone retail
    // ctor); ~CPlay is the real 0x8c830 /GX dtor. Both defined out-of-class in their
    // owning TUs (GruntzMgrTransition.cpp / CPlayDtor.cpp).
    CPlay();
    virtual ~CPlay() OVERRIDE; // slot 0 (0x8c830)

    virtual GameStateId Update() OVERRIDE; // GAMESTATE_PLAY (3);  (slot 4)
    virtual i32 Render() OVERRIDE;         // THE per-frame heart (this TU)

    // typed views of the inherited CState owner back-ptr (+0x4):
    CWorld* m_4w() {
        return (CWorld*)m_4;
    }

    // The start-point marker array (m_startMarkers) is a real CByteArray/CPtrArray whose
    // data(+0x374)/count(+0x378) FindStartPointAt walks directly (byte-identical to
    // the old raw m_markerData/m_markerCount fields).
    CHitMarker** markerData() {
        return *(CHitMarker***)((char*)&m_startMarkers + 4);
    }
    i32 markerCount() {
        return *(i32*)((char*)&m_startMarkers + 8);
    }

    // CPlay's own per-frame helper methods (the thunks Render dispatches to
    // with `mov ecx,esi`). External no-body -> reloc-masked.
    i32 StepInputA(); // (THIS TU)
    void StepWorldB();
    void ViewPreStep(void* view); // (on m_c->m_24)
    void ViewPostStep(void* view);
    // PlayCueAt: (cueId,a2,a3,a4,a5,a6,a7,rectSrc)
    void PlayCueAt(
        i32 cueId,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 rectSrc
    );                                                           // (THIS TU)
    void LoadSBITextEdges(char* name);                           // 0x0d1710 (THIS TU)
    i32 BuildGruntNamespaceList(i32 arg);                        // 0x0dd050 (THIS TU)
    i32 RegisterNamespace(CString& name, i32 a, i32 b, i32 arg); // call 0x2bc1 (THIS TU sibling)
    void PostHud(i32 wParam);
    void MarkerBegin(i32 now);
    void StepC(); // (THIS TU)
    i32 GetAmbientId();
    void StepScroll();    // (THIS TU)
    i32 OnRegion1(i32 z); // (THIS TU)
    i32 OnRegion2(i32 z); // (THIS TU)
    i32 OnRegion3(i32 z); // (THIS TU)
    i32 OnRegion4(i32 z); // (THIS TU)
    void OnRegion5();

    // The viewport-clamp sub-steps (THIS TU): shrink/clamp the active viewport then
    // push it down the draw chain. Both share a common apply-tail.
    i32 ClampViewport(i32 inset);   // 0x0d8dc0 (THIS TU)
    i32 ClampViewport2(i32 stride); // 0x0d8ed0 (THIS TU)
    i32 NotifyVisibleEntities();    // 0x0d9050 (THIS TU)
    // ClampViewport's no-change fallback (resets the viewport then re-applies). (THIS TU)
    i32 ResetViewport(); // 0x0d8c60 (thiscall on this)
    // CPlay state-exit teardown (THIS TU): ready-gate, slot-21 notify, renderer
    // refresh, then clear the registry's per-frame words + run its +0x70 teardown.
    void OnExit();      // 0x0cb400
    void ModeCleanup(); // 0x0cb740  vtable slot 0x22 mode/state-exit teardown
    // CPlay state-activation (vtable slot 8; body in PlayStateActivate.cpp): chain
    // the base activate, register the level TILEZ/IMAGEZ namespaces, run the level-
    // specific init chain, kick the state timer. Reached directly by CTriggerMgr.
    i32 OnActivate(); // 0x0cb800

    // --- leaf sub-helpers the THIS-TU functions call (external, reloc-masked) ---
    void StepC_ModeA(i32 z); // (thiscall, 1 arg) StepC m_viewMode==1
    void StepC_ModeB(i32 z); // (thiscall, 1 arg) StepC else
    void RegionEnter();      // (thiscall, no arg) OnRegion on-enter
    void RegionLeave();      // (thiscall, no arg) OnRegion on-leave
    // StepInputA's two engine callees (free fns):
    // int  Eng_InputProbe(stdcall, a,b,edge-ptr,axis-ptr,0x10)
    // void Eng_InputDispatch(cdecl, 0,0,probe-result)
    void MarkerEnd(i32 now);               // (m_beginMarker begin marker)
    void GutsStep();                       // (m_guts step)
    void FrameTimerBegin(i32 now);         // (m_frameMarker begin)
    void FrameTimerEnd(i32 flag, i32 now); // (m_frameMarker end)
    void SnapPostMessage(i32 wParam);
    void GutsStepB();
    void GutsStepC();
    void WorldSubstep();
    void Overlay1(i32 now, i32 z);
    void Overlay2(void* a, i32 z);
    void WorldBlit(i32 now);             // (m_4->m_5c)
    void InputSubStep(void* in);         // (m_4->m_70)
    void RegCue(void* sink, i32 wParam); // (reg->m_60)
    void SnapWalk();

    // --- the trace-discovered CPlay sub-steps reconstructed in this TU ---
    void ApplyGameOptions(); // 0x036be0 (THIS TU)
    void DrawWorldFrame();   // 0x0c9c20 (THIS TU)
    i32 DrawWorldFrames();   // 0x0c9cc0 (THIS TU)
    // The two timeGetTime-instrumented frame variants (the dev profiler builds the
    // "Delta=.." / "Input=.." timing lines via the cached g_pTimeGetTime fn-ptr).
    i32 ProfileDeltaFrame(); // 0x0ca0a0 (THIS TU)
    i32 ProfileInputFrame(); // 0x0c9e40 (THIS TU)
    // 0x0cf770: the Fps/Objs/Pos/Timing/Sent debug-overlay renderer (body defined in
    // DrawDebugStats.cpp; called by Render's tail + CMulti::PumpB). Was misnamed
    // "ProfFlushTail" here and re-declared on a fake CDbgView view - one method.
    void DrawDebugStats();
    i32 DispatchHudClick(i32, i32, i32);                // 0x0ce530 (THIS TU)
    i32 BeginGridWalk(const char*, i32, i32, i32, i32); // 0x0d0920 (THIS TU)
    i32 StepGridWalk(i32 dt);                           // 0x0d0a60 (THIS TU)
    i32 HandleDragMove(i32 a, i32 x, i32 y);            // 0x0d0db0 (THIS TU)
    i32 ResetGoals(i32, i32);                           // 0x0d5f00 (THIS TU)
    // The status-bar HUD (SBI_RectOnly) reaches these on the current play-state
    // (g_gameReg->m_curState downcast to CPlay); reloc-masked, bodies out-of-line.
    i32 SetState(i32 cur, i32 prev); // 0x0d5b20  set the highlight-cursor state pair
    i32 HiRefresh(i32 a);            // 0x0d6560  highlight-cursor refresh
    i32 BuildHelpReveal();           // 0x0d72c0 (THIS TU)
    i32 RegisterInputBindings();     // 0x0d9160 (THIS TU)
    // Tiny vtable forwarder: tail-call the slot-3 ready gate (Vfunc3).
    i32 ForwardReady(); // 0x0cee70
    // Region pause/resume pair (vtable slots 24/25, shared by CDemo/CMulti):
    // PauseGame saves the game clock into m_savedClock + freezes the world; ResumeGame
    // restores the clock + unpauses. Migrated from engine_boundary (CPlay).
    i32 PauseGame();  // 0x0cee90
    i32 ResumeGame(); // 0x0cef00
    // The HandleCommand cheat receivers (reloc-masked; reached via the play-state
    // lookup PickPlayOrPausedState): SetCursorFrame gives the selected grunt item
    // `item` (the 0x80e5..0x8104 ITEMCHEAT family; thunk 0x17a8); Flip returns the
    // AMBIENT%d variant index for the 0x8086 Monolith cheat (thunk 0x1df2).
    i32 SetCursorFrame(i32 item); // 0x0d1b30
    i32 Flip();                   // 0x0da200
    // ArmSnapshot (0x0d9240): latch the snapshot timer (base=clock, dur=arg2) and
    // the active flag (arg1). CanQuickSave (0x0da3b0): all-idle predicate gating
    // the auto/quick path. PostHudRect (0x0da440): post the HUD rect to the world
    // timeline then clear the ready/drag-snap gates. Migrated from engine_boundary.
    i32 ArmSnapshot(i32 active, i32 dur); // 0x0d9240
    i32 CanQuickSave();                   // 0x0da3b0
    i32 PostHudRect();                    // 0x0da440
    // Two more draw/present sub-steps migrated from the engine_boundary backlog:
    i32 DrawWorldPresent(); // 0x0cefc0 (double world-draw + present + manager tick)
    i32 PresentAndFlush();  // 0x0cba10 (restore-mode guard + present-or-notify + flush)
    // Overlay sub-step migrated from the engine_boundary backlog:
    i32 EnterOverlayDrag(i32 arg); // 0x0d6440 (arm overlay-drag + guts busy words)
    void Helper2c7f();             // 0x0d6440 prep sub-step (thiscall on this, reloc-masked)
    // leaf engine callees the above dispatch to (external, reloc-masked):
    void HudClickInRect(i32 a, i32 x, i32 y); // 0x4a9500 (thiscall on this)
    // HandleDragMove's own leaf callees (external, reloc-masked):
    void DragHudInRect(i32 a, i32 x, i32 y); // 0x4a95d0 (thiscall on this)
    void DragSnapTo(i32 x, i32 y);           // 0x4fe860 (thiscall on this)
    void EndDragSel();                       // 0x4da2d0 (thiscall on this)

    // ---- per-level resource loaders (trace-discovered, THIS TU) ----
    // Each casts `this` to a typed loader view (CPlay.cpp): the +0xc resource
    // manager (with its m_10/m_28/m_2c sub-registries) and the +0x28/+0x34 bank
    // sources are reached through offset-specific sub-types that Render models
    // differently, so a single struct-view cast at entry keeps Render's matched
    // member typing untouched.
    i32 LoadImageBanks();                              // 0x0cffe0  (the GRUNTZ/GAME bank cache)
    i32 LoadActionTileSprites(i32 force);              // 0x0db600
    i32 LoadLevelSounds(i32 force);                    // 0x0db6c0
    i32 LoadLevelImages(i32 force);                    // 0x0db7e0
    i32 LoadGameImages(i32 force);                     // 0x0db8a0
    i32 LoadGameSounds(i32 force);                     // 0x0db930
    i32 LoadGameAnims(i32 force);                      // 0x0db9b0
    i32 BuildMusicCategoryTable(i32);                  // 0x0dba30  (the MIDIZ category installer)
    i32 LoadGruntSoundNamespaces(CLoadNotify* notify); // 0x0dd830 (GRUNTZ_* sound installer)
    i32 BuildSpriteImageKeyTable(CLoadNotify* notify); // 0x0dd540 (GRUNTZ_* image installer)
    i32 BuildAnizKeyTable(CLoadNotify* notify);        // 0x0ddaa0 (GRUNTZ_* anim installer)

    // ---- the keyboard/UI command dispatcher (THIS TU) ----
    i32 OnKeyCommand(i32 key, i32 flag); // 0x0cbaf0
    // Two large play-state sub-steps the dispatcher tail-calls (external/reloc-masked;
    // deferred to the final sweep): the mode-enter gate (0x0d6fa0) and the per-frame
    // play-state reset (0x0d60b0).
    i32 EnterMode(i32 mode); // 0x0d6fa0
    i32 ResetPlayState();    // 0x0d60b0

    // ---- the trace-discovered CPlay __thiscall cluster (THIS TU) ----
    // ResetForMode (0x0c8a10): capture+hide the cursor, enter a mode, then reset
    // the per-frame drag/world-ready state and three world sub-objects.
    i32 ResetForMode(i32 mode); // 0x0c8a10
    // FindStartPointAt (0x0d5f90): registry-gated hit-test over this->m_374[] +-0x20
    // marker boxes; outputs the matched marker's coords. ret 0x10 (4 args).
    i32 FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY); // 0x0d5f90
    // FreeListTeardown (0x0cb480): release the per-level allocations back onto the
    // global free list (m_374[]/m_3ac[]/m_48c[] arrays + the per-type config rows).
    void FreeListTeardown(); // 0x0cb480
    // CPlayDtorBody (0x0c8700): the ~CPlay teardown body - free the per-frame
    // workers (m_320/m_guts/m_hitTest/m_beginMarker/m_frameMarker), clear the four g_mgrSettings config
    // rows, flush the m_startMarkers/m_3a4[4]/m_488 free-list arrays, then run the base dtor.
    void CPlayDtorBody(); // 0x0c8700
    // AddLevelGruntz (0x0d5960): walk the registry object list and register each
    // valid grunt object with the session; logs "Could not add Grunt" on failure.
    i32 AddLevelGruntz(); // 0x0d5960
    // SetEffectSpriteDurations (0x0dc060): stamp the +0x18 duration on each named
    // effect-sound descriptor looked up in the sound registry's name map.
    i32 SetEffectSpriteDurations(); // 0x0dc060
    // BuildWarlordNameTable (0x0dd340): probe the 0x39/0x3a warlord ids then bind the
    // NAPOLEAN/VIKING/PATTON CString names. CString temps force the /GX EH frame.
    i32 BuildWarlordNameTable(i32 arg); // 0x0dd340
    // ResetPlayState's own reloc-masked CPlay-thiscall leaves (external):
    void ResetGoalGeom(i32 lo, i32 hi); // 0x2e28 thunk  (this, lo, hi)
    i32 PrepareReset();                 // 0x1d75 thunk  (this) -> proceed gate
    // FreeListTeardown's reloc-masked CPlay-thiscall leaf (external):
    void Teardown1780(); // 0x1780 thunk  (this) early teardown step
    // BuildWarlordNameTable's reloc-masked CPlay-thiscall leaves (external):
    i32 ProbeWarlord(i32 id, i32 a, i32 b, i32 c);                 // 0x12da thunk  -> found
    i32 BindWarlordName(const CString& name, i32 a, i32 b, i32 c); // 0x2bc1 thunk
    // LoadWarlordSprites (0x0d65d0): ensure every sprite set a placed warlord needs is
    // loaded - full campaign preload (registry m_134 != 1) or the in-level walk of the
    // placed-object display list (renderer A's m_10). Re-homed from the ApiCaller
    // backlog; reuses ProbeWarlord (0x12da) + BindWarlordName (0x2bc1). WarlordLoadTick
    // (0x1019) is the per-set progress tick.
    i32 LoadWarlordSprites(i32 ctx, i32* loaded); // 0x0d65d0
    void WarlordLoadTick(i32);                    // 0x1019 thunk (progress tick)

    // SyncState (0x0d7520): the mode-dispatched serialize/round-trip of the play
    // state's 64-bit timer blocks + three child sync sub-objects (guts / frame
    // marker / begin marker); mode 8 (re)inits the ambient-sound cue. mode 4 =
    // write (archive vtbl[0x30]), mode 7 = read (archive vtbl[0x2c]).
    i32 SyncState(struct CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // 0x0d7520
    // SyncState's own reloc-masked CPlay-thiscall leaves (external, no body):
    i32 HeaderSerialize(struct CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // 0x4016 thunk
    i32 SyncWrite19fb(struct CSerialArchive* ar); // 0x19fb thunk (mode-4)
    i32 SyncRead2f7c(struct CSerialArchive* ar);  // 0x2f7c thunk (mode-7)

    // ---- CPlay-specific members (offsets pinned by the Render disasm) ----
    i32 m_inputWarmup1; // +0x1a8  StepInputA first-frame one-shot latch
    i32 m_inputWarmup2; // +0x1ac  StepInputA second-frame one-shot latch
    i32 m_inputHalfSel; // +0x1b0  StepInputA mirrored-half selector (0/1)
    // +0x1b4: the first of five destructible MFC members ~CPlay tears down (reverse
    // decl order); typed here so the dtor's /GX member fold falls out (CPlayDtor.cpp).
    CString m_1b4; // +0x1b4
    char m_pad1b8[0x1cc - 0x1b8];
    i32 m_savedClock; // +0x1cc  saved game clock (PauseGame stashes / ResumeGame + teardown restore to g_645588)
    char m_pad1d0[0x2d0 - 0x1d0];
    i32 m_packetsRcvd; // +0x2d0  net packets received (debug HUD "Rcvd = %i")
    i32 m_packetsSent; // +0x2d4  net packets sent (debug HUD "Sent = %i")
    char m_pad2d8[0x2dc - 0x2d8];
    // +0x2dc: the "guts"/UI subsystem the per-frame Step + the HUD/drag-select
    // dispatches run on (the click/drag/clear entry points + the busy-state words).
    struct GutsSubsystem {
        // 0x34bd: a per-frame guts step (thiscall(now)). reloc-masked.
        void Step(i32 now);
        // 0xfe6b0: the profiled-frame status-bar tick (thiscall, no arg). reloc-masked.
        void StatusBarTick();
        // 0x4ff9d0: a HUD click-at-point dispatch (thiscall(a, x, y)). reloc-masked.
        void HudClickAt(i32 a, i32 x, i32 y);
        // 0x4ff9f0: the drag-select press/move dispatch (thiscall(a, x, y)). reloc-masked.
        void DragSelect(i32 a, i32 x, i32 y);
        // 0x501420: drag-select clear/cancel (thiscall(flag)). reloc-masked.
        void DragClear(i32 flag);
        // 0x500cb0: the viewport-clamp apply (thiscall, no arg). reloc-masked.
        void ClampApply();
        // OnKeyCommand bracket-key guts sub-steps (reloc-masked ILT thunks):
        void StepBracketR(); // 0x4fe520  (']')
        void StepBracketL(); // 0x4fe460  ('[')
        void StepMinus();    // 0x4fe600  ('-')
        // EnterOverlayDrag (0x0d6440) guts sub-steps (reloc-masked ILT thunks):
        void Guts123f();             // (thiscall, no arg)  m_state==2 path
        void Guts1d61(i32 a, i32 b); // (thiscall, 2 args)  m_mode!=5 path
        void Guts427d(i32 a, i32 b); // (thiscall, 2 args)
        void Guts125d();             // (thiscall, no arg)
        void Guts35b2(i32 a);        // (thiscall, 1 arg)
        void Guts12fd(i32 a);        // (thiscall, 1 arg)
        void Guts16ea();             // (thiscall, no arg)
        void Guts367a();             // (thiscall, no arg)  ResumeGame
        // HandleTileClick HUD hit-test dispatch (thiscall, reloc-masked):
        i32 HitTest3ad5(i32 x, i32 y); // -> slot index or -1
        void Apply3ebd(i32 idx);       // apply the hit slot
        // SyncState (0x0d7520) round-trips the guts state through the archive via
        // this reloc-masked 4-arg serialize entry (CLevelSync::Sync 0x1084d0).
        i32 Sync(struct CSerialArchive* ar, i32 mode, i32 a2, i32 a3);
        i32 m_state; // +0x0  subsystem state (==2 -> ready)
        char p4[0x10 - 0x4];
        RECT m_rect10; // +0x10  HUD/click bounds (HandleTileClick)
        char p20[0x10c - 0x20];
        i32 m_mode; // +0x10c  mode word (==5 -> overlay busy)
        char p[0x548 - 0x110];
        i32 m_548; // +0x548  overlay-drag arm latch
        char p54c[0x550 - 0x54c];
        i32 m_busyA, m_busyB; // +0x550  win/lose-suppress busy words
        char q[0x574 - 0x558];
        i32 m_snapPostSel; // +0x574  snapshot post-message selector
        char r578[0x614 - 0x578];
        i32 m_614; // +0x614  mode height (SetVideoMode)
    }* m_guts;     // +0x2dc guts/UI subsystem
    // +0x2e0: a hit-test/region sink (HandleDragMove: m_hitTest->HitTest(x, y)).
    struct HitTestSink {
        // HitTest @? IS CChatBoxOwner::HitTest; cast at the call.
        // StepZoom @? IS CChatBoxOwner::Configure; cast at the call.
        char p0[0x10];
        i32 m_10; // +0x10  active-overlay gate (OnKeyCommand forward)
    }* m_hitTest;
    CPlaySerialChild* m_beginMarker; // +0x2e4  begin-marker sink (MarkerBegin; SyncState serialize)
    i32 m_dragSnapActive;            // +0x2e8  drag-snap-active latch (HandleDragMove snap path)
    i32 m_dragInProgress;            // +0x2ec  box-drag-in-progress latch (HandleDragMove)
    char m_pad2f0[0x2f8 - 0x2f0];
    i32 m_levelId; // +0x2f8  level/region id (==0x66 -> booty-region init)
    char m_pad2fc[0x304 - 0x2fc];
    i32 m_dragClampMaxX; // +0x304  drag-clamp max X
    i32 m_dragClampMaxY; // +0x308  drag-clamp max Y
    i32 m_worldReady;    // +0x30c  world-ready gate (0 until inited)
    RECT m_hudRect;      // +0x310  HUD/selection rect buffer fed to the HUD draw
    i32 m_overlayActive; // +0x320  show-overlay/banner gate
    char m_pad324[0x328 - 0x324];
    i32 m_bootyTimerLo, m_bootyTimerHi, m_bootyInterval,
        m_bootyIntervalHi; // +0x328  booty-region 64-bit timer
    i32 m_ambientTimerLo, m_ambientTimerHi, m_ambientInterval,
        m_ambientIntervalHi; // +0x338  ambient-init timer
    i32 m_ambientInitDone;   // +0x348  ambient-init DONE latch
    char m_pad34c[0x350 - 0x34c];
    i32 m_syncTimerLo, m_syncTimerHi, m_syncInterval,
        m_syncIntervalHi; // +0x350  play-state 64-bit sync timer (SyncState first block)
    i32 m_tileClickX;     // +0x360  tile-click snapped X (HandleTileClick)
    i32 m_tileClickY;     // +0x364  tile-click snapped Y (HandleTileClick)
    i32 m_dragInhibit1;   // +0x368  drag/select inhibit gate
    i32 m_dragInhibit2;   // +0x36c  drag/select inhibit gate
    // +0x370: a CByteArray/CPtrArray of start-point markers (the 2nd destructible
    // member); FindStartPointAt reads its data(+4)/count(+8) via markerData()/
    // markerCount(). +0x3a4: a CByteArray[4] (the 3rd..? member, one ??_M vector fold).
    CByteArray m_startMarkers; // +0x370  (data@+4 = marker-ptr array, count@+8 = marker count)
    char m_pad384[0x3a4 - 0x384];
    CByteArray m_3a4[4]; // +0x3a4  (4 * 0x14)
    CTimer*
        m_frameMarker; // +0x3f4  frame-marker/timeline CTimer (SyncState serialize; 0x8107 cheat)
    i32 m_cueTimerLo, m_cueTimerHi, m_cueInterval,
        m_cueIntervalHi; // +0x3f8  AMBIENT-cue 64-bit timer
    i32 m_cueToggle;     // +0x408  AMBIENT-cue on/off toggle
    i32 m_lastCueId;     // +0x40c  PlayCueAt last-shown cueId (de-dupe gate)
    CString
        m_cueText; // +0x410  4th destructible member (PlayCueAt reads &m_cueText as its de-dupe state)
    i32 m_drewThisFrame; // +0x414  per-frame "drew" flag (cleared at entry)
    char m_pad418[0x430 - 0x418];
    i32 m_region0TimerLo, m_region0TimerHi, m_region0Interval, m_region0IntervalHi; // +0x430
    i32 m_region1TimerLo, m_region1TimerHi, m_region1Interval, m_region1IntervalHi; // +0x440
    i32 m_region2TimerLo, m_region2TimerHi, m_region2Interval, m_region2IntervalHi; // +0x450
    i32 m_region3TimerLo, m_region3TimerHi, m_region3Interval, m_region3IntervalHi; // +0x460
    i32 m_region0Gate;   // +0x470  region-0 gate (OnRegion2 / extra HUD layer)
    i32 m_region1Gate;   // +0x474  region-1 gate (OnRegion1 / alt-input draw)
    i32 m_region2Gate;   // +0x478  region-2 gate (OnRegion3)
    i32 m_region3Gate;   // +0x47c  region-3 gate (OnRegion4)
    i32 m_viewMode;      // +0x480  StepC/OnRegion view-mode discriminator (0=idle/1/2)
    i32 m_hudSuppressed; // +0x484  HUD-suppress gate (DispatchHudClick early-out)
    CByteArray m_488;    // +0x488  5th destructible member (0x14 bytes)
    char m_pad49c[0x4a0 - 0x49c];
    i32 m_snapBaseLo, m_snapBaseHi, m_snapDur,
        m_snapDurHi;        // +0x4a0  snapshot 64-bit base + duration
    i32 m_snapshotActive;   // +0x4b0  snapshot ACTIVE latch
    i32 m_scrollEdgeActive; // +0x4b4  edge active bits
    i32 m_scrollEdgeLock;   // +0x4b8  edge lock bits
    i32 m_revealFrame;      // +0x4bc  reveal-strip frame counter (BuildHelpReveal)
    // +0x4c0  reveal-strip cap sprite objects (passed by-ptr to the HUD-strip draw).
    void *m_revealCapMid, *m_revealCapEnd, *m_revealCapStart;
    // +0x4cc: the level/tile frame grid GrabTile/AdvanceTile walk (CFrameGrid, above)
    CFrameGrid* m_grid;   // +0x4cc  level grid object
    i32 m_gridCurFrame;   // +0x4d0  current tile/frame id
    i32 m_gridHasSprite;  // +0x4d4  has-grid-sprite flag
    i32 m_gridDelayBase;  // +0x4d8  step-delay base
    i32 m_gridDelayCount; // +0x4dc  step-delay countdown
    i32 m_gridRow;        // +0x4e0  current row index
    struct ScrollSink {
        char p0[0x40];
        i32 m_flags; // +0x40  drag/select state flags (bit0 = active)
        char p44[0x5c - 0x44];
        i32 m_scrollX;     // +0x5c  scroll offset X (StepScroll out)
        i32 m_scrollY;     // +0x60  scroll offset Y (StepScroll out)
    }* m_scrollSink;       // +0x4e4  StepScroll's scroll-offset sink + drag flags
    i32 m_gridWalkActive;  // +0x4e8  grid-walk active flag
    i32 m_renderDisabled;  // +0x4ec  Render hard early-out gate
    i32 m_4f0;             // +0x4f0  highlight-busy gate (SBI_RectOnly reads it non-zero => bail)
    i32 m_winLoseBanner;   // +0x4f4  win/lose banner gate
    i32 m_inGame;          // +0x4f8  PRIMARY mode: nonzero = main in-game frame
    i32 m_overlayDrag;     // +0x4fc  overlay-drag-active flag
    i32 m_paused;          // +0x500  paused/no-step flag
    i32 m_dragEndNotify;   // +0x504  drag-end notify gate
    i32 m_lastScrollTimeX; // +0x508  last-scroll time (horizontal)
    i32 m_lastScrollTimeY; // +0x50c  last-scroll time (vertical)
    i32 m_stepCountdown;   // +0x510  per-frame entity-step countdown
    char m_pad514[0x518 - 0x514];
    CGruntzSoundInnerZ*
        m_savedZonedSound; // +0x518  saved currently-playing zoned sound (region pause/resume)

    // Engine-label backlog stubs.
    void PlayBacklog08c9d0();
    i32 winapi_0cdb10_PostMessageA(i32, i32, i32);
    // HandleTileClick (0xceae0): the menu/pause-state pointer-click handler - the
    // mouse-input twin of OnKeyCommand. Gated resume/report/unpause chain, then an
    // overlay probe + a HUD hit-test + a grid-snapped world marker place/cancel.
    i32 HandleTileClick(i32 a, i32 x, i32 y);
    i32 winapi_0d0b30_CopyRect(i32);
    i32 LoadCursorSprites(i32 frame, i32 flag);
    i32 LoadScrollSpeedOptions();
    i32 BuildGruntTypeNameTable(i32, i32, i32, i32);

    // HandleMousePress (0x0ce660): vtable slot 16 (+0x40) - the in-game
    // pointer/click dispatcher (mouse sibling of OnKeyCommand). Re-homed from
    // GameMouseHandler.cpp; reaches the guts/status-bar sub-objects at m_guts /
    // m_hitTest / m_4 / m_c which that TU casts to its local facet views.
    i32 HandleMousePress(i32 msg, i32 x, i32 y); // 0x0ce660

    // The two per-frame plane-list sub-steps re-homed from CPlayPlaneScan.cpp: walk
    // the renderer's embedded plane list (m_c->renderer+0x10) and dispatch on each
    // plane descriptor's type. Both take a stack MFC temp -> /GX.
    i32 ScanBuildTiles();   // 0x0d53d0
    i32 ScanShuffleQuads(); // 0x0d9290
};

// ===========================================================================
// The frame-clock + singleton globals CPlay::Render reads each frame.
// ===========================================================================
// The dev/render-state singleton DispatchHudClick reads (*g_645578); its +0x18 is
// a flags word masked with 0x20 to gate the HUD-rect post.
struct StateMgrBZ {
    i32 m_0, m_4, m_8; // +0x00..+0x08
    char m_padc[0x10 - 0xc];
    i32 m_10, m_14; // +0x10, +0x14
    i32 m_18;       // +0x18  flags
    i32 Flush();    // 0x385e0 (?Flush@) (the +0x578 state-mgr flush)
};

extern "C" {
    extern u32 g_645580;            // g_lastNow  (-> mirror g_6bf3c0)
    extern u32 g_645584;            // g_lastDelta
    extern u32 g_645588;            // g_accumMs (the running game clock)
    extern CGameRegistry* g_64556c; // the singleton ptr
    extern StateMgrBZ* g_645578;    // the dev/render-state singleton (DispatchHudClick)
    extern i32 g_644c54;            // a default cue/message wParam
    extern u32 g_6bf3c0;            // draw-clock mirror
    extern u32 g_6bf3bc;            // draw-delta mirror
}

#endif // SRC_GRUNTZ_CPLAY_H
