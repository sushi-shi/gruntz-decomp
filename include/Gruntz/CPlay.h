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
    virtual void BeginScene(int z); // slot 9  (+0x24)
    virtual void s0a();
    virtual void s0b();
    virtual void s0c();
    virtual void Present(int a, int b); // slot 13 (+0x34)
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
    int m_10; // +0x10  scroll origin X / viewport left
    int m_14; // +0x14  scroll origin Y / viewport top
    int m_18; // +0x18  viewport right
    int m_1c; // +0x1c  viewport bottom
    char p20[0x5c - 0x20];
    // +0x5c -> a geom block: StepScroll reads (m_5c+0x40).{m_0,m_4}; the world
    // blit reads (m_5c).{m_84,m_88}.
    struct M5c {
        void DrawA(); // 0x563300  per-frame world-draw sub-step A
        void DrawB(); // 0x563370  per-frame world-draw sub-step B
        char p0[0x40];
        int m_40_0;
        int m_40_4;
        char p48[0x84 - 0x48];
        int m_84;
        int m_88;
    }* m_5c; // +0x5c camera geom
};

// m_c (the view/anim sub-object holder).
struct CView {
    char p0[0x4];
    struct M4 { // +0x04  the renderer-state object
        char p0[0x10];
        struct M10 {
            char p0[0x2c];
            void* m_2c;
        }* m_10; // +0x10 -> +0x2c surface
        struct M14 {
            char p0[0x2c];
            void* m_2c;
        }* m_14;    // +0x14 -> +0x2c draw surface (view obj)
        void* m_18; // +0x18  the present target
    }* m_4;
    CRenderer* m_8; // +0x08  renderer A
    CRenderer* m_c; // +0x0c  renderer B (present)
    // +0x10 -> +0x10 is a CMapPtrToPtr (BeginGridWalk looks up the frame grid).
    struct M10 {
        char p0[0x10];
        struct CMap {
            void Lookup(int key, void*& out); // 0x1b8008 (thiscall)
        } m_10;                               // +0x10  the grid map
    }* m_10;                                  // +0x10
    char p14[0x20 - 0x14];
    void* m_20;         // +0x20  a frame profiler timer (timeGetTime x2)
    CDrawSurface* m_24; // +0x24  the draw-surface (PushView / Pre/PostStep)
};

// The world/level draw object at m_4->m_54 (the camera blit thiscall).
struct CWorldDraw {
    void Blit(int a, int b);
};

// The plane/render geom block reached as m_4->m_30->m_24->m_5c (ResetGoals'
// float target). +0x8 flags bit0 gates the scale-multiply; +0x10/+0x14 receive
// the recomputed coords; +0x18/+0x1c hold the scale factors.
struct CPlaneGeom {
    void Recompute(); // 0x161c90  RecomputePlaneCoords (thiscall, no arg)
    char p0[0x8];
    int m_8; // +0x8  flags
    char p0c[0x10 - 0xc];
    float m_10, m_14; // +0x10  recomputed plane coords (out)
    float m_18, m_1c; // +0x18  scale factors
};

// The world input dispatcher reached as m_4->m_4 (RegisterInputBindings binds
// the nine keyboard controls on it). 0x53d4e0 is a thiscall(code, flag).
struct CInputDispatch {
    void Bind(int code, int flag); // 0x53d4e0
};

// m_4 (the CState owner back-ptr -> the world/level object).
struct CWorld {
    char p0[0x4];
    CInputDispatch* m_4; // +0x04  the input dispatcher (RegisterInputBindings)
    char p8[0xc - 0x8];
    void* m_c; // +0x0c  a "active grunt"/selection ptr (==0 selects overlay path)
    char p10[0x30 - 0x10];
    struct M30 {
        char p0[0x24];
        struct M24 {
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
    struct M68 {
        // 0x3017: a per-frame frame-timer step (thiscall(now)). reloc-masked.
        void Step(int now);
        // 0x48f7b0: the fixed-sub-step variant (thiscall(now, delta, accum)).
        void StepFull(int now, int delta, int accum);
        // 0x78060: post a by-value HUD rect + flag (DispatchHudClick). reloc-masked.
        void HudRect(RECT r, int flag);
        char p0[0x230];
        int m_230; // +0x230  substep gate (cleared by ResetGoals)
        char p234[0x23c - 0x234];
        struct M23c {
            char p0[0x8];
            int m_8; // +0x8  flags (ResetGoals ORs 0x10000)
        }* m_23c;    // +0x23c  goal object (ResetGoals)
    }* m_68;         // +0x68  -> +0x230 substep gate
    void* m_6c;      // +0x6c  a frame-timer object (Eng_FrameTimerStep)
    void* m_70;      // +0x70  an input sub-object
    // +0x74: the sprite factory (BeginGridWalk loads the grid's frame sprite).
    struct CSpriteFactory {
        void* LoadSprite(void* desc, int flag); // 0x4e23c0 (thiscall)
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
    int m_0;
    int m_4;
};

// ===========================================================================
// CPlay - the in-game PLAY state. Extends CState from +0x1a8. The per-frame
// Render reads a large block of CPlay-specific members (camera/scroll rects,
// message latches, per-region one-shot FX gates). Offsets pinned by Render.
// ===========================================================================
class CPlay : public CState {
public:
    virtual int Update() OVERRIDE; // return 3;  (slot 4)
    virtual int Render() OVERRIDE; // THE per-frame heart (this TU)

    // typed views of the inherited CState owner back-ptr (+0x4):
    CWorld* m_4w() {
        return (CWorld*)m_4;
    }

    // CPlay's own per-frame helper methods (the thunks Render dispatches to
    // with `mov ecx,esi`). External no-body -> reloc-masked.
    int StepInputA(); // (THIS TU)
    void StepWorldB();
    void ViewPreStep(void* view); // (on m_c->m_24)
    void ViewPostStep(void* view);
    // PlayCueAt: (cueId,a2,a3,a4,a5,a6,a7,rectSrc)
    void PlayCueAt(
        int cueId,
        int a2,
        int a3,
        int a4,
        int a5,
        int a6,
        int a7,
        int rectSrc
    ); // (THIS TU)
    void PostHud(int wParam);
    void MarkerBegin(int now);
    void StepC(); // (THIS TU)
    int GetAmbientId();
    void StepScroll();    // (THIS TU)
    int OnRegion1(int z); // (THIS TU)
    int OnRegion2(int z); // (THIS TU)
    int OnRegion3(int z); // (THIS TU)
    int OnRegion4(int z); // (THIS TU)
    void OnRegion5();

    // --- leaf sub-helpers the THIS-TU functions call (external, reloc-masked) ---
    void StepC_ModeA(int z); // (thiscall, 1 arg) StepC m_480==1
    void StepC_ModeB(int z); // (thiscall, 1 arg) StepC else
    void RegionEnter();      // (thiscall, no arg) OnRegion on-enter
    void RegionLeave();      // (thiscall, no arg) OnRegion on-leave
    // StepInputA's two engine callees (free fns):
    // int  Eng_InputProbe(stdcall, a,b,edge-ptr,axis-ptr,0x10)
    // void Eng_InputDispatch(cdecl, 0,0,probe-result)
    void MarkerEnd(int now);               // (m_2e4 begin marker)
    void GutsStep();                       // (m_2dc step)
    void FrameTimerBegin(int now);         // (m_3f4 begin)
    void FrameTimerEnd(int flag, int now); // (m_3f4 end)
    void SnapPostMessage(int wParam);
    void GutsStepB();
    void GutsStepC();
    void WorldSubstep();
    void Overlay1(int now, int z);
    void Overlay2(void* a, int z);
    void WorldBlit(int now);             // (m_4->m_5c)
    void InputSubStep(void* in);         // (m_4->m_70)
    void RegCue(void* sink, int wParam); // (reg->m_60)
    void SnapWalk();

    // --- the trace-discovered CPlay sub-steps reconstructed in this TU ---
    void DrawWorldFrame();                      // 0x0c9c20 (THIS TU)
    int DrawWorldFrames();                      // 0x0c9cc0 (THIS TU)
    int DispatchHudClick(int, int, int);        // 0x0ce530 (THIS TU)
    int BeginGridWalk(int, int, int, int, int); // 0x0d0920 (THIS TU)
    int StepGridWalk(int dt);                   // 0x0d0a60 (THIS TU)
    int ResetGoals(int, int);                   // 0x0d5f00 (THIS TU)
    int BuildHelpReveal();                      // 0x0d72c0 (THIS TU)
    int RegisterInputBindings();                // 0x0d9160 (THIS TU)
    // leaf engine callees the above dispatch to (external, reloc-masked):
    void HudClickInRect(int a, int x, int y); // 0x4a9500 (thiscall on this)

    // ---- CPlay-specific members (offsets pinned by the Render disasm) ----
    int m_1a8; // +0x1a8  StepInputA latch-1 (one-shot)
    int m_1ac; // +0x1ac  StepInputA latch-2 (one-shot)
    int m_1b0; // +0x1b0  StepInputA half-selector
    char m_pad1b4[0x2dc - 0x1b4];
    struct M2dc {
        // 0x34bd: a per-frame guts step (thiscall(now)). reloc-masked.
        void Step(int now);
        // 0x4ff9d0: a HUD click-at-point dispatch (thiscall(a, x, y)). reloc-masked.
        void HudClickAt(int a, int x, int y);
        int m_0; // +0x0  subsystem state (==2 -> ready)
        char p4[0x10c - 0x4];
        int m_10c;
        char p[0x550 - 0x110];
        int m_550, m_554;
        char q[0x574 - 0x558];
        int m_574;
    }* m_2dc;    // +0x2dc subsystem
    void* m_2e0; // +0x2e0  marker push sink
    void* m_2e4; // +0x2e4  begin-marker sink
    int m_2e8;   // +0x2e8  HUD-click latch (cleared by DispatchHudClick)
    char m_pad2ec[0x2f8 - 0x2ec];
    int m_2f8; // +0x2f8  level/state id (==0x66 -> booty-region init)
    char m_pad2fc[0x30c - 0x2fc];
    int m_30c;  // +0x30c  world-ready gate
    RECT m_310; // +0x310  a color/rect buffer fed to the HUD draw
    int m_320;  // +0x320  show-overlay gate / object ptr
    char m_pad324[0x328 - 0x324];
    int m_328, m_32c, m_330, m_334; // +0x328  booty-region 64-bit timer + interval + hi
    int m_338, m_33c, m_340, m_344; // +0x338  ambient-init timer + interval + hi
    int m_348;                      // +0x348  ambient-init DONE latch
    char m_pad34c[0x3f4 - 0x34c];
    void* m_3f4; // +0x3f4  frame-marker/timeline object (+0x30..0x4c reset block)
    int m_3f8, m_3fc, m_400, m_404; // +0x3f8  AMBIENT-cue 64-bit timer + interval + hi
    int m_408;                      // +0x408  AMBIENT-cue toggle
    int m_40c;                      // +0x40c  PlayCueAt last-wParam latch (de-dupe gate)
    char m_410[0x414 - 0x410];      // +0x410  PlayCueAt cue-state object (addr taken)
    int m_414;                      // +0x414  per-frame "drew" flag (cleared at entry)
    char m_pad418[0x430 - 0x418];
    int m_430, m_434, m_438, m_43c; // +0x430  scroll-region-1 timer
    int m_440, m_444, m_448, m_44c; // +0x440  scroll-region-2 timer
    int m_450, m_454, m_458, m_45c; // +0x450  scroll-region-3 timer
    int m_460, m_464, m_468, m_46c; // +0x460  scroll-region-4 timer
    int m_470;                      // +0x470  draw-extra-layer gate
    int m_474;                      // +0x474  alt-input-draw gate
    int m_478;                      // +0x478  scroll-region-3 gate
    int m_47c;                      // +0x47c  scroll-region-4 gate
    int m_480;                      // +0x480  StepC/OnRegion0 view-mode discriminator (0/1/2)
    int m_484;                      // +0x484  HUD-suppress gate (DispatchHudClick early-out)
    char m_pad488[0x4a0 - 0x488];
    int m_4a0, m_4a4, m_4a8, m_4ac; // +0x4a0  snapshot 64-bit base + duration
    int m_4b0;                      // +0x4b0  snapshot ACTIVE latch
    char m_pad4b4[0x4bc - 0x4b4];
    int m_4bc;               // +0x4bc  reveal-strip frame counter (BuildHelpReveal)
    int m_4c0, m_4c4, m_4c8; // +0x4c0  three reveal-strip sprites
    // +0x4cc: the level/tile grid block GrabTile/AdvanceTile walk:
    //   m_4cc -> a grid object (+0x64 first row, +0x68 last row, +0x14 row table)
    struct CFrameGrid {
        void SetDelay(int d);    // 0x552480 (thiscall)
        void SetSprite(void* s); // 0x552520 (thiscall)
        char p0[0x14];
        int* m_14; // +0x14  row/frame table
        char p18[0x64 - 0x18];
        int m_64; // +0x64  first frame index
        int m_68; // +0x68  last frame index
    }* m_4cc;     // +0x4cc  level grid object
    int m_4d0;    // +0x4d0  current tile id
    int m_4d4;    // +0x4d4  has-grid flag
    int m_4d8;    // +0x4d8  step delay base
    int m_4dc;    // +0x4dc  step delay countdown
    int m_4e0;    // +0x4e0  current row index
    struct M4e4 {
        char p0[0x5c];
        int m_5c;
        int m_60;
    }* m_4e4;  // +0x4e4  StepScroll's scroll-offset sink (writes +0x5c/+0x60)
    int m_4e8; // +0x4e8  grid-walk active flag
    int m_4ec; // +0x4ec  hard early-out gate
    char m_pad4f0[0x4f4 - 0x4f0];
    int m_4f4; // +0x4f4  win/lose banner gate
    int m_4f8; // +0x4f8  PRIMARY mode switch
    int m_4fc; // +0x4fc  overlay-active flag
    int m_500; // +0x500  paused/no-step flag
    char m_pad504[0x510 - 0x504];
    int m_510; // +0x510  per-frame countdown

    // Engine-label backlog stubs.
    void Stub_08c9d0();
    int winapi_0cdb10_PostMessageA(int, int, int);
    int winapi_0ceae0_PostMessageA(int, int, int);
    int winapi_0d0b30_CopyRect(int);
    void LoadCursorSprites(int, int);
    void LoadScrollSpeedOptions();
    void BuildGruntTypeNameTable(int, int, int, int);
};

// ===========================================================================
// The frame-clock + singleton globals CPlay::Render reads each frame.
// ===========================================================================
// The dev/render-state singleton DispatchHudClick reads (*g_645578); its +0x18 is
// a flags word masked with 0x20 to gate the HUD-rect post.
struct CRenderState {
    char p0[0x18];
    int m_18; // +0x18  flags
};

extern "C" {
    extern unsigned int g_645580;   // g_lastNow  (-> mirror g_6bf3c0)
    extern unsigned int g_645584;   // g_lastDelta
    extern unsigned int g_645588;   // g_accumMs (the running game clock)
    extern CGameRegistry* g_64556c; // the singleton ptr
    extern CRenderState* g_645578;  // the dev/render-state singleton (DispatchHudClick)
    extern int g_644c54;            // a default cue/message wParam
    extern unsigned int g_6bf3c0;   // draw-clock mirror
    extern unsigned int g_6bf3bc;   // draw-delta mirror
}

#endif // SRC_GRUNTZ_CPLAY_H
