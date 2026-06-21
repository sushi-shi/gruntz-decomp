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
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03();
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07();
    virtual void s08();
    virtual void BeginScene(int z);            // slot 9  (+0x24)
    virtual void s0a(); virtual void s0b(); virtual void s0c();
    virtual void Present(int a, int b);        // slot 13 (+0x34)
};

// The draw-surface object at m_c->m_24 (the target of the thiscall PushView +
// the ViewPreStep/PostStep sub-steps). Its +0x5c holds the camera geometry the
// world blit reads (+0x84 / +0x88).
struct CDrawSurface {
    void PushView(void *view, void *renderer);
    void PreStep();
    void PostStep();
    char p0[0x10];
    int   m_10;     // +0x10  scroll origin X (read by StepScroll)
    int   m_14;     // +0x14  scroll origin Y (read by StepScroll)
    char p18[0x5c - 0x18];
    // +0x5c -> a geom block: StepScroll reads (m_5c+0x40).{m_0,m_4}; the world
    // blit reads (m_5c).{m_84,m_88}.
    struct M5c { char p0[0x40]; int m_40_0; int m_40_4;
                 char p48[0x84 - 0x48]; int m_84; int m_88; } *m_5c;  // +0x5c camera geom
};

// m_c (the view/anim sub-object holder).
struct CView {
    char p0[0x4];
    struct M4 {                 // +0x04  the renderer-state object
        char p0[0x10];
        struct M10 { char p0[0x2c]; void *m_2c; } *m_10;  // +0x10 -> +0x2c surface
        struct M14 { char p0[0x2c]; void *m_2c; } *m_14;  // +0x14 -> +0x2c draw surface (view obj)
        void *m_18;             // +0x18  the present target
    } *m_4;
    CRenderer *m_8;             // +0x08  renderer A
    CRenderer *m_c;             // +0x0c  renderer B (present)
    char p10[0x20 - 0x10];
    void *m_20;                 // +0x20  a frame profiler timer (timeGetTime x2)
    CDrawSurface *m_24;         // +0x24  the draw-surface (PushView / Pre/PostStep)
};

// The world/level draw object at m_4->m_54 (the camera blit thiscall).
struct CWorldDraw {
    void Blit(int a, int b);
};

// m_4 (the CState owner back-ptr -> the world/level object).
struct CWorld {
    char p0[0xc];
    void *m_c;                  // +0x0c  a "active grunt"/selection ptr (==0 selects overlay path)
    char p10[0x48 - 0x10];
    void *m_48;                 // +0x48  the sound manager (PlaySound/FindSound/StopSound)
    char p4c[0x54 - 0x4c];
    CWorldDraw *m_54;           // +0x54  the world/level draw object (camera blit)
    void *m_5c;                 // +0x5c  a 2nd world layer
    char p60[0x68 - 0x60];
    struct M68 { char p0[0x230]; void *m_230; } *m_68;  // +0x68  -> +0x230 substep gate
    void *m_6c;                 // +0x6c  a frame-timer object (Eng_FrameTimerStep)
    void *m_70;                 // +0x70  an input sub-object
};

// ===========================================================================
// CState (base) - the shared canonical definition (full 41-slot vftable + the
// ctor-pinned scalar layout). CPlay's Render drives the high slots
// (BeginFrameClear/RenderSlow/RenderFast) and reads m_c/m_4 as typed pointers.
// ===========================================================================
#include <Gruntz/CState.h>

// A {x,y} edge pair StepInputA overlays on the CState scroll/input block
// (the flat ints at +0x188 first half, +0x198 second half).
struct Edge { int m_0; int m_4; };

// ===========================================================================
// CPlay - the in-game PLAY state. Extends CState from +0x1a8. The per-frame
// Render reads a large block of CPlay-specific members (camera/scroll rects,
// message latches, per-region one-shot FX gates). Offsets pinned by Render.
// ===========================================================================
class CPlay : public CState {
public:
    virtual int  Update();          // return 3;  (slot 4)
    virtual int  Render();          // THE per-frame heart (this TU)

    // typed views of the inherited CState owner back-ptr (+0x4):
    CWorld *m_4w() { return (CWorld *)m_4; }

    // CPlay's own per-frame helper methods (the thunks Render dispatches to
    // with `mov ecx,esi`). External no-body -> reloc-masked.
    int  StepInputA();                          // (THIS TU)
    void StepWorldB();
    void ViewPreStep(void *view);               // (on m_c->m_24)
    void ViewPostStep(void *view);
    // PlayCueAt: (cueId,a2,a3,a4,a5,a6,a7,rectSrc)
    void PlayCueAt(int cueId, int a2, int a3, int a4, int a5, int a6, int a7,
                   int rectSrc);                // (THIS TU)
    void PostHud(int wParam);
    void MarkerBegin(int now);
    void StepC();                               // (THIS TU)
    int  GetAmbientId();
    void StepScroll();                          // (THIS TU)
    int  OnRegion1(int z);                      // (THIS TU)
    int  OnRegion2(int z);                      // (THIS TU)
    int  OnRegion3(int z);                      // (THIS TU)
    int  OnRegion4(int z);                      // (THIS TU)
    void OnRegion5();

    // --- leaf sub-helpers the THIS-TU functions call (external, reloc-masked) ---
    void StepC_ModeA(int z);                    // (thiscall, 1 arg) StepC m_480==1
    void StepC_ModeB(int z);                    // (thiscall, 1 arg) StepC else
    void RegionEnter();                         // (thiscall, no arg) OnRegion on-enter
    void RegionLeave();                         // (thiscall, no arg) OnRegion on-leave
    // StepInputA's two engine callees (free fns):
    // int  Eng_InputProbe(stdcall, a,b,edge-ptr,axis-ptr,0x10)
    // void Eng_InputDispatch(cdecl, 0,0,probe-result)
    void MarkerEnd(int now);                    // (m_2e4 begin marker)
    void GutsStep();                            // (m_2dc step)
    void FrameTimerBegin(int now);              // (m_3f4 begin)
    void FrameTimerEnd(int flag, int now);      // (m_3f4 end)
    void SnapPostMessage(int wParam);
    void GutsStepB();
    void GutsStepC();
    void WorldSubstep();
    void Overlay1(int now, int z);
    void Overlay2(void *a, int z);
    void WorldBlit(int now);                    // (m_4->m_5c)
    void InputSubStep(void *in);                // (m_4->m_70)
    void RegCue(void *sink, int wParam);        // (reg->m_60)
    void SnapWalk();

    // ---- CPlay-specific members (offsets pinned by the Render disasm) ----
    int   m_1a8;        // +0x1a8  StepInputA latch-1 (one-shot)
    int   m_1ac;        // +0x1ac  StepInputA latch-2 (one-shot)
    int   m_1b0;        // +0x1b0  StepInputA half-selector
    char  m_pad1b4[0x2dc - 0x1b4];
    struct M2dc { char p0[0x10c]; int m_10c; char p[0x550-0x110];
                  int m_550, m_554; char q[0x574-0x558]; int m_574; } *m_2dc;  // +0x2dc subsystem
    void *m_2e0;        // +0x2e0  marker push sink
    void *m_2e4;        // +0x2e4  begin-marker sink
    char  m_pad2e8[0x2f8 - 0x2e8];
    int   m_2f8;        // +0x2f8  level/state id (==0x66 -> booty-region init)
    char  m_pad2fc[0x30c - 0x2fc];
    int   m_30c;        // +0x30c  world-ready gate
    RECT  m_310;        // +0x310  a color/rect buffer fed to the HUD draw
    int   m_320;        // +0x320  show-overlay gate / object ptr
    char  m_pad324[0x328 - 0x324];
    int   m_328, m_32c, m_330, m_334;   // +0x328  booty-region 64-bit timer + interval + hi
    int   m_338, m_33c, m_340, m_344;   // +0x338  ambient-init timer + interval + hi
    int   m_348;        // +0x348  ambient-init DONE latch
    char  m_pad34c[0x3f4 - 0x34c];
    void *m_3f4;        // +0x3f4  frame-marker/timeline object (+0x30..0x4c reset block)
    int   m_3f8, m_3fc, m_400, m_404;   // +0x3f8  AMBIENT-cue 64-bit timer + interval + hi
    int   m_408;        // +0x408  AMBIENT-cue toggle
    int   m_40c;        // +0x40c  PlayCueAt last-wParam latch (de-dupe gate)
    char  m_410[0x414 - 0x410];   // +0x410  PlayCueAt cue-state object (addr taken)
    int   m_414;        // +0x414  per-frame "drew" flag (cleared at entry)
    char  m_pad418[0x430 - 0x418];
    int   m_430, m_434, m_438, m_43c;   // +0x430  scroll-region-1 timer
    int   m_440, m_444, m_448, m_44c;   // +0x440  scroll-region-2 timer
    int   m_450, m_454, m_458, m_45c;   // +0x450  scroll-region-3 timer
    int   m_460, m_464, m_468, m_46c;   // +0x460  scroll-region-4 timer
    int   m_470;        // +0x470  draw-extra-layer gate
    int   m_474;        // +0x474  alt-input-draw gate
    int   m_478;        // +0x478  scroll-region-3 gate
    int   m_47c;        // +0x47c  scroll-region-4 gate
    int   m_480;        // +0x480  StepC/OnRegion0 view-mode discriminator (0/1/2)
    char  m_pad484[0x4a0 - 0x484];
    int   m_4a0, m_4a4, m_4a8, m_4ac;   // +0x4a0  snapshot 64-bit base + duration
    int   m_4b0;        // +0x4b0  snapshot ACTIVE latch
    char  m_pad4b4[0x4e4 - 0x4b4];
    struct M4e4 { char p0[0x5c]; int m_5c; int m_60; }
         *m_4e4;        // +0x4e4  StepScroll's scroll-offset sink (writes +0x5c/+0x60)
    char  m_pad4e8[0x4ec - 0x4e8];
    int   m_4ec;        // +0x4ec  hard early-out gate
    char  m_pad4f0[0x4f4 - 0x4f0];
    int   m_4f4;        // +0x4f4  win/lose banner gate
    int   m_4f8;        // +0x4f8  PRIMARY mode switch
    int   m_4fc;        // +0x4fc  overlay-active flag
    int   m_500;        // +0x500  paused/no-step flag
    char  m_pad504[0x510 - 0x504];
    int   m_510;        // +0x510  per-frame countdown

    // Engine-label backlog stubs.
    void Stub_08c9d0();
};

// ===========================================================================
// The frame-clock + singleton globals CPlay::Render reads each frame.
// ===========================================================================
extern "C" {
    extern unsigned int    g_645580;   // g_lastNow  (-> mirror g_6bf3c0)
    extern unsigned int    g_645584;   // g_lastDelta
    extern unsigned int    g_645588;   // g_accumMs (the running game clock)
    extern CGameRegistry  *g_64556c;   // the singleton ptr
    extern int             g_644c54;   // a default cue/message wParam
    extern unsigned int    g_6bf3c0;   // draw-clock mirror
    extern unsigned int    g_6bf3bc;   // draw-delta mirror
}

#endif // SRC_GRUNTZ_CPLAY_H
