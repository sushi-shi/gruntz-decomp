// CPlay.h - the in-game PLAY state, the
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
// <Gruntz/CGameRegistry.h> with the CGrunt resolvers in Grunt.h.
#include <Gruntz/CGameRegistry.h>

// ===========================================================================
// Sub-object layouts CPlay::Render walks through (only the offsets it reads).
// ===========================================================================

// The renderer/draw object (m_c->m_8 = renderer A, m_c->m_c = renderer B). Its
// vtable carries the per-frame draw slots: +0x24 begin-scene (1 arg), +0x34
// present (2 args). Modeled with a padded virtual interface so the indirect
// `call [vtbl+0x24]` / `[vtbl+0x34]` shapes fall out.
struct CRenderer {
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void BeginScene(i32 z); // slot 9  (+0x24)
    virtual void s0a();
    virtual void s0b();
    virtual void s0c();
    virtual void Present(i32 a, i32 b); // slot 13 (+0x34)
};

// The draw-surface object at m_c->m_24 (the target of the thiscall PushView +
// the ViewPreStep/PostStep sub-steps). Its +0x5c holds the camera geometry the
// world blit reads (+0x84 / +0x88).
struct CDrawSurface {
    void PushView(void* view, void* renderer);
    void PreStep();
    void PostStep();
    char p0[0x10];
    // +0x10: the viewport rect {left,top,right,bottom}; StepScroll reads .left/.top
    // as the scroll origin, DispatchHudClick reads all four as the bounds box.
    RECT m_viewport; // +0x10  viewport rect (also the scroll origin .left/.top)
    char p20[0x5c - 0x20];
    // +0x5c -> a geom block: StepScroll reads (m_5c+0x40).{m_originX,m_originY};
    // the world blit reads (m_5c).{m_84,m_88}.
    struct CameraGeom {
        void DrawA(); // 0x563300  per-frame world-draw sub-step A
        void DrawB(); // 0x563370  per-frame world-draw sub-step B
        char p0[0x40];
        i32 m_originX; // +0x40
        i32 m_originY; // +0x44
        char p48[0x84 - 0x48];
        i32 m_84;
        i32 m_88;
    }* m_5c; // +0x5c camera geom
};

// m_c (the view/anim sub-object holder).
struct CView {
    char p0[0x4];
    struct RenderState { // +0x04  the renderer-state object
        char p0[0x10];
        struct SurfaceA {
            char p0[0x2c];
            void* m_2c;
        }* m_10; // +0x10 -> +0x2c surface
        struct SurfaceB {
            char p0[0x2c];
            void* m_2c;
        }* m_14;    // +0x14 -> +0x2c draw surface (view obj)
        void* m_18; // +0x18  the present target
    }* m_4;
    CRenderer* m_8; // +0x08  renderer A
    CRenderer* m_c; // +0x0c  renderer B (present)
    // +0x10 -> +0x10 is a CMapPtrToPtr (BeginGridWalk looks up the frame grid).
    struct GridMapHolder {
        char p0[0x10];
        struct CMap {
            void Lookup(i32 key, void*& out); // 0x1b8008 (thiscall)
        } m_10;                               // +0x10  the grid map
    }* m_10;                                  // +0x10
    char p14[0x20 - 0x14];
    void* m_20;         // +0x20  a frame profiler timer (timeGetTime x2)
    CDrawSurface* m_24; // +0x24  the draw-surface (PushView / Pre/PostStep)
};

// The world/level draw object at m_4->m_54 (the camera blit thiscall).
struct CWorldDraw {
    void Blit(i32 a, i32 b);
};

// The plane/render geom block reached as m_4->m_30->m_24->m_5c (ResetGoals'
// float target). +0x8 flags bit0 gates the scale-multiply; +0x10/+0x14 receive
// the recomputed coords; +0x18/+0x1c hold the scale factors.
struct CPlaneGeom {
    void Recompute(); // 0x161c90  RecomputePlaneCoords (thiscall, no arg)
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
};

// m_4 (the CState owner back-ptr -> the world/level object).
struct CWorld {
    char p0[0x4];
    CInputDispatch* m_4; // +0x04  the input dispatcher (RegisterInputBindings)
    char p8[0xc - 0x8];
    void* m_c; // +0x0c  a "active grunt"/selection ptr (==0 selects overlay path)
    char p10[0x30 - 0x10];
    struct RenderStateHolder {
        char p0[0x24];
        struct PlaneGeomHolder {
            char p0[0x5c];
            CPlaneGeom* m_5c; // +0x5c
        }* m_24;              // +0x24
    }* m_30;                  // +0x30  the render-state holder (ResetGoals)
    char p34[0x48 - 0x34];
    void* m_48; // +0x48  the sound manager (PlaySound/FindSound/StopSound)
    char p4c[0x54 - 0x4c];
    CWorldDraw* m_54; // +0x54  the world/level draw object (camera blit)
    char p58[0x5c - 0x58];
    void* m_5c; // +0x5c  a 2nd world layer
    char p60[0x68 - 0x60];
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
        char p0[0x230];
        i32 m_230; // +0x230  substep gate (cleared by ResetGoals)
        char p234[0x23c - 0x234];
        struct GoalObject {
            char p0[0x8];
            i32 m_flags; // +0x8  flags (ResetGoals ORs 0x10000)
        }* m_23c;        // +0x23c  goal object (ResetGoals)
        char p240[0x2a8 - 0x240];
        i32 m_2a8; // +0x2a8  drag-end suppress flag (HandleDragMove tail)
    }* m_68;       // +0x68  -> +0x230 substep gate
    void* m_6c;    // +0x6c  a frame-timer object (Eng_FrameTimerStep)
    void* m_70;    // +0x70  an input sub-object
    // +0x74: the sprite/animation loader (BeginGridWalk loads the grid's frame
    // sprite). NOT the engine CSpriteFactory (that one is CreateSprite); this is a
    // distinct LoadSprite API, named for its role to avoid a false identity.
    struct SpriteLoader {
        void* LoadSprite(void* desc, i32 flag); // 0x4e23c0 (thiscall)
    }* m_74;
    // +0x158: a flat config-array (stride 71*8 = 0x238 bytes); entry [id].m_0 is
    // the per-grunt-type sprite descriptor BeginGridWalk feeds to LoadSprite.
    char p78[0x158 - 0x78];
    char m_158[1]; // base of the config array (indexed by id*0x238)
};

// ===========================================================================
// CState (base) - the shared canonical definition (full 41-slot vftable + the
// ctor-pinned scalar layout). CPlay's Render drives the high slots
// (BeginFrameClear/RenderSlow/RenderFast) and reads m_c/m_4 as typed pointers.
// ===========================================================================
#include <Gruntz/CState.h>

// A {x,y} edge pair StepInputA overlays on the CState scroll/input block
// (the flat ints at +0x188 first half, +0x198 second half).
struct Edge {
    i32 m_0;
    i32 m_4;
};

// ===========================================================================
// CPlay - the in-game PLAY state. Extends CState from +0x1a8. The per-frame
// Render reads a large block of CPlay-specific members (camera/scroll rects,
// message latches, per-region one-shot FX gates). Offsets pinned by Render.
// ===========================================================================
class CPlay : public CState {
public:
    virtual i32 Update() OVERRIDE; // return 3;  (slot 4)
    virtual i32 Render() OVERRIDE; // THE per-frame heart (this TU)

    // typed views of the inherited CState owner back-ptr (+0x4):
    CWorld* m_4w() {
        return (CWorld*)m_4;
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
    ); // (THIS TU)
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
    void DrawWorldFrame();                      // 0x0c9c20 (THIS TU)
    i32 DrawWorldFrames();                      // 0x0c9cc0 (THIS TU)
    i32 DispatchHudClick(i32, i32, i32);        // 0x0ce530 (THIS TU)
    i32 BeginGridWalk(i32, i32, i32, i32, i32); // 0x0d0920 (THIS TU)
    i32 StepGridWalk(i32 dt);                   // 0x0d0a60 (THIS TU)
    i32 HandleDragMove(i32 a, i32 x, i32 y);    // 0x0d0db0 (THIS TU)
    i32 ResetGoals(i32, i32);                   // 0x0d5f00 (THIS TU)
    i32 BuildHelpReveal();                      // 0x0d72c0 (THIS TU)
    i32 RegisterInputBindings();                // 0x0d9160 (THIS TU)
    // leaf engine callees the above dispatch to (external, reloc-masked):
    void HudClickInRect(i32 a, i32 x, i32 y); // 0x4a9500 (thiscall on this)
    // HandleDragMove's own leaf callees (external, reloc-masked):
    void DragHudInRect(i32 a, i32 x, i32 y); // 0x4a95d0 (thiscall on this)
    void DragSnapTo(i32 x, i32 y);           // 0x4fe860 (thiscall on this)
    void EndDragSel();                       // 0x4da2d0 (thiscall on this)

    // ---- CPlay-specific members (offsets pinned by the Render disasm) ----
    i32 m_inputWarmup1; // +0x1a8  StepInputA first-frame one-shot latch
    i32 m_inputWarmup2; // +0x1ac  StepInputA second-frame one-shot latch
    i32 m_inputHalfSel; // +0x1b0  StepInputA mirrored-half selector (0/1)
    char m_pad1b4[0x2dc - 0x1b4];
    // +0x2dc: the "guts"/UI subsystem the per-frame Step + the HUD/drag-select
    // dispatches run on (the click/drag/clear entry points + the busy-state words).
    struct GutsSubsystem {
        // 0x34bd: a per-frame guts step (thiscall(now)). reloc-masked.
        void Step(i32 now);
        // 0x4ff9d0: a HUD click-at-point dispatch (thiscall(a, x, y)). reloc-masked.
        void HudClickAt(i32 a, i32 x, i32 y);
        // 0x4ff9f0: the drag-select press/move dispatch (thiscall(a, x, y)). reloc-masked.
        void DragSelect(i32 a, i32 x, i32 y);
        // 0x501420: drag-select clear/cancel (thiscall(flag)). reloc-masked.
        void DragClear(i32 flag);
        i32 m_state; // +0x0  subsystem state (==2 -> ready)
        char p4[0x10c - 0x4];
        i32 m_mode; // +0x10c  mode word (==5 -> overlay busy)
        char p[0x550 - 0x110];
        i32 m_busyA, m_busyB; // +0x550  win/lose-suppress busy words
        char q[0x574 - 0x558];
        i32 m_snapPostSel; // +0x574  snapshot post-message selector
    }* m_guts;             // +0x2dc guts/UI subsystem
    // +0x2e0: a hit-test/region sink (HandleDragMove: m_hitTest->HitTest(x, y)).
    struct HitTestSink {
        i32 HitTest(i32 x, i32 y); // 0x421140 (thiscall) -> nonzero = consumed
    }* m_hitTest;
    void* m_beginMarker;  // +0x2e4  begin-marker sink (MarkerBegin)
    i32 m_dragSnapActive; // +0x2e8  drag-snap-active latch (HandleDragMove snap path)
    i32 m_dragInProgress; // +0x2ec  box-drag-in-progress latch (HandleDragMove)
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
    char m_pad34c[0x368 - 0x34c];
    i32 m_dragInhibit1; // +0x368  drag/select inhibit gate
    i32 m_dragInhibit2; // +0x36c  drag/select inhibit gate
    char m_pad370[0x3f4 - 0x370];
    void* m_frameMarker; // +0x3f4  frame-marker/timeline object (+0x30..0x4c reset block)
    i32 m_cueTimerLo, m_cueTimerHi, m_cueInterval,
        m_cueIntervalHi;            // +0x3f8  AMBIENT-cue 64-bit timer
    i32 m_cueToggle;                // +0x408  AMBIENT-cue on/off toggle
    i32 m_lastCueId;                // +0x40c  PlayCueAt last-shown cueId (de-dupe gate)
    char m_cueState[0x414 - 0x410]; // +0x410  PlayCueAt per-cue de-dupe state object (addr taken)
    i32 m_drewThisFrame;            // +0x414  per-frame "drew" flag (cleared at entry)
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
    char m_pad488[0x4a0 - 0x488];
    i32 m_snapBaseLo, m_snapBaseHi, m_snapDur,
        m_snapDurHi;      // +0x4a0  snapshot 64-bit base + duration
    i32 m_snapshotActive; // +0x4b0  snapshot ACTIVE latch
    char m_pad4b4[0x4bc - 0x4b4];
    i32 m_revealFrame; // +0x4bc  reveal-strip frame counter (BuildHelpReveal)
    i32 m_revealCapMid, m_revealCapEnd, m_revealCapStart; // +0x4c0  reveal-strip cap sprites
    // +0x4cc: the level/tile frame grid GrabTile/AdvanceTile walk:
    //   m_grid -> a grid object (+0x64 first row, +0x68 last row, +0x14 row table)
    struct CFrameGrid {
        void SetDelay(i32 d);    // 0x552480 (thiscall)
        void SetSprite(void* s); // 0x552520 (thiscall)
        char p0[0x14];
        i32* m_rowTable; // +0x14  row/frame table
        char p18[0x64 - 0x18];
        i32 m_firstRow;   // +0x64  first frame index
        i32 m_lastRow;    // +0x68  last frame index
    }* m_grid;            // +0x4cc  level grid object
    i32 m_gridCurFrame;   // +0x4d0  current tile/frame id
    i32 m_gridHasSprite;  // +0x4d4  has-grid-sprite flag
    i32 m_gridDelayBase;  // +0x4d8  step-delay base
    i32 m_gridDelayCount; // +0x4dc  step-delay countdown
    i32 m_gridRow;        // +0x4e0  current row index
    struct ScrollSink {
        char p0[0x40];
        i32 m_flags; // +0x40  drag/select state flags (bit0 = active)
        char p44[0x5c - 0x44];
        i32 m_scrollX;    // +0x5c  scroll offset X (StepScroll out)
        i32 m_scrollY;    // +0x60  scroll offset Y (StepScroll out)
    }* m_scrollSink;      // +0x4e4  StepScroll's scroll-offset sink + drag flags
    i32 m_gridWalkActive; // +0x4e8  grid-walk active flag
    i32 m_renderDisabled; // +0x4ec  Render hard early-out gate
    char m_pad4f0[0x4f4 - 0x4f0];
    i32 m_winLoseBanner; // +0x4f4  win/lose banner gate
    i32 m_inGame;        // +0x4f8  PRIMARY mode: nonzero = main in-game frame
    i32 m_overlayDrag;   // +0x4fc  overlay-drag-active flag
    i32 m_paused;        // +0x500  paused/no-step flag
    i32 m_dragEndNotify; // +0x504  drag-end notify gate
    char m_pad508[0x510 - 0x508];
    i32 m_stepCountdown; // +0x510  per-frame entity-step countdown

    // Engine-label backlog stubs.
    void Stub_08c9d0();
    i32 winapi_0cdb10_PostMessageA(i32, i32, i32);
    i32 winapi_0ceae0_PostMessageA(i32, i32, i32);
    i32 winapi_0d0b30_CopyRect(i32);
    void LoadCursorSprites(i32, i32);
    void LoadScrollSpeedOptions();
    void BuildGruntTypeNameTable(i32, i32, i32, i32);
};

// ===========================================================================
// The frame-clock + singleton globals CPlay::Render reads each frame.
// ===========================================================================
// The dev/render-state singleton DispatchHudClick reads (*g_645578); its +0x18 is
// a flags word masked with 0x20 to gate the HUD-rect post.
struct CRenderState {
    char p0[0x18];
    i32 m_18; // +0x18  flags
};

extern "C" {
    extern u32 g_645580;            // g_lastNow  (-> mirror g_6bf3c0)
    extern u32 g_645584;            // g_lastDelta
    extern u32 g_645588;            // g_accumMs (the running game clock)
    extern CGameRegistry* g_64556c; // the singleton ptr
    extern CRenderState* g_645578;  // the dev/render-state singleton (DispatchHudClick)
    extern i32 g_644c54;            // a default cue/message wParam
    extern u32 g_6bf3c0;            // draw-clock mirror
    extern u32 g_6bf3bc;            // draw-delta mirror
}

#endif // SRC_GRUNTZ_CPLAY_H
