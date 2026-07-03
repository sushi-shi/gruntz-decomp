// CPlay.cpp - CPlay::Render: the in-game PLAY state's per-frame
// step+draw, the heart of the running game. CARCASS reconstruction: the control
// flow, the CPlay/CState/CGameRegistry member offsets, and the ordered per-frame
// call sequence are faithful; field/callee names are placeholders and unmatched
// engine callees are external no-body fns (reloc-masked). See CPlay.h for the
// layout + helper-method map.
//
// =====================  THE PER-FRAME CARCASS (the deliverable)  ============
// CPlay::Render runs once per game frame (RezMgr::PerFrameTick -> m_mode->
// vtbl[+0x14]). It is a 3-way dispatch on two state words (m_renderDisabled the hard
// early-out, m_inGame the primary mode) wrapped in a C++ SEH/EH frame (a stack
// CString temp in the alt path -> /GX). The three paths share one WORLD-DRAW
// block and a common return-1 tail.
//
//   this->vtbl[+0x7c](0, m_cursorX, m_cursorY);          // BeginFrameClear (a virtual)
//   m_drewThisFrame = 0;                                    // per-frame "drew" flag
//   if (m_renderDisabled) return 1;                          // hard early-out
//
//   if (m_inGame) {                  // ---- MAIN in-game frame ----
//       StepInputA(); StepWorldB(); ViewPreStep(m_c->m_24);
//       g_6bf3c0=g_645580; g_6bf3bc=g_645584;     // mirror the draw clock
//       DRAW_WORLD();                             // shared world-draw block
//       <AMBIENT-cue timer +0x3f8, 0x1f4ms, toggles m_cueToggle -> PlayCueAt 0x8128>
//       MarkerBegin(now); GutsStep();             // m_beginMarker marker + m_guts step
//       if (m_c->m_renderState->m_14 == 0) return 1;        // no view -> bail
//       FrameTimerEnd; DrawSurfaceFlush; GutsStepX; ViewPostStep; return 1;
//   } else if (m_4->m_c==0 && !(reg->m_134!=2 && m_overlayDrag)) {
//                                 // ---- MENU/PAUSE-OVERLAY frame ----
//       FrameTimerBegin(now); Eng_FrameTimerStep(m_4->m_6c,0);
//       if (m_levelId==0x66) <booty-region one-shot +0x328 0x2710ms -> RegCue 0x33e>;
//       StepInputA(); StepC();
//       if (m_ambientInitDone==0) <AMBIENT level-init: wsprintf "AMBIENT%d" -> PlaySound/
//                      FindSound -> latch m_ambientInitDone=1>;
//       if (m_region0Gate) { Eng_BeginScene(m_c->m_4->m_14->m_2c); GutsStepB(); }
//       if (m_worldReady==0) { if (m_4->m_68->m_230) WorldSubstep(); StepWorldB(); }
//       StepScroll();
//       if (0x12<dt && dt<0xc8) RenderFast() else RenderSlow();   // UNSIGNED gate
//       DRAW_WORLD();                             // same shared world-draw block
//       InputSubStep(m_4->m_70);
//       if (m_overlayActive && m_guts->state-ok) ShowOverlay();   // on-screen banner
//       WorldBlit(g_645584);
//       if (m_c->m_4->m_14==0) return 1;
//       if (m_snapshotActive) SnapshotStep();                // screenshot countdown +0x4a0
//       <four scroll-region one-shots at +0x430/+0x440/+0x450/+0x460>; return 1;
//   } else {                      // ---- m_4->m_c != 0 short path ----
//       StepInputA();
//       if (m_c->m_4->m_14==0) return 1;
//       <cursor profiler m_c->m_frameProfiler: timeGetTime x2>;
//       if (m_paused) DRAW_ONLY()                    // paused: present + win/lose FX
//       else { if (--m_stepCountdown>0) <entity step + level cue>; if (m_ambientInitDone==0) AmbientInit(); }
//       MarkerBegin(now); PostHud(0); DrawSurfaceFlushTail(); return 1;
//   }
//
// THE SHARED WORLD-DRAW BLOCK (verbatim across both world-draw sites):
//   m_c->m_8->vtbl[+0x24](0);                     // renderer A: begin scene
//   Eng_PushView(m_c->m_24, m_c->m_4->m_14, m_c->m_8);
//   m_c->m_c->vtbl[+0x34](m_c->m_4->m_14, m_c->m_4->m_18);          // present
//   WorldBlit(m_c->m_24->m_5c->m_84, ->m_88) on m_4->m_54;
//   if (m_c->m_frameProfiler) { t=timeGetTime(); profiler(m_c->m_frameProfiler,t) x2; }   // frame profiler
//   MarkerBegin(g_645584); GutsStep();                             // marker + guts
//
// The per-ENTITY layer is one indirection down: g_entityList is walked
// by the world object (m_4->m_54) inside the reloc-masked camera-blit call. The
// next targets are CPlay's own sub-steps + the world-draw helpers
// (push-view, surface-flush, camera blit, HUD).
// ============================================================================

#include <Gruntz/CPlay.h>
#include <rva.h>
#include <Gruntz/ResMgr.h> // CResMgr + its image/sound/anim registries (m_10/m_28/m_2c)

// The zoned sound-bank manager (CWorld::m_48); RegionEnter/RegionLeave pause +
// resume the currently-playing zoned sound via its real (named) methods.
#include <Dsndmgr/CGruntzSoundZ.h>
#include <Globals.h>

// The registry's +0x68 cue-sink B sub-object CanQuickSave probes at +0x400.
struct CRegSub68 {
    char p0[0x400];
    i32 m_400; // +0x400  pending/busy gate
};

// ---- MFC primitives reused verbatim from the engine (reloc-masked). ----
#include <Gruntz/CString.h>
extern i32 MapLookup(void* map, void* key, void*& out); // CMapPtrToPtr::Lookup

// ---- The global CButeMgr text-config tree (the singleton). Modeled as
//      a minimal class so PlayCueAt's `mov ecx,<singleton>; call GetInt`
//      reloc-masks against the already-matched GetInt (butemgr unit). --
#include <Bute/ButeMgr.h>
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The shared engine text renderer (src/Wap32/EngStr.cpp, __cdecl). Forward-declared
// with the exact struct/signature so the call reloc-masks against EngStr.cpp's symbol.
struct EngStrRenderObj;
void EngStr_DrawText(
    EngStrRenderObj* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
);
#define g_buteText (&g_buteMgr)

// ---- StepInputA / PlayCueAt leaf engine callees (free fns / reloc-masked). ----
extern "C" {
    void Eng_SurfaceFlush(void* surf, i32 z); // (begin, 0)
    void Eng_BeginScene(void* surf, i32 z);
    void Eng_Profiler1(void* timer, u32 t);
    void Eng_Profiler2(void* timer, u32 t);
    void Eng_HudDraw(void* hud, RECT* r, i32 c);
    void Eng_HudFlush(void* hud);
    i32 Eng_PlaySound(void* snd, const char* name, i32 flag);
    void* Eng_FindSound(void* snd, const char* name);
    void Eng_StopSound(void* snd, i32 flag);
    void Eng_FrameTimerStep(void* t, i32 now);
    // --- StepInputA ---
    i32 __stdcall Eng_InputProbe(i32 a, i32 b, i32 axis, void* edge, i32 n);
    void Eng_InputDispatch(i32 z0, i32 z1, i32 probe); // (cdecl, 3)
    // --- OnRegion3/4 leaf cues ---
    void Eng_RegionCueA(i32 a, i32 b, i32 c, i32 d, i32 e); // (cdecl, 5)
    // --- PlayCueAt cue renderers (cdecl, 9 args each) ---
    void Eng_CueRenderTop(
        void* cueObj,
        void* cueState,
        RECT* r,
        i32 a2,
        i32 one,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7
    );
    void Eng_CueRenderDef(
        void* cueObj,
        void* cueState,
        RECT* r,
        i32 a2,
        i32 one,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7
    );
    // BuildHelpReveal's per-strip HUD draw (0x115300, cdecl, 6 args). reloc-masked.
    void Eng_HudStrip(void* target, void* obj, i32 x, i32 w, i32 one, i32 zero);
}

// A reg->m_68 sink that OnRegion4 posts to.
struct CRegSink {
    void Post(i32 a, i32 b);
};
// PlayCueAt's per-cue de-dupe object at this+0x410.
struct CCueState {
    i32 Probe(i32 wParam);
};

// Per-frame timer intervals (the game clock g_645588 is in ms).
enum {
    CUE_INTERVAL_MS = 0x1f4,     // 500 ms  ambient/win-lose cue toggle
    BOOTY_INTERVAL_MS = 0x2710,  // 10000 ms booty-region one-shot
    REGION_INTERVAL_MS = 0x7530, // 30000 ms scroll-region re-arm
    FIXED_SUBSTEP_MS = 0x12,     // 18 ms   world fixed-substep quantum
};

// m_viewMode (StepC / OnRegion2 discriminator).
enum {
    VIEW_MODE_IDLE = 0, // no view yet -> bail
    VIEW_MODE_A = 1,    // mode-A sub-step
    VIEW_MODE_B = 2,    // mode-B sub-step
};

// ===========================================================================
// ApplyGameOptions (0x036be0) operates entirely on the global game manager
// (*g_64556c, a CGruntzMgr) -- it ignores `this` (the first instruction reloads
// the singleton into ecx), so it compiles byte-identically whether modeled as a
// free fn or a CPlay method (the trace's this/ecx is dead here). Modeled as a
// CPlay method per the runtime attribution.
//
// A minimal VIEW of the CGruntzMgr game-manager singleton (0x64556c) this function
// touches (the one true shape lives in <Gruntz/GruntzMgr.h>: ApplyOpt==SetRunState
// @0x092340, StoreInputFlag @0x0919d0, StoreInputState @0x091a10, m_48==m_sound).
// NOT merged to the canonical header here: this TU includes the real
// <Dsndmgr/CGruntzSoundZ.h>, whose CGruntzSoundZ would be redefined by GruntzMgr.h's
// inline CGruntzSoundZ (a separate triplication to resolve first). All the sinks are
// external/no-body so the call rel32 displacements reloc-mask; local view retained.
SIZE_UNKNOWN(CGameMgrSettings);
struct CGameMgrSettings {
    void ApplyOpt(i32 v);        // 0x492340  (thiscall)
    void StoreInputFlag(i32 v);  // 0x4919d0  CGruntzMgr::StoreInputFlag
    void StoreInputState(i32 v); // 0x491a10  CGruntzMgr::StoreInputState

    struct CSound {
        char p0[0x28];
        i32 m_28;                   // +0x28  gate (skip the XMIDI push when 0)
        void SetXMidiVolume(i32 v); // 0x138950 (thiscall on m_48)
    };

    char p0[0x48];
    CSound* m_48; // +0x48  sound object
    char p4c[0x100 - 0x4c];
    i32 m_isVoiceEnabled; // +0x100
    char p104[0x118 - 0x104];
    i32 m_isEasyMode; // +0x118
    char p11c[0x124 - 0x11c];
    i32 m_scrollSpeed; // +0x124
};

// The option-source globals ApplyGameOptions pushes/stores (uninitialized .bss).
extern "C" {
    DATA(0x0024556c)
    extern CGameMgrSettings* g_mgrSettings; // = g_64556c (the CGruntzMgr singleton)
    DATA(0x0020ccc4)
    extern i32 g_videoResolutionMode;
    DATA(0x002455b4)
    extern i32 g_gate_2455b4;
}

// A free helper (FUN_004923b0, cdecl/1 arg) run on the XMIDI-active path before
// the sound-object volume push. External/no-body -> reloc-masked.
extern "C" void Eng_OptCommit(i32 v); // 0x4923b0

// ===========================================================================
// CPlay::ApplyGameOptions (0x036be0) - push the current option/registry values
// into the game manager (*g_64556c). Mirrors the video-resolution mode global,
// stamps three manager words (+0x118/+0x100/+0x124), and - unless the runtime
// gates g_gate_2455b4/bc/c0 say otherwise - forwards the input flag/state options
// and (when the sound object's +0x28 gate is live) commits the XMIDI volume.
// ===========================================================================
// @early-stop
// register-coloring wall (~76%). Control flow, all member offsets
// (+0x118/+0x100/+0x124/+0x48/+0x28), the 12 option/gate globals, the 5 callees
// and the redundant g_gate_2455b4 re-test are byte-faithful and all relocs pair;
// the residual is the non-steerable eax-vs-ecx coloring of the reloaded manager
// pointer (retail pins it in ecx, our cl picks eax) which cascades into the temp
// regs + the top videomode/b4-load schedule. See docs/patterns/zero-register-pinning.md.
RVA(0x00036be0, 0xd3)
void CPlay::ApplyGameOptions() {
    if (g_mgrSettings == 0) {
        return;
    }
    g_mgrSettings->m_isEasyMode = g_opt_22bd70;
    g_videoResolutionMode = g_opt_22bdc8;
    if (g_gate_2455b4 == 0) {
        if (g_gate_2455bc == 0) {
            g_mgrSettings->ApplyOpt(g_opt_22bd84);
            g_mgrSettings->StoreInputFlag(g_opt_22bd6c);
            g_mgrSettings->m_isVoiceEnabled = g_opt_22bdd4;
            g_mgrSettings->StoreInputState(g_opt_22bdc4);
        }
        if (g_gate_2455b4 == 0 && g_gate_2455c0 == 0 && g_mgrSettings->m_48->m_28 != 0) {
            Eng_OptCommit(g_opt_22bdd0);
            g_mgrSettings->m_48->SetXMidiVolume(g_opt_22bdcc);
        }
    }
    g_mgrSettings->m_scrollSpeed = g_opt_22bd68;
}

// CPlay::Update() (slot 4): the PLAY state's ID = 3.
RVA(0x0008c910, 0x6)
GameStateId CPlay::Update() {
    return GAMESTATE_PLAY;
}

// ===========================================================================
// CPlay::Render  (vtable slot +0x14)
// ===========================================================================
RVA(0x000c8cf0, 0xc14)
i32 CPlay::Render() {
    // --- frame entry: clear the per-frame flag, then a `this`-virtual begin. ---
    // (the m_drewThisFrame=0 store is scheduled INTO the BeginFrameClear arg setup.)
    m_drewThisFrame = 0;
    BeginFrameClear(0, m_cursorX, m_cursorY); // this->vtbl[+0x7c](0, m_cursorX, m_cursorY)

    if (m_renderDisabled != 0) {
        return 1; // hard early-out
    }

    if (m_inGame != 0) {
        // =================================================================
        // ---- MAIN in-game frame ----
        // =================================================================
        StepInputA();                  // poll/sim sub-step A
        StepWorldB();                  // world/camera sub-step B
        m_c->m_drawSurface->PreStep(); // on m_c->m_24 (view pre-step)

        g_6bf3c0 = g_645580; // mirror the draw clock
        g_6bf3bc = g_645584;

        // --- shared world-draw block #1 ---
        m_c->m_rendererA->BeginScene(0); // m_c->m_8->vtbl[+0x24](0)
        m_c->m_drawSurface->PushView(
            m_c->m_renderState->m_14,
            m_c->m_rendererA
        ); // (thiscall on m_24)
        m_c->m_rendererB->Present(
            m_c->m_renderState->m_14,
            m_c->m_renderState->m_18
        ); // vtbl[+0x34]
        m_4w()->m_54->Blit(m_c->m_drawSurface->m_5c->m_84, m_c->m_drawSurface->m_5c->m_88);
        if (m_c->m_frameProfiler != 0) { // frame profiler
            u32 t = timeGetTime();
            Eng_Profiler1(m_c->m_frameProfiler, t);
            Eng_Profiler2(m_c->m_frameProfiler, t);
        }
        MarkerBegin((i32)g_645584); // m_beginMarker begin-marker
        GutsStep();                 // m_guts step

        // --- periodic AMBIENT-cue timer (+0x3f8, 0x1f4 ms; toggles m_cueToggle) ---
        {
            u32 elapsed = g_645588 - (u32)m_cueTimerLo;
            if (elapsed >= (u32)m_cueInterval) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = (i32)g_645588;
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8128, 0x78, 0, 0xff, 0xff, 0, 1, 0); // cue
            }
        }

        if (m_c->m_renderState->m_14 == 0) {
            return 1; // no view -> bail
        }

        FrameTimerBegin((i32)g_645584);                      // m_frameMarker begin
        FrameTimerEnd(0, (i32)g_645584);                     // wait: end takes (this,flag)
        Eng_SurfaceFlush(m_c->m_renderState->m_10->m_2c, 0); // surface flush
        // GutsStepX(m_region0Gate, m_guts, reg) -> the post-draw step (modeled via
        // the same external; here as the marker/guts step):
        GutsStep();                     // (post-draw guts)
        m_c->m_drawSurface->PostStep(); // on m_c->m_24
        return 1;                       // -> draw tail
    }

    // m_inGame == 0
    if (m_4w()->m_c != 0) {
        goto alt2;
    }
    {
        CGameRegistry* reg = g_64556c;
        if (reg->m_134 != 2 && m_overlayDrag != 0) {
            goto alt2;
        }
    }

    // =================================================================
    // ---- MENU / PAUSE-OVERLAY frame ----
    // =================================================================
    {
        CWorld* w = m_4w();
        FrameTimerBegin((i32)g_645584); // m_frameMarker begin
        Eng_FrameTimerStep(w->m_6c, 0); // m_4->m_6c step

        if (m_levelId == 0x66) { // booty-region one-shot
            u32 elapsed = g_645588 - (u32)m_bootyTimerLo;
            if (elapsed >= (u32)m_bootyInterval) {
                RegCue(g_64556c->m_cueSink, 0x33e); // reg->m_cueSink cue
                m_bootyInterval = BOOTY_INTERVAL_MS;
                m_bootyIntervalHi = 0;
                m_bootyTimerLo = (i32)g_645588;
                m_bootyTimerHi = 0;
            }
        }

        StepInputA();
        StepC();

        // --- AMBIENT level-init one-shot (+0x348) ---
        if (m_ambientInitDone == 0) {
            u32 elapsed = g_645588 - (u32)m_ambientTimerLo;
            if (elapsed >= (u32)m_ambientInterval) {
                i32 id = GetAmbientId();
                CString name;
                (void)name; // [esp+0x10] CString temp (/GX)
                char buf[0x80];
                wsprintfA(buf, "AMBIENT%d", id); // s_AMBIENT%d
                if (g_64556c->m_14 != 0) {
                    Eng_PlaySound(w->m_48, buf, 1);
                } else {
                    void* out = 0;
                    void* snd = Eng_FindSound(w->m_48, buf);
                    if (snd != 0) {
                        out = snd;
                    }
                    if (out != 0) {
                        Eng_StopSound(out, 1);
                    }
                }
                m_ambientInitDone = 1;
            }
        }

        if (m_region0Gate != 0) { // extra HUD/overlay layer
            Eng_BeginScene(m_c->m_renderState->m_10->m_2c, 0);
            GutsStepB(); // m_guts
        }

        if (m_worldReady == 0) { // world-ready init
            if (w->m_68->m_230 != 0) {
                WorldSubstep();
            }
            StepWorldB();
        }

        StepScroll();

        // per-frame timing gate (UNSIGNED clamp: 0x12 < dt < 0xc8):
        {
            u32 dt = g_645584;
            if (dt > 0x12 && dt < 0xc8) {
                RenderFast(); // call [eax+0xa0]
            } else {
                RenderSlow(); // call [edx+0x9c]
            }
        }

        // --- shared world-draw block #2 ---
        m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
        m_c->m_rendererB->Present(m_c->m_renderState->m_14, m_c->m_renderState->m_18); // present
        if (m_region1Gate != 0) {
            StepC(); // alt-input draw
        } else {
            m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
            m_c->m_rendererB->Present(m_c->m_renderState->m_14, m_c->m_renderState->m_18);
        }
        MarkerBegin((i32)g_645584);
        GutsStep();
        InputSubStep(w->m_70); // m_4->m_70

        if (m_overlayActive != 0 && m_guts->m_mode != 5) { // on-screen overlay/banner
            Overlay1(0, (i32)g_645584);
            Overlay2(m_c, 0);
        }

        WorldBlit((i32)g_645584); // on m_4->m_5c (thiscall)
        if (m_c->m_renderState->m_14 == 0) {
            return 1;
        }

        // --- snapshot/screenshot countdown (+0x4a0/+0x4a8) ---
        if (m_snapshotActive != 0) {
            u32 now = g_645588;
            u32 dur = (u32)(m_snapDur + m_snapBaseLo) - now;
            if ((i32)dur >= 0) {
                // duration elapsed: post a message + reset the marker block + walk.
                if (m_guts->m_snapPostSel != 0) {
                    SnapPostMessage(5); // reg->m_68 (5)
                } else {
                    SnapPostMessage(g_644c54);
                }
                // reset the m_frameMarker marker block (+0x30..0x4c):
                FrameTimerEnd(0, 0);
                GutsStepC(); // m_guts
                m_snapshotActive = 0;
                // walk the level tree (CMapPtrToPtr::Lookup):
                if (g_64556c->m_15c != 0) {
                    void* out = 0;
                    MapLookup(g_64556c->m_world->m_8, g_64556c->m_15c, out);
                    if (out != 0) {
                        SnapWalk();
                    }
                }
            } else {
                // not yet: build a CString temp, CopyRect the viewport, HudDraw.
                CString tmp;
                (void)tmp; // [esp+0x10] CString temp
                tmp.Format("%s", "");
                // m_30 is the shared CSpriteFactoryHolder; this WIP path reads it
                // as a resource map whose +0x24 holds the CopyRect-source rect.
                CopyRect(&m_hudRect, (const RECT*)((char*)g_64556c->m_world + 0x24));
                Eng_HudDraw(g_64556c->m_world, &m_hudRect, 1);
            }
            // (CString temp dtor runs here under the EH frame)
        }

        // --- the four scroll-region one-shots (+0x430/+0x440/+0x450/+0x460) ---
        FrameTimerEnd(0, m_frameMarker != 0); // reset
        OnRegion5();
        Eng_FrameTimerStep(w->m_68, 0); // m_4->m_68

        if (m_winLoseBanner != 0 && m_guts->m_busyA == 0 && m_guts->m_busyB == 0) {
            // win/lose banner timer (+0x3f8 again, 0x1f4 ms):
            u32 elapsed = g_645588 - (u32)m_cueTimerLo;
            if (elapsed >= (u32)m_cueInterval) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = (i32)g_645588;
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8129, 0x78, 0, 0xff, 0xff, 0, 1, 0);
            }
        }

        MarkerBegin((i32)g_645584);
        PostHud(0);
        if (m_worldReady != 0) { // optional HUD overlay draw
            Eng_HudDraw(m_c->m_renderState->m_10->m_2c, &m_hudRect, 0xff); // (this=m_4->m_10->m_2c)
        }
        Eng_SurfaceFlush(m_c->m_renderState->m_10->m_2c, 0);

        // --- the four screen-region scroll one-shots: each is
        // a 64-bit "inside region" elapsed test that fires its OnRegion handler. ---
        if (m_region0Gate != 0) { // region-1 (+0x430)
            u32 e = g_645588 - (u32)m_region0TimerLo;
            if (e >= (u32)m_region0Interval) {
                OnRegion2((i32)g_645588);
            }
        }
        if (m_region1Gate != 0) { // region-2 (+0x440)
            u32 e = g_645588 - (u32)m_region1TimerLo;
            if (e >= (u32)m_region1Interval) {
                OnRegion1((i32)g_645588);
            }
        }
        if (m_region2Gate != 0) { // region-3 (+0x450)
            u32 e = g_645588 - (u32)m_region2TimerLo;
            if (e >= (u32)m_region2Interval) {
                OnRegion3((i32)g_645588);
            }
        }
        if (m_region3Gate != 0) { // region-4 (+0x460)
            u32 e = g_645588 - (u32)m_region3TimerLo;
            if (e >= (u32)m_region3Interval) {
                OnRegion4((i32)g_645588);
            }
        }
        return 1; // -> draw tail
    }

alt2:
    // =================================================================
    // ---- the m_4->m_c != 0 short path ----
    // =================================================================
    StepInputA();
    if (m_c->m_renderState->m_14 == 0) {
        return 1;
    }
    {
        if (m_c->m_frameProfiler != 0) { // cursor/frame profiler
            u32 t = timeGetTime();
            Eng_Profiler1(m_c->m_frameProfiler, t);
            Eng_Profiler2(m_c->m_frameProfiler, t);
        }
        if (m_paused != 0) {
            // ---- the paused frame: draw-only ----
            m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
            m_c->m_rendererB->Present(
                m_c->m_renderState->m_14,
                m_c->m_renderState->m_18
            ); // present
            GutsStep();
            if (m_guts->m_busyA == 0 && m_guts->m_busyB == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0); // win/lose
            }
            FrameTimerEnd(1, 0);
        } else {
            // ---- the active short frame: entity step + cues ----
            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
                m_c->m_rendererB->Present(
                    m_c->m_renderState->m_14,
                    m_c->m_renderState->m_18
                ); // present
                GutsStep();
                Eng_FrameTimerStep(m_guts, 0x32);
                PlayCueAt(m_lastCueId, 0x78, 0, 0xff, 0xff, 0, 1, 0); // (cueId=m_lastCueId)
                FrameTimerEnd(1, 0);
            }
            if (m_ambientInitDone == 0) {
                // the same AMBIENT level-init one-shot (+0x348):
                u32 elapsed = g_645588 - (u32)m_ambientTimerLo;
                if (elapsed >= (u32)m_ambientInterval) {
                    i32 id = GetAmbientId();
                    CString name;
                    (void)name;
                    char buf[0x80];
                    wsprintfA(buf, "AMBIENT%d", id);
                    if (g_64556c->m_14 != 0) {
                        Eng_PlaySound(m_4w()->m_48, buf, 1);
                    } else {
                        void* out = 0;
                        void* snd = Eng_FindSound(m_4w()->m_48, buf);
                        if (snd != 0) {
                            out = snd;
                        }
                        if (out != 0) {
                            Eng_StopSound(out, 1);
                        }
                    }
                    m_ambientInitDone = 1;
                }
            }
        }
        MarkerBegin((i32)g_645584);
        PostHud(0);
        Eng_SurfaceFlush(m_c->m_renderState->m_10->m_2c, 0);
    }
    return 1; // draw tail
}

// ===========================================================================
// CPlay::OnExit (0x0cb400) - the PLAY-state teardown: run the ready-gate forwarder,
// the slot-21 notify, refresh renderer A, then clear the registry's per-frame word
// (+0x128), drop its mode back to 0 if it was 3, and run the +0x70 object's teardown
// (vtable slot 0). The registry's per-exit fields live in a local member-type view.
// ===========================================================================
// Member-type view of the registry singleton for the exit path: its +0x70 teardown
// object (slot-0 virtual), the +0x128 per-frame clear word and the +0x134 mode.
struct CRegExit {
    char p0[0x70];
    struct Teardown {
        virtual void Shutdown(); // slot 0 (+0x0)
    }* m_70;                     // +0x70
    char p74[0x128 - 0x74];
    i32 m_128; // +0x128
    char p12c[0x134 - 0x12c];
    i32 m_134; // +0x134
};

// @early-stop
// ~95% regalloc wall: logic + all 5 relocs pair; retail threads the reloaded registry
// global through eax for all three accesses (m_70 -> eax -> ecx) where MSVC5 here
// colors the first store's load into ecx (and the m_70 deref straight into ecx).
// Not source-steerable (eax/ecx coloring of a thrice-reloaded global).
// docs/patterns/zero-register-pinning.md.
RVA(0x000cb400, 0x58)
void CPlay::OnExit() {
    ForwardReady();
    Vslot21();
    if (m_c) {
        m_c->m_rendererA->Refresh();
    }
    ((CRegExit*)g_64556c)->m_128 = 0;
    if (((CRegExit*)g_64556c)->m_134 == 3) {
        ((CRegExit*)g_64556c)->m_134 = 0;
    }
    ((CRegExit*)g_64556c)->m_70->Shutdown();
}

// ===========================================================================
// CPlay::ModeCleanup (0x0cb740) - vtable slot 0x22 mode/state-exit teardown.
// For each live sub-object of the view holder (m_c) and the world (m_4) it runs
// that object's teardown method (two are virtual). Self-contained (no DIR32
// relocs); the view ptr is re-read before every block (no cached local). The
// sub-object offsets here differ from Render's CView typing, so a local view-cast
// keeps Render's member typing untouched.
// ===========================================================================
// Two renderer/draw sub-objects torn down through a vtable slot. Dummy virtuals
// pad the slot index so `o->Teardown()` lowers to `mov edx,[o]; call [edx+slot]`.
struct CExitV58 { // teardown at vtable +0x58 (slot 22)
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s0a();
    virtual void s0b();
    virtual void s0c();
    virtual void s0d();
    virtual void s0e();
    virtual void s0f();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void s13();
    virtual void s14();
    virtual void s15();
    virtual void Teardown(); // slot 22 (+0x58)
};
struct CExitV44 { // teardown at vtable +0x44 (slot 17)
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s0a();
    virtual void s0b();
    virtual void s0c();
    virtual void s0d();
    virtual void s0e();
    virtual void s0f();
    virtual void s10();
    virtual void Teardown(); // slot 17 (+0x44)
};
// The view holder (this->m_c) as the exit walk reads it.
struct CExitView {
    char p0[0x8];
    struct M8 {
        void Refresh(); // 0x159ef0
    }* m_8;             // +0x8
    struct Mc {
        void Teardown(); // 0x163c60
    }* m_c;              // +0xc
    CExitV58* m_10;      // +0x10  virtual slot 0x58
    char p14[0x24 - 0x14];
    CExitV44* m_24; // +0x24  virtual slot 0x44
    struct M28 {
        char p0[0x2c];
        struct Inner {
            void Teardown(); // 0x137a80
        }* m_2c;             // +0x2c
        void Release();      // 0x157bc0
    }* m_28;                 // +0x28
    struct M2c {
        void Teardown(); // 0x152720
    }* m_2c;             // +0x2c
};
// The world (this->m_4) as the exit walk reads it.
struct CExitWorld {
    char p0[0x48];
    struct M48 {
        void Teardown(); // 0x138530
    }* m_48;             // +0x48
    char p4c[0x54 - 0x4c];
    struct M54 {
        void Reset(); // 0x28ab thunk
    }* m_54;          // +0x54
};

// @early-stop
// reload-register regalloc tail (99.62%): logic byte-exact and every call reloc
// pairs; the only residual is 4 bytes - the intermediate register cl picks for the
// re-read of m_c before `m_28->Release` and of m_4 before `m_54->Reset` (cl folds
// into ecx/edx, retail loads via eax/ecx). The view ptr IS re-read each block as
// retail does; this is the reread-member-view-pointer.md regalloc residual, not
// source-steerable (the surrounding standalone blocks already match). Final sweep.
RVA(0x000cb740, 0x8f)
void CPlay::ModeCleanup() {
    if (m_c) {
        if (((CExitView*)m_c)->m_28->m_2c) {
            ((CExitView*)m_c)->m_28->m_2c->Teardown();
        }
        ((CExitView*)m_c)->m_28->Release();
    }
    if (m_4) {
        ((CExitWorld*)m_4)->m_48->Teardown();
        ((CExitWorld*)m_4)->m_54->Reset();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_10->Teardown();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_2c->Teardown();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_24->Teardown();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_8->Refresh();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_c->Teardown();
    }
}

// ===========================================================================
// CPlay::OnKeyCommand (0x0cbaf0) - the PLAY-state keyboard/UI command dispatcher.
// Early-outs on the HUD-suppress gate, then a priority chain: resume from a paused/
// disabled frame (re-arm the in-game mode), bail to ReportError if the per-frame
// reset is still pending, un-pause, or (no active grunt) forward to the overlay
// layer / dispatch the bracket-key zoom guts steps. __thiscall(key, flag), ret 8.
// ===========================================================================
// @early-stop
// ~82% identical-return-epilogue tail-merge wall: the whole priority chain + every
// cmp/call is byte-faithful and all relocs pair, but MSVC5's epilogue merger picks a
// different set of return-1/return-0 sites to share vs inline than retail (retail
// inlines the m_hudSuppressed return-1 + shares the two return-0 tails; we do the
// reverse), cascading a small offset shift. Not source-steerable.
// docs/patterns/identical-return-epilogue-tailmerge.md.
RVA(0x000cbaf0, 0x16f)
i32 CPlay::OnKeyCommand(i32 key, i32 flag) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_renderDisabled = 0;
        m_hudSuppressed = 1;
        EnterMode(3);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            return 1;
        }
        m_4w()->ReportError(0x800a, 0x456);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_4w()->m_4->m_4->m_4, 0x111, 0x816e, 0);
        return 1;
    }
    if (m_4w()->m_c != 0) {
        return 0;
    }
    if (m_hitTest->m_10 != 0) {
        ((CWorldLayer*)m_4w()->m_5c)->Forward3508(key, flag);
        return 1;
    }
    if (key == 0x5d) {
        m_guts->StepBracketR();
        return 1;
    }
    if (key == 0x5b) {
        m_guts->StepBracketL();
        return 1;
    }
    if (key == 0x2d) {
        m_guts->StepMinus();
        return 1;
    }
    if (key == 0x3d || key == 0x2b) {
        m_guts->Guts123f();
        m_hitTest->StepZoom(m_guts->m_state == 1 ? 2 : 1);
        return 1;
    }
    return 0;
}

// ===========================================================================
// CPlay::ResetViewport (0x0d8c60) - ClampViewport's no-change fallback: build the
// full-screen viewport rect (a guts-state-dependent left/right bias), optionally
// re-center it to a 0xc0 box (region-0 gate), pin the view discriminator to idle,
// install the rect on the draw-surface and run the world apply-tail. __thiscall.
// ===========================================================================
// @early-stop
// ~95% regalloc wall: the SetRect dispatch, the region-0 re-center block and the
// apply-tail are byte-faithful with all relocs pairing; only the 5-instruction
// prologue colors the (m_4, right, bottom) trio eax/ecx-swapped vs retail (retail
// pins m_4 in ecx and right in eax; we mirror). Tried explicit load ordering.
// docs/patterns/zero-register-pinning.md.
RVA(0x000d8c60, 0xea)
i32 CPlay::ResetViewport() {
    CWorld* w = m_4w();
    GutsSubsystem* guts = m_guts;
    i32 right = w->m_8c;
    i32 state = guts->m_state;
    i32 bottom = w->m_90;
    RECT r;
    if (state == 1) {
        SetRect(&r, 0xa0, 0, right - 1, bottom - 1);
    } else if (state == 0) {
        SetRect(&r, 0, 0, right - 0xa1, bottom - 1);
    } else {
        SetRect(&r, 0, 0, right - 1, bottom - 1);
    }
    if (m_region0Gate) {
        i32 halfW = (r.right - r.left) / 2;
        i32 halfH = (r.bottom - r.top) / 2;
        r.left = r.left + halfW - 0x60;
        r.top = r.top + halfH - 0x60;
        r.right = r.right + (0x60 - halfW);
        r.bottom = r.bottom + (0x60 - halfH);
    }
    m_viewMode = VIEW_MODE_IDLE;
    m_c->m_drawSurface->SetClipRect(&r);
    m_4w()->ClampApply();
    return 1;
}

// ===========================================================================
// CPlay::StepC - the menu/overlay-frame view
// sub-step. A 3-way switch on the view-mode discriminator m_viewMode: 0 = idle (no
// view yet, bail), 1 = mode-A sub-step, 2(+) = mode-B sub-step. MSVC hoists the
// shared `push 4` out of the if/else (both helpers take the same constant).
// ===========================================================================
RVA(0x000d8d90, 0x1e)
void CPlay::StepC() {
    i32 mode = m_viewMode;
    if (mode == VIEW_MODE_IDLE) {
        return;
    }
    if (mode == VIEW_MODE_A) {
        StepC_ModeA(4);
    } else {
        StepC_ModeB(4);
    }
}

// ===========================================================================
// CPlay::ClampViewport (0x0d8dc0) - if the active viewport (m_c->m_24+0x10) is
// wider/taller than 0xc0, inset that axis by `inset` (both edges); if NEITHER axis
// was clamped, just reset the viewport and bail. Otherwise install the clamped rect
// on the draw-surface (SetClipRect), re-prepare the held surface, and run the guts +
// world apply-steps. Returns 1 if clamped, 0 otherwise. __thiscall, ret 4.
// ===========================================================================
// @early-stop
// ~94% regalloc wall - the whole clamp body + the apply-tail call chain are byte-
// faithful and all relocs pair; retail pins `clamped` in edx and the rect-base copy
// in ebx where MSVC5 here colors them edi/ebx-swapped (a cascade off the entry
// `xor edx,edx` vs `xor edi,edi`). Not source-steerable (tried explicit rect-base
// pointer + reordered the clamped init). docs/patterns/zero-register-pinning.md.
RVA(0x000d8dc0, 0xce)
i32 CPlay::ClampViewport(i32 inset) {
    CView* v = m_c;
    RECT* vp = &v->m_drawSurface->m_viewport;
    RECT r;
    r.left = vp->left;
    r.top = vp->top;
    r.right = vp->right;
    r.bottom = vp->bottom;

    i32 clamped = 0;
    if (r.right - r.left > 0xc0) {
        r.left += inset;
        r.right -= inset;
        clamped = 1;
    }
    if (r.bottom - r.top > 0xc0) {
        r.top += inset;
        r.bottom -= inset;
        clamped = 1;
    }
    if (clamped == 0) {
        ResetViewport();
        return 0;
    }

    m_c->m_drawSurface->SetClipRect(&r);
    m_c->m_renderState->m_14->m_2c->Prepare(0);
    m_guts->ClampApply();
    m_4w()->ClampApply();
    return 1;
}

// ===========================================================================
// CPlay::ClampViewport2 (0x0d8ed0) - the asymmetric viewport clamp. Each axis whose
// extent is BELOW the world limit (horizontal m_4->m_8c, biased down 0xa0 unless the
// guts subsystem is ready; vertical m_4->m_90) is EXPANDED by `stride` on both edges,
// then clamped into [0, limit-1]. If neither axis moved, reset the viewport and bail;
// otherwise install the clamped rect + run the apply-tail. __thiscall, ret 4.
// ===========================================================================
// @early-stop
// ~86% regalloc wall - the asymmetric clamp logic, the guts-gated horizontal bound
// (m_8c / m_8c-0xa0, inlined ternary so it stays in edi not a spill), both axis
// [0, limit-1] clamps, and the 4-call apply-tail are byte-faithful with all relocs
// pairing. The residual: retail carries the `clamped` accumulator through `ecx`
// across the two blocks (allocating an extra spill slot -> `sub esp,0x1c`) where
// MSVC5 here writes it straight to [esp+0x10] (`sub esp,0x18`), shifting the rect
// slots by 4 and renaming the block-2 temps. Not source-steerable (the two-block
// boolean-OR register-merge is the compiler's spill choice).
// docs/patterns/zero-register-pinning.md.
RVA(0x000d8ed0, 0x128)
i32 CPlay::ClampViewport2(i32 stride) {
    i32 clamped = 0;
    CView* v = m_c;
    CWorld* w = m_4w();
    GutsSubsystem* guts = m_guts;

    i32* rp = (i32*)&v->m_drawSurface->m_viewport;
    RECT r;
    r.left = rp[0];
    r.top = rp[1];
    r.right = rp[2];
    r.bottom = rp[3];

    i32 hlimit = w->m_8c;
    i32 vlimit = w->m_90;

    if (r.right - r.left < (guts->m_state == 2 ? hlimit : hlimit - 0xa0)) {
        r.left -= stride;
        r.right += stride;
        if (r.left < 0) {
            r.left = 0;
        }
        if (r.right >= hlimit) {
            r.right = hlimit - 1;
        }
        clamped = 1;
    }
    if (r.bottom - r.top < vlimit) {
        r.top -= stride;
        r.bottom += stride;
        if (r.top < 0) {
            r.top = 0;
        }
        if (r.bottom >= vlimit) {
            r.bottom = vlimit - 1;
        }
        clamped = 1;
    }

    if (clamped == 0) {
        ResetViewport();
        return 0;
    }

    m_c->m_drawSurface->SetClipRect(&r);
    m_c->m_renderState->m_14->m_2c->Prepare(0);
    m_guts->ClampApply();
    m_4w()->ClampApply();
    return 1;
}

// CPlay::RegionEnter (0x0d88f0) - on entering a special region, save the
// currently-playing zoned sound (m_savedZonedSound) and silence the bank, then (when the dev
// window is up) start the "CURSE" cue. The shared on-enter sub-step the OnRegion
// one-shots call. Migrated from engine_boundary (CPlay).
RVA(0x000d88f0, 0x44)
void CPlay::RegionEnter() {
    if (m_savedZonedSound == 0) {
        CWorld* w = m_4w();
        m_savedZonedSound = w->m_48->m_pCurrent;
        w->m_48->StopAll();
    }
    if (g_64556c->m_14 != 0) {
        m_4w()->m_48->PlayByName("CURSE", 0);
    }
}

// CPlay::RegionLeave (0x0d8960) - on leaving (only when no region gate is still
// set and a sound was saved), stop the bank, restore the saved zoned sound, and
// (dev-window) restart it. Migrated from engine_boundary (CPlay).
RVA(0x000d8960, 0x75)
void CPlay::RegionLeave() {
    if (m_region0Gate == 0 && m_region1Gate == 0 && m_region2Gate == 0 && m_region3Gate == 0
        && m_savedZonedSound != 0) {
        m_4w()->m_48->IsPlaying();
        m_4w()->m_48->m_pCurrent = m_savedZonedSound;
        if (g_64556c->m_14 != 0) {
            m_4w()->m_48->Restart(1);
        }
        m_savedZonedSound = 0;
    }
}

// ===========================================================================
// The four screen-region scroll one-shots.
// Each: thiscall(int z), set its region-active gate to bool(z), call the shared
// enter/leave sub-step, (re)arm its 64-bit countdown timer (interval 0x7530 ms,
// lo = g_645588, hi = 0), and return 1. They share ONE shape; OnRegion2 also
// pins the view discriminator m_viewMode, OnRegion3/4 fire an extra cue on enter/leave.
// ===========================================================================
RVA(0x000d8a00, 0x73)
i32 CPlay::OnRegion2(i32 z) // (region-0 / gate m_region0Gate, timer +0x430)
{
    if (z != 0) {
        m_region0Gate = 1;
        RegionEnter();
        m_viewMode = VIEW_MODE_A;
    } else {
        m_region0Gate = 0;
        RegionLeave();
        m_viewMode = VIEW_MODE_B;
    }
    m_region0Interval = REGION_INTERVAL_MS;
    m_region0IntervalHi = 0;
    *(u64*)&m_region0TimerLo = g_645588; // 64-bit store: lo=g_645588, hi=0
    return 1;
}

RVA(0x000d8aa0, 0x5f)
i32 CPlay::OnRegion1(i32 z) // (region-1 / gate m_region1Gate, timer +0x440)
{
    if (z != 0) {
        m_region1Gate = 1;
        RegionEnter();
    } else {
        m_region1Gate = 0;
        RegionLeave();
    }
    m_region1Interval = REGION_INTERVAL_MS;
    m_region1IntervalHi = 0;
    *(u64*)&m_region1TimerLo = g_645588; // 64-bit store: lo=g_645588, hi=0
    return 1;
}

RVA(0x000d8b20, 0x74)
i32 CPlay::OnRegion3(i32 z) // (region-2 / gate m_region2Gate, timer +0x450)
{
    if (z != 0) {
        m_region2Gate = 1;
        RegionEnter();
        Eng_RegionCueA(REGION_INTERVAL_MS, 6, 6, 0, 0x2d);
    } else {
        m_region2Gate = 0;
        RegionLeave();
    }
    m_region2Interval = REGION_INTERVAL_MS;
    m_region2IntervalHi = 0;
    *(u64*)&m_region2TimerLo = g_645588; // 64-bit store: lo=g_645588, hi=0
    return 1;
}

RVA(0x000d8bc0, 0x71)
i32 CPlay::OnRegion4(i32 z) // (region-3 / gate m_region3Gate, timer +0x460)
{
    if (z != 0) {
        m_region3Gate = 1;
        RegionEnter();
    } else {
        m_region3Gate = 0;
        RegionLeave();
        ((CRegSink*)g_64556c->m_68)->Post(-1, 0); // reg->m_68->Post(0xffffffff, 0)
    }
    m_region3Interval = REGION_INTERVAL_MS;
    m_region3IntervalHi = 0;
    *(u64*)&m_region3TimerLo = g_645588; // 64-bit store: lo=g_645588, hi=0
    return 1;
}

// ===========================================================================
// CPlay::NotifyVisibleEntities (0x0d9050) - notify the draw surface of the (slightly
// inflated) viewport, then walk the renderer's entity list and, for every entity
// whose type-discriminator (entity->m_8->m_7c[4], the slot-4 method pointer) is one
// of 12 known "visible-notify" types, dispatch its slot-0x2c notify with the held
// view surface. Returns 1. __thiscall, ret 0.
//
// The 12 type addresses are reached in retail through ILT jump-thunks (thunk_FUN_*);
// the recompile binds them to the direct functions, so the `cmp eax,imm32; je` BYTES
// match retail 1:1 but the 12 DIR32 operands are differently-named (thunk vs direct)
// reloc-masked symbols.
// ===========================================================================
// @early-stop
// ~75% reloc-masked plateau. The viewport-notify, the entity-list walk, the 12-way
// `cmp eax,<type>; je` type-discriminator chain (same order, same encoding) and the
// slot-0x2c notify dispatch are all byte-faithful. The residual is (1) the 12 DIR32
// reloc-name mismatches - retail compares against ILT thunk addresses, the recompile
// against the direct function addresses (the delinker has no symbol for the thunks),
// and (2) a small head regalloc swap + retail's extra `push 0` frame slot. Logic
// complete; the reloc-name set cannot pair (docs/patterns/reloc-typing-vptr-global.md,
// objdiff-reloc-scoring), so deferred to the final sweep.
// ===========================================================================

// The renderer's per-entity list node (m_c->m_8 + 0x10 -> +4 head; next at +0).
// Each node holds the entity at +0x8 whose +0x7c vtable's slot-4 is the type id
// NotifyVisibleEntities switches on, and whose own slot-0x2c is the notify.
struct CVisEntityType {
    void* s0[4];
    void* m_10; // [0x10] slot-4 method pointer = the type discriminator
};
struct CVisEntityVtbl;
struct CVisEntity {
    CVisEntityVtbl* vptr;
    char p4[0x7c - 0x04];
    CVisEntityType* m_7c;    // +0x7c
    void Notify(void* held); // vtbl[0x2c]
};
typedef void (CVisEntity::*VisNotifyFn)(void*);
struct CVisEntityVtbl {
    char s0[0x2c];
    VisNotifyFn Notify; // [0x2c]
};
inline void CVisEntity::Notify(void* held) {
    (this->*(vptr->Notify))(held);
}
struct CVisNode {
    CVisNode* m_next; // +0x00
    char p4[0x8 - 0x4];
    CVisEntity* m_8; // +0x08
};

// The 12 known "visible-notify" entity-type discriminators: the slot-4 method
// pointers (in retail, the ILT jump-thunks) of 12 entity classes. The entity's
// m_7c[4] is compared by ADDRESS against each, so they are modeled as functions and
// the compares lower to `cmp eax, OFFSET Fn` (DIR32). The thunk-vs-direct reloc
// naming is the scoring artifact; the compare bytes match retail.
extern "C" {
    void VisFn_40fe90();        // 0x40fe90
    void VisFn_4bf150();        // 0x4bf150
    void VisFn_423b40();        // 0x423b40
    void VisFn_Roll();          // 0x4cd70  (Roll)
    void VisFn_41e570();        // 0x41e570
    void VisFn_41e520();        // 0x41e520
    void VisFn_49b410();        // 0x49b410
    void VisFn_IntersectRect(); // 0x432060 (winapi_032060_IntersectRect)
    void VisFn_49b310();        // 0x49b310
    void VisFn_CBattlezDlg();   // 0x414b30 (CBattlezDlg)
    void VisFn_4fce80();        // 0x4fce80
}

RVA(0x000d9050, 0xc7)
i32 CPlay::NotifyVisibleEntities() {
    CView* v = m_c;
    i32* vp = (i32*)&v->m_drawSurface->m_viewport;
    CView::RenderState::SurfaceB* held = v->m_renderState->m_14;
    CVisNode* node = *(CVisNode**)((char*)v->m_rendererA + 0x14);

    RECT r;
    r.left = vp[0];
    r.top = vp[1];
    r.right = vp[2] + 1;
    r.bottom = vp[3] + 1;
    held->m_2c->NotifyClip(&r);

    while (node != 0) {
        CVisEntity* o = node->m_8;
        void* id = o->m_7c->m_10;
        if (id == (void*)VisFn_40fe90 || id == (void*)VisFn_4bf150 || id == (void*)VisFn_423b40
            || id == (void*)VisFn_Roll || id == (void*)VisFn_41e570 || id == (void*)VisFn_41e520
            || id == (void*)VisFn_40fe90 || id == (void*)VisFn_49b410
            || id == (void*)VisFn_IntersectRect || id == (void*)VisFn_49b310
            || id == (void*)VisFn_CBattlezDlg || id == (void*)VisFn_4fce80) {
            o->Notify(held);
        }
        node = node->m_next;
    }
    return 1;
}

// ===========================================================================
// CPlay::StepScroll - per-frame scroll-offset
// recompute. Reads the draw-surface (m_c->m_24) scroll origin (+0x10/+0x14) and
// its geom block (+0x5c -> +0x40.{x,y}), adds the BeginFrameClear extents
// (m_cursorX/m_cursorY), aligns each axis DOWN to a 0x20 boundary (+0x10 bias) and
// stores the result into the scroll-offset sink m_scrollSink (+0x5c X, +0x60 Y).
// ===========================================================================
// @early-stop
// register-coloring wall (~81.6%). Logic + all member offsets are byte-faithful;
// retail pins the draw-surface ptr in edx and geom in esi, our cl swaps them to
// esi/edx, which renames the temp regs and floats two scheduled adds by one slot.
// Not source-steerable (no local-pin/re-read variant flips the edx<->esi choice);
// confirmed coexists with ApplyGameOptions at top/absent/adjacent placement.
// See docs/patterns/zero-register-pinning.md.
RVA(0x000d1ac0, 0x4f)
void CPlay::StepScroll() {
    CDrawSurface* v = m_c->m_drawSurface;
    CDrawSurface::CameraGeom* geom = v->m_5c;

    i32 y = m_cursorY + (geom->m_originY - v->m_viewport.top);  // [edx+4]-m_14; +=m_cursorY
    i32 x = geom->m_originX + (m_cursorX - v->m_viewport.left); // [edx]; +=m_cursorX-m_10

    y = (y & ~0x1f) + 0x10; // align down 0x20 (and al,0xe0); + 0x10
    x = (x & ~0x1f) + 0x10; // align down 0x20 (and edi,~0x1f); + 0x10

    m_scrollSink->m_scrollX = x;
    m_scrollSink->m_scrollY = y;
}

// ===========================================================================
// CPlay::StepInputA - per-frame input poll.
// Two boot-time one-shot latches (m_inputWarmup1/m_inputWarmup2 fire on the first two frames),
// then a per-frame edge/axis probe over one of two mirrored half-blocks selected
// by m_inputHalfSel. Probes the draw-surface (m_c->m_4->m_14->m_2c); if absent returns 0.
// On a hit it dispatches the probed control. Returns 1.
// ===========================================================================
RVA(0x000d11e0, 0x9b)
i32 CPlay::StepInputA() {
    if (m_inputWarmup1 == 0) {
        m_inputWarmup1 = 1;
        return 1;
    }
    if (m_inputWarmup2 == 0) {
        m_inputWarmup2 = 1;
        return 1;
    }

    i32 axisVal;
    Edge* edge;
    void* halfPtr;
    if (m_inputHalfSel == 0) {
        axisVal = m_160;
        edge = (Edge*)&m_188;
        halfPtr = &m_168;
    } else {
        axisVal = m_164;
        edge = (Edge*)&m_198;
        halfPtr = &m_178;
    }

    // null-check the draw surface m_c->m_4->m_14->m_2c (walks through the this reg).
    void* probeTarget = m_c->m_renderState->m_14->m_2c;
    if (probeTarget == 0) {
        return 0;
    }

    i32 r = Eng_InputProbe(edge->m_0, edge->m_4, axisVal, halfPtr, 0x10);
    if (r != 0) {
        Eng_InputDispatch(0, 0, r);
    }
    return 1;
}

// ===========================================================================
// CPlay::LoadSBITextEdges (0x0d1710) - draw the status-bar text `name` clamped to
// the viewport rect inset by the "Font" Text{Left,Top,Right,Bottom}Edge margins,
// then arm a 2-frame step countdown. /GX EH frame (the CString local).
// ===========================================================================
// @early-stop
// ~97.8%: every instruction matches except retail reserves a 0x24 local frame vs
// our 0x18, which shifts the [esp+N] displacements by 0xc throughout. The ctor/
// assign/GetInt x4/SetRect/EngStr_DrawText sequence + the single `top` spill are
// byte-identical; the residual is MSVC's total-frame/EH-scratch sizing (tried rect-
// before-CString reorder, no change). Logic byte-faithful.
RVA(0x000d1710, 0x122)
void CPlay::LoadSBITextEdges(char* name) {
    CString s;
    s = name;

    RECT rect;
    RECT& vp = m_c->m_drawSurface->m_viewport;
    i32 l = vp.left, t = vp.top, r = vp.right, b = vp.bottom;
    i32 bottom = b - g_buteText->GetInt("Font", "TextBottomEdge");
    i32 right = r - g_buteText->GetInt("Font", "TextRightEdge");
    i32 top = t + g_buteText->GetInt("Font", "TextTopEdge");
    i32 left = l + g_buteText->GetInt("Font", "TextLeftEdge");
    SetRect(&rect, left, top, right, bottom);

    EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&s, (i32)&rect, 0x78, 1, 0xff, 0xff, 0, 1);
    m_stepCountdown = 2;
}

// ===========================================================================
// CPlay::PlayCueAt - the ambient/
// positional on-screen text-cue. Args: (cueId, a2, a3, a4, a5, a6, a7, rectSrc).
// De-dupes via the per-cue state object this+0x410 (skip if the same cueId is
// already showing AND its Probe says still-live). Builds the cue RECT from the
// font text-margins (CButeMgr GetInt "Font"/"Text{Left,Top,Right,Bottom}Edge")
// applied to either the caller's rect (rectSrc != 0) or the active viewport
// (this->m_c->m_24+0x10). a3 selects the Top vs Default cue renderer.
// ===========================================================================
RVA(0x000d1890, 0x1ba)
void CPlay::PlayCueAt(i32 cueId, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 rectSrc) {
    RECT rect;

    if (cueId != m_lastCueId) {
        if (((CCueState*)&m_cueText)->Probe(cueId) == 0) {
            return; // still-live other cue -> skip
        }
        m_lastCueId = cueId;
    }

    if (rectSrc != 0) {
        i32* src = (i32*)rectSrc;
        i32 bottom = src[3] - g_buteText->GetInt("Font", "TextBottomEdge");
        i32 right = src[2] - g_buteText->GetInt("Font", "TextRightEdge");
        i32 top = src[1] + g_buteText->GetInt("Font", "TextTopEdge");
        i32 left = src[0] + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    } else {
        // the viewport rect (m_c->m_drawSurface->m_viewport) ptr (edx) does not survive
        // the GetInt calls, so all 4 corners are read up front.
        RECT& vp = m_c->m_drawSurface->m_viewport;
        i32 l = vp.left, t = vp.top, r = vp.right, b = vp.bottom;
        i32 bottom = b - g_buteText->GetInt("Font", "TextBottomEdge");
        i32 right = r - g_buteText->GetInt("Font", "TextRightEdge");
        i32 top = t + g_buteText->GetInt("Font", "TextTopEdge");
        i32 left = l + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    if (a3 != 0) {
        Eng_CueRenderTop(m_c, &m_cueText, &rect, a2, 1, a4, a5, a6, a7);
    } else {
        Eng_CueRenderDef(m_c, &m_cueText, &rect, a2, 1, a4, a5, a6, a7);
    }
}

// ===========================================================================
// CPlay::DrawWorldFrame (0x0c9c20) - one in-game world-draw frame: the begin
// virtual (this->vtbl[+0x98]), the m_c->m_24->m_5c sub-step (if present), mirror
// the draw clock, present (m_c->m_8->vtbl[+0x24]), the m_4->m_68 frame-timer step,
// the optional mode-3 reg cue, then the m_guts guts step.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall — structure byte-identical, only temp-register naming
// (eax vs edx in the pointer-chain derefs) + one m_4/global load-order in the
// Step-call setup differ; see docs/patterns/zero-register-pinning.md.
RVA(0x000c9c20, 0x79)
void CPlay::DrawWorldFrame() {
    Vslot26(); // this->vtbl[+0x98]()  (begin-frame virtual, thiscall)
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawA();
    }
    g_6bf3c0 = g_645580;
    g_6bf3bc = g_645584;
    m_c->m_rendererA->BeginScene(0);   // m_c->m_8->vtbl[+0x24](0)
    m_4w()->m_68->Step((i32)g_645584); // m_4->m_68 frame-timer step
    if (g_64556c->m_134 == 3) {
        g_64556c->PerFrameCue();
    }
    m_guts->Step((i32)g_645584); // m_guts guts step
}

// ===========================================================================
// CPlay::DrawWorldFrames (0x0c9cc0) - the fixed-step world catch-up loop: divide
// the frame delta by 0x12 (the fixed sub-step), then run that many world-draw
// frames (clamping the first/last sub-step to 0x12), finishing with one tail
// frame-timer step over the full delta.
// ===========================================================================
// @early-stop
// regalloc wall — full control flow + the fixed-substep loop + 3-arg StepFull
// calls byte-structure-identical; MSVC colors now/accum into ebx/ebp swapped vs
// retail and the prologue spill schedule differs. See docs/patterns/zero-register-pinning.md.
RVA(0x000c9cc0, 0x12e)
i32 CPlay::DrawWorldFrames() {
    i32 delta = (i32)g_645584;
    i32 steps = (i32)((u32)delta / FIXED_SUBSTEP_MS); // 0x38e38e39 magic-div by 18
    i32 now = (i32)g_645580;
    i32 accum = (i32)g_645588;
    i32 rem = delta - steps * FIXED_SUBSTEP_MS;
    i32 saveDelta = delta; // [esp+0x1c]
    i32 saveAccum = accum; // [esp+0x20]
    i32 saveNow = now;     // [esp+0x24]
    if (rem != 0) {
        steps = steps + 1;
    }
    now -= delta;
    accum -= delta;
    if (steps > 0) {
        i32 last = steps - 1;
        i32 i = 0;
        do {
            i32 dt = (i == last && rem != 0) ? rem : FIXED_SUBSTEP_MS;
            accum += dt;
            now += dt;
            m_4w()->m_68->StepFull(now, dt, accum);
            if (i > 0 && i < last) {
                if (m_c->m_drawSurface->m_5c != 0) {
                    m_c->m_drawSurface->m_5c->DrawB();
                }
            }
            Vslot26(); // this->vtbl[+0x98]()
            if (m_c->m_drawSurface->m_5c != 0) {
                m_c->m_drawSurface->m_5c->DrawA();
            }
            m_c->m_rendererA->BeginScene(0);
            m_4w()->m_68->Step((i32)g_645584);
            if (g_64556c->m_134 == 3) {
                g_64556c->PerFrameCue();
            }
            m_guts->Step((i32)g_645584);
            i++;
        } while (i < steps);
    }
    m_4w()->m_68->StepFull(saveNow, saveDelta, saveAccum); // tail step
    return steps;
}

// ===========================================================================
// The dev frame profiler (CPlay::ProfileDeltaFrame / ProfileInputFrame).
// Instrumented variants of the world-draw + present that bracket each phase with
// the cached timeGetTime fn-ptr (g_pTimeGetTime, pinned in a callee-saved reg)
// and emit a per-phase timing line through the variadic logger ProfLog into the
// shared text sink g_profSink.
// ===========================================================================
extern "C" {
    DATA(0x002c4650)
    extern u32(WINAPI* g_pTimeGetTime)(); // PTR_timeGetTime_006c4650
    // The profiler line sink (a global text buffer; its ADDRESS is the logger arg).
    DATA(0x00245524)
    extern i32 g_profSink; // DAT_00645524
    // The variadic profiler logger (cdecl). 0x1b2cf5.
    void ProfLog(void* sink, const char* fmt, ...);
}
// The two timing accumulators ProfileInputFrame folds the back-half phases into.
extern "C" {}

// The draw-surface flush sink (m_c->m_renderState->m_10->m_2c) torn through a thiscall flush.
struct CProfFlush {
    void Flush(i32 z); // 0x13e850 (thiscall)
};

// ===========================================================================
// CPlay::ProfileDeltaFrame (0x0ca0a0) - the simple profiled frame: run the
// frame-rate split (RenderFast/Slow), world-blit, push-view + present, then log
// "Delta/Update/Draw/NumUpdates", flush, and run the final camera draw-B.
// ===========================================================================
RVA(0x000ca0a0, 0x101)
i32 CPlay::ProfileDeltaFrame() {
    u32(WINAPI * tg)() = g_pTimeGetTime;
    i32 updates = 0;
    u32 t0 = tg();
    u32 d = g_645584;
    if (d > 0x12 && d < 0xc8) {
        updates = RenderFast();
    } else {
        RenderSlow();
    }
    i32 renderMs = (i32)(tg() - t0);
    m_4w()->m_54->Blit(m_c->m_drawSurface->m_5c->m_84, m_c->m_drawSurface->m_5c->m_88);
    u32 t2 = tg();
    m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
    m_c->m_rendererB->Present(m_c->m_renderState->m_14, m_c->m_renderState->m_18);
    i32 presentMs = (i32)(tg() - t2);
    ProfLog(
        &g_profSink,
        "Delta=%i, Update=%i, Draw=%i, NumUpdates=%i    ",
        (i32)g_645584,
        renderMs,
        presentMs,
        updates
    );
    ProfFlushTail();
    ((CProfFlush*)m_c->m_renderState->m_10->m_2c)->Flush(0);
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawB();
    }
    return 1;
}

// The profiled-frame report tail (cdecl free fn, 3 args): the manager singleton,
// the guts subsystem and the region-0 gate. 0xebd70. reloc-masked.
extern "C" void ProfReport(void* mgr, void* guts, i32 gate);

// ===========================================================================
// CPlay::ProfileInputFrame (0x0c9e40) - the fully-instrumented frame: nine
// timeGetTime-bracketed phases (input/activate/deact/update/hit-test/draw/fixed/
// status-bar) logged in one "Input=.." line, then the flush + camera draw-B whose
// times are stashed in the cross-frame accumulators (g_profAccA/g_profAccB read
// at log time = the PREVIOUS frame's flush/draw-B). __thiscall, ret 0.
// ===========================================================================
// @early-stop
// profiler-scheduling wall: the body is the complete, correct reconstruction (the
// nine phase brackets in order, the BeginScene(1)/m_68->Step/m_guts->Step update
// block, the PushView/Present draw block, the m_guts status-bar tick, the 11-arg
// ProfLog with the cross-frame g_profAccA/g_profAccB accumulators, then the timed
// flush + draw-B writing those globals for next frame, and the ProfReport tail).
// MSVC pins the g_pTimeGetTime fn-ptr in esi across all 14 calls as retail does,
// but the seven live phase-times spill to a different set of stack slots / the
// ebx/ebp coloring of deact/update differs, so the slot-reuse schedule diverges
// despite identical logic. Same idiom as ProfileDeltaFrame (byte-exact); the extra
// phases push it onto the documented register/stack-scheduling plateau. Deferred
// to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x000c9e40, 0x1d7)
i32 CPlay::ProfileInputFrame() {
    m_4w()->m_54->Blit(m_c->m_drawSurface->m_5c->m_84, m_c->m_drawSurface->m_5c->m_88); // untimed
    u32(WINAPI * tg)() = g_pTimeGetTime;

    u32 t1 = tg();
    Vslot26(); // this->vtbl[+0x98]
    i32 activateMs = (i32)(tg() - t1);

    u32 t3 = tg();
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawA();
    }
    i32 deactMs = (i32)(tg() - t3);

    u32 t5 = tg();
    m_c->m_rendererA->BeginScene(1);
    m_4w()->m_68->Step((i32)g_645584);
    m_guts->Step((i32)g_645584);
    i32 updateMs = (i32)(tg() - t5);

    u32 t7 = tg();
    i32 hitTestMs = (i32)(tg() - t7);

    u32 t9 = tg();
    m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
    i32 drawMs = (i32)(tg() - t9);

    u32 t11 = tg();
    m_c->m_rendererB->Present(m_c->m_renderState->m_14, m_c->m_renderState->m_18);
    i32 fixedMs = (i32)(tg() - t11);

    u32 t13 = tg();
    m_guts->StatusBarTick(); // 0xfe6b0
    i32 statusBarMs = (i32)(tg() - t13);

    ProfLog(
        &g_profSink,
        "Input=%i, Activate=%i, Deact=%i, Update=%i, HitTest=%i, Draw=%i, Fixed=%i, "
        "StatusBar=%i, Flip=%i  ",
        activateMs,
        deactMs,
        g_profAccA,
        updateMs,
        hitTestMs,
        drawMs,
        fixedMs,
        statusBarMs,
        g_profAccB
    );

    ProfFlushTail();
    g_profAccB = (i32)tg();
    ((CProfFlush*)m_c->m_renderState->m_10->m_2c)->Flush(0);
    g_profAccB = (i32)(tg() - (u32)g_profAccB);
    g_profAccA = (i32)tg();
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawB();
    }
    g_profAccA = (i32)(tg() - (u32)g_profAccA);
    ProfReport(g_64556c, m_guts, m_region0Gate);
    return 1;
}

// ===========================================================================
// CPlay::ResetGoals (0x0d5f00) - clear the world goal object's pending bit
// (m_4->m_68->m_23c, OR 0x10000 into +0x8), reset the substep gate (m_68->m_230),
// then recompute the plane geom (m_4->m_30->m_24->m_5c) from the (x,y) args:
// store them as floats, scaling by the geom's +0x18/+0x1c factors unless bit0 of
// +0x8 is set, then call RecomputePlaneCoords.
// ===========================================================================
RVA(0x000d5f00, 0x69)
i32 CPlay::ResetGoals(i32 x, i32 y) {
    CWorld* w = m_4w();
    CWorld::WorldTimeline* g = w->m_68;
    if (g->m_23c != 0) {
        g->m_23c->m_flags |= 0x10000;
        g->m_23c = 0;
    }
    g->m_230 = 0;
    CPlayPlaneGeom* pg = m_4w()->m_30->m_24->m_5c;
    if ((pg->m_8 & 1) == 0) {
        pg->m_10 = (float)x * pg->m_18;
        pg->m_14 = (float)y * pg->m_1c;
    } else {
        pg->m_10 = (float)x;
        pg->m_14 = (float)y;
    }
    pg->Recompute();
    return 1;
}

// ===========================================================================
// CPlay::RegisterInputBindings (0x0d9160) - register the nine keyboard control
// bindings (the WM_KEYDOWN-style codes 0x100-0x102 / 0x200-0x206, each with flag
// 0x40) on the world input dispatcher (m_4->m_4).
// ===========================================================================
RVA(0x000d9160, 0xac)
i32 CPlay::RegisterInputBindings() {
    m_4w()->m_4->Bind(0x102, 0x40);
    m_4w()->m_4->Bind(0x100, 0x40);
    m_4w()->m_4->Bind(0x200, 0x40);
    m_4w()->m_4->Bind(0x201, 0x40);
    m_4w()->m_4->Bind(0x202, 0x40);
    m_4w()->m_4->Bind(0x203, 0x40);
    m_4w()->m_4->Bind(0x204, 0x40);
    m_4w()->m_4->Bind(0x205, 0x40);
    m_4w()->m_4->Bind(0x206, 0x40);
    return 1;
}

// CPlay::ArmSnapshot (0x0d9240) - thiscall(active, dur). When `active`, latch the
// snapshot duration (dur) and base clock (g_645588) 64-bit timers; always store
// `active` into m_snapshotActive. Migrated from engine_boundary (CPlay).
// @early-stop
// scheduling wall (99.2%): logic + regalloc byte-exact except the two independent
// 64-bit-base stores (m_snapBaseLo/m_snapBaseHi) emit in hi,lo order where retail
// emits lo,hi; cl fills the g_645588 load-use latency gap with the hi=0 store.
// Loading the clock into a local forces lo,hi but diverges the whole regalloc
// (push edi / immediates) to 62% — not source-steerable.
RVA(0x000d9240, 0x3c)
i32 CPlay::ArmSnapshot(i32 active, i32 dur) {
    if (active != 0) {
        m_snapDur = dur;
        m_snapDurHi = 0;
        m_snapBaseLo = g_645588;
        m_snapBaseHi = 0;
    }
    m_snapshotActive = active;
    return 1;
}

// CPlay::CanQuickSave (0x0da3b0) - all-idle predicate: returns 1 only when the
// render is enabled, not in a main frame, no overlay-drag, no active snapshot, the
// guts subsystem is idle (m_548/m_busyA/m_busyB all 0), the registry has no active
// selection (reg->m_c), and the cue-sink B busy gate is set. Migrated from
// engine_boundary (CPlay).
RVA(0x000da3b0, 0x6e)
i32 CPlay::CanQuickSave() {
    if (m_renderDisabled == 0 && m_inGame == 0 && m_overlayDrag == 0 && m_snapshotActive == 0
        && m_guts->m_548 == 0 && m_guts->m_busyA == 0 && m_guts->m_busyB == 0
        && g_64556c->m_frameGate == 0 && ((CRegSub68*)g_64556c->m_68)->m_400 != 0) {
        return 1;
    }
    return 0;
}

// CPlay::PostHudRect (0x0da440) - if the world is ready, post the HUD/selection
// rect (by value, with the dev-state 0x20 flag) to the world timeline, then clear
// the ready / drag-snap gates. Migrated from engine_boundary (CPlay).
RVA(0x000da440, 0x60)
i32 CPlay::PostHudRect() {
    if (m_worldReady != 0) {
        m_4w()->m_68->HudRect(m_hudRect, g_645578->m_18 & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    return 1;
}

// ===========================================================================
// CPlay::StepGridWalk (0x0d0a60) - the per-frame frame-grid advance. If the walk
// is inactive (m_gridWalkActive==0) bail. Decrement the delay countdown (m_gridDelayCount) by dt; while
// it is still positive just return. When it expires reload it from m_gridDelayBase, advance
// the row index (m_gridRow), look up that row's frame in the grid table (clamped to
// [m_64,m_68]); if the lookup is empty wrap back to the first row.
// ===========================================================================
// @early-stop
// regalloc/save-scheduling wall — logic + control flow identical; retail defers the
// `push esi`/`pop esi` past the two early returns (into the m_gridDelayCount>dt block) and colors
// idx/grid into edx/eax swapped vs MSVC here. See docs/patterns/zero-register-pinning.md.
RVA(0x000d0a60, 0x92)
i32 CPlay::StepGridWalk(i32 dt) {
    if (m_gridWalkActive == 0) {
        return 1;
    }
    if ((u32)m_gridDelayCount > (u32)dt) {
        m_gridDelayCount = m_gridDelayCount - dt;
        return 1;
    }
    m_gridDelayCount = m_gridDelayBase;
    m_gridRow = m_gridRow + 1;
    i32 idx = m_gridRow;
    CFrameGrid* g = m_grid;
    i32 frame;
    if (idx >= g->m_firstRow && idx <= g->m_lastRow) {
        frame = g->m_rowTable[idx];
    } else {
        frame = 0;
    }
    m_gridCurFrame = frame;
    if (frame == 0) {
        m_gridCurFrame = g->m_rowTable[g->m_firstRow];
        m_gridRow = g->m_firstRow;
    }
    return 1;
}

// ===========================================================================
// CPlay::DispatchHudClick (0x0ce530) - route a HUD pointer event at (x,y). If the
// HUD is suppressed (m_hudSuppressed) just succeed. If an overlay is up (m_overlayActive) and the guts
// subsystem isn't busy (m_guts->m_state!=2 && m_guts->m_mode!=5), forward to the in-rect
// handler. If a pending HUD rect is armed (m_worldReady), post it (gated by the dev-flag
// 0x645578->m_18 & 0x20) and clear m_worldReady/m_dragSnapActive. Finally, unless the guts subsystem
// is busy (==2), point-test against the viewport box (m_c->m_24+0x10): inside ->
// succeed, outside -> forward to the click-at-point handler.
// ===========================================================================
// @early-stop
// zero-register-pinning wall — structure + offsets byte-identical; retail pins ebx=0
// (xor ebx,ebx + cmp ebx,[member] null tests) and colors y into ebp (5 callee-saves),
// MSVC here uses test/4 saves. No source lever forces it. docs/patterns/zero-register-pinning.md.
RVA(0x000ce530, 0xe3)
i32 CPlay::DispatchHudClick(i32 a, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_overlayActive != 0 && m_guts->m_state != 2 && m_guts->m_mode != 5) {
        HudClickInRect(a, x, y);
    }
    if (m_worldReady != 0) {
        m_4w()->m_68->HudRect(m_hudRect, g_645578->m_18 & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    if (m_guts->m_state == 2) {
        return 1;
    }
    RECT& vp = m_c->m_drawSurface->m_viewport;
    if (x >= vp.left && x <= vp.right && y >= vp.top && y <= vp.bottom) {
        return 1;
    }
    m_guts->HudClickAt(a, x, y);
    return 1;
}

// ===========================================================================
// CPlay::BeginGridWalk (0x0d0920) - arm the frame-grid walk for a level cue. Look
// up the grid object for `key` in the view's grid map (m_c->m_10->m_10); bail if
// absent. If `hasGrid`, load the grid's frame sprite from the world sprite factory
// (m_4->m_74, retrying via the registry's factory) using the per-type descriptor
// from the world config array (m_4+0x158, indexed by g_644c54), then push it into
// the grid (SetDelay 0xa / SetSprite). Finally seed the current row (m_gridCurFrame) from
// `index` (clamped to [m_64,m_68]) and, if non-empty, latch the e8/delay state.
// ===========================================================================
// @early-stop
// arg-push-scheduling wall — control flow + the config-array index + map Lookup +
// grid-setup byte-identical; the LoadSprite retry pushes (prev,1) in the opposite
// order and the g_64556c reload sits one slot off. docs/patterns/statement-schedule-faithful.md.
RVA(0x000d0920, 0xfe)
i32 CPlay::BeginGridWalk(i32 key, i32 index, i32 e8, i32 delay, i32 hasGrid) {
    if (m_c == 0) {
        return 1;
    }
    void* grid = 0;
    m_c->m_imageRegistry->m_10.Lookup(key, grid);
    m_grid = (CFrameGrid*)grid;
    if (grid == 0) {
        return 1;
    }
    m_gridHasSprite = hasGrid;
    if (hasGrid != 0) {
        CWorld* w = m_4w();
        i32 id = g_644c54;
        void* spr = w->m_74->LoadSprite(*(void**)(w->m_158 + (id * 0x47) * 8), 0);
        if (spr == 0) {
            spr = ((CWorld::SpriteLoader*)g_64556c->m_74)->LoadSprite(spr, 1);
        }
        m_grid->SetDelay(0xa);
        m_grid->SetSprite(spr);
    }
    CFrameGrid* g = m_grid;
    i32 frame;
    if (index >= g->m_firstRow && index <= g->m_lastRow) {
        frame = g->m_rowTable[index];
    } else {
        frame = 0;
    }
    m_gridCurFrame = frame;
    if (frame != 0) {
        m_gridRow = index;
        m_gridWalkActive = e8;
        m_gridDelayBase = delay;
        m_gridDelayCount = delay;
    }
    return 1;
}

// ===========================================================================
// CPlay::HandleDragMove (0x0d0db0) - the per-frame drag/box-select update at
// pointer (x,y) with selector `a`. Bails while the primary mode (m_inGame) or the
// paused flag (m_paused) is set. If an overlay is up (m_overlayActive) and the guts subsystem
// isn't busy, forwards to the in-rect HUD drag. Then a 3-way:
//   m_dragSnapActive  -> a drag is being snapped: snap to (m_snapOriginX+x, m_snapOriginY+y) and re-arm m_scrollSink.
//   m_overlayDrag  -> an overlay drag: dispatch m_guts->DragSelect and return.
//   else   -> the world box-drag: point-test against the viewport box
//             (m_c->m_24+0x10). INSIDE -> finish (clear m_dragInProgress, normalize the sel
//             rect, re-arm m_scrollSink); OUTSIDE -> continue the drag (post to m_hitTest's
//             hit-test, else either WorldPost the world-space delta or DragSelect
//             + clamp the sel rect into the box, then the m_dragEndNotify end-notify).
// ===========================================================================
// @early-stop
// block-placement + min/max-clamp regalloc wall (~35%). Logic, control flow,
// member offsets, the full call set, the box point-test and BOTH drag-rect clamp
// ladders are byte-faithful; the RECT-local copy already forced the matching
// `sub esp,0x10` frame and the goto folds the two snap/inside tails into retail's
// shared Lf1a. The residual is non-source-steerable codegen choice: (1) retail
// hoists the cold OUTSIDE-of-box clamp block to the function end (forward-jumped)
// while MSVC here lays it inline after the box-test; (2) the m_overlayActive/m_dragSnapActive probes
// color into ecx vs eax; (3) the four min/max ternaries pick the eax/ecx operand
// for the clamp the opposite way. The block-float matches the family in
// docs/patterns/nested-if-success-deepest-error-tail.md; the regalloc residual is
// docs/patterns/zero-register-pinning.md.
RVA(0x000d0db0, 0x347)
i32 CPlay::HandleDragMove(i32 a, i32 x, i32 y) {
    // box corners declared (uninitialized) up front so the `goto rearm` tail
    // doesn't cross their initialization (MSVC C2362); they're filled only on
    // the box-drag path below and unused at the rearm label.
    i32 left, top, right, bottom;
    if (m_inGame != 0) {
        return 1;
    }
    if (m_paused != 0) {
        return 1;
    }
    if (m_overlayActive != 0 && m_guts->m_state != 2 && m_guts->m_mode != 5) {
        DragHudInRect(a, x, y);
    }

    if (m_dragSnapActive != 0) {
        if (m_guts == 0) {
            return 1;
        }
        DragSnapTo(m_snapOriginX + x, m_snapOriginY + y);
        goto rearm; // -> shared m_scrollSink re-arm tail
    }

    if (m_overlayDrag != 0) {
        m_guts->DragSelect(a, x, y);
        return 1;
    }

    // --- the world box-drag: point-test (x,y) against the viewport box ---
    // (box copied to a 0x10 RECT local so the clamp ladders re-read top/right/
    //  bottom from [esp+0x14/0x18/0x1c] across the DragSelect call. The INSIDE
    //  path is the fall-through "success" so the OUTSIDE block floats to the
    //  tail; see docs/patterns/nested-if-success-deepest-error-tail.md.)
    RECT box = m_c->m_drawSurface->m_viewport;
    left = box.left;
    top = box.top;
    right = box.right;
    bottom = box.bottom;
    if (x >= left && x <= right && y >= top && y <= bottom) {
        // INSIDE the box -> finish the drag.
        if (m_dragInProgress != 0) {
            m_guts->DragClear(-1);
        }
        m_dragInProgress = 0;
        if (m_worldReady != 0) {
            // normalize {m_cursorX,m_cursorY}..{m_dragClampMaxX,m_dragClampMaxY} into min/max:
            m_hudRect.left = m_cursorX < m_dragClampMaxX ? m_cursorX : m_dragClampMaxX;
            m_hudRect.right = m_cursorX > m_dragClampMaxX ? m_cursorX : m_dragClampMaxX;
            m_hudRect.top = m_cursorY < m_dragClampMaxY ? m_cursorY : m_dragClampMaxY;
            m_hudRect.bottom = m_cursorY > m_dragClampMaxY ? m_cursorY : m_dragClampMaxY;
            goto rearm; // -> shared m_scrollSink re-arm tail
        }

        // m_worldReady == 0: the hit-test / world-post branch.
        if (m_hitTest->HitTest(x, y) != 0 || m_4w()->m_c != 0 || m_inGame != 0
            || m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
            // (a second, distinct re-arm landing pad in retail.)
            ScrollSink* s2 = m_scrollSink;
            if (s2 == 0) {
                return 1;
            }
            s2->m_flags |= 1;
            return 1;
        }
        if (m_levelId != 0) {
            if (m_scrollSink != 0) {
                m_scrollSink->m_flags |= 1;
            }
        } else {
            if (m_scrollSink != 0) {
                m_scrollSink->m_flags &= ~1;
            }
        }
        CDrawSurface* v = m_c->m_drawSurface;
        i32 wx = v->m_5c->m_originX - v->m_viewport.left + x;
        i32 wy = v->m_5c->m_originY - v->m_viewport.top + y;
        m_4w()->m_68->WorldPost(wx, wy);
        return 1;
    }

    // OUTSIDE the box -> continue dragging (the cold block, floated to the tail).
    if (m_scrollSink != 0) {
        m_scrollSink->m_flags |= 1;
    }
    m_dragInProgress = 1;
    m_guts->DragSelect(a, x, y);
    if (m_worldReady != 0) {
        // clamp the selection rect into [box, m_dragClampMaxX/m_dragClampMaxY]:
        i32 lo = m_cursorX > left ? m_cursorX : left;
        m_hudRect.left = lo;
        if (lo >= m_dragClampMaxX) {
            m_hudRect.left = m_dragClampMaxX;
        }
        i32 hi = m_cursorX < right ? m_cursorX : right;
        m_hudRect.right = hi;
        if (hi > m_dragClampMaxX) {
            m_hudRect.right = m_dragClampMaxX;
        }
        i32 tlo = m_cursorY <= top ? m_cursorY : top;
        m_hudRect.top = tlo;
        if (tlo < m_dragClampMaxY) {
            m_hudRect.top = m_dragClampMaxY;
        }
        i32 thi = m_cursorY < bottom ? m_cursorY : bottom;
        m_hudRect.bottom = thi;
        if (thi > m_dragClampMaxY) {
            m_hudRect.bottom = m_dragClampMaxY;
        }
    }
    if (m_dragEndNotify != 0 && m_4w()->m_68->m_2a8 == 0) {
        EndDragSel();
    }
    return 1;

rearm:
    ScrollSink* s = m_scrollSink;
    if (s == 0) {
        return 1;
    }
    s->m_flags |= 1;
    return 1;
}

// ===========================================================================
// CPlay::BuildHelpReveal (0x0d72c0) - the per-frame help-overlay "wipe" animation.
// Bails if there is no view (m_c->m_4->m_14). On the first frame (m_revealFrame==1) draws
// the two static end-cap strips. Each frame computes the wipe column from the
// frame counter (counter * 3.7857, truncated), then either snaps the single cap to
// the right edge (counter>=0x37) or sweeps a column of strips (0xe0 - i*3.7857) for
// i in [counter, 0x37). Always draws the trailing cap and advances the counter.
// ===========================================================================
// @early-stop
// control-flow/float-schedule wall — prologue + the two cap strips + the per-strip
// HudStrip pushes + the __ftol column math are byte-faithful; the counter>=0x37 cap
// branch merges into the trailing-cap tail via a shared landing pad the C if/else
// can't reproduce 1:1, and the x87 fmul/fild ordering diverges. ~68%.
RVA(0x000d72c0, 0x128)
i32 CPlay::BuildHelpReveal() {
    void* view = m_c->m_renderState->m_14;
    if (view == 0) {
        return 0;
    }
    if (m_revealFrame == 1) {
        Eng_HudStrip(view, m_revealCapStart, 0x140, 0x1a6, 1, 0);
        Eng_HudStrip(m_c, m_revealCapMid, 0xe0, 0x1a6, 1, 0);
    }

    i32 counter = m_revealFrame;
    i32 col = (i32)((float)counter * 3.7857143878936768f);
    if (counter < 0x37) {
        i32 i = counter;
        do {
            i32 x = 0xe0 - (i32)((float)i * -3.7857143878936768f);
            Eng_HudStrip(m_c, m_revealCapMid, x, 0x1a6, 1, 0);
            i++;
        } while (i < 0x37);
    } else {
        Eng_HudStrip(m_c, m_revealCapMid, col + 0xe0, 0x1a6, 1, 0);
    }

    Eng_HudStrip(m_c, m_revealCapEnd, 0x1b4, 0x1a6, 1, 0);
    m_revealFrame = m_revealFrame + 1;
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x0008c9d0, 0x2bd)
void CPlay::Stub_08c9d0() {}

// =========================================================================
// CState::SetBeginClearParams
// Stores the two BeginFrameClear arguments and returns 1.
//
RVA(0x0008c970, 0x1c)
i32 CState::SetBeginClearParams(i32 unused, i32 arg2, i32 arg3) {
    m_cursorX = arg2;
    m_cursorY = arg3;
    return 1;
}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x000cdb10, 0x80c)
i32 CPlay::winapi_0cdb10_PostMessageA(i32, i32, i32) {
    return 0;
}

// ===========================================================================
// CPlay::HandleTileClick (0x0ceae0) - the menu/pause-state pointer-click handler,
// the mouse-input twin of OnKeyCommand: same hudSuppressed / renderDisabled(resume)
// / inGame(reset-or-report) / paused(unpause+PostMessage) priority chain, then the
// no-active-grunt overlay path: probe the overlay object, early-out on the HUD hit
// rect, run the guts HUD hit-test, else (inside the world rect) snap the click to
// the tile grid and place/cancel the world marker. Returns 1 in every path.
// ===========================================================================
// A tiny view of the overlay object cached at CPlay::m_overlayActive (+0x320): it is
// a pointer (Render null-checks it as a gate), whose click probe consumes the event.
SIZE_UNKNOWN(COverlayClick);
struct COverlayClick {
    i32 Probe(i32 a, i32 x, i32 y); // reloc-masked thiscall
};
// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md): the whole
// priority chain, the overlay probe, both HUD/world rect hit-tests, the grid-snap
// math + PlaceMarker/CancelMarker tail are all byte-faithful (the grid math +
// 7-arg PlaceMarker push match exactly). Residual: MSVC assigns the x-coord to
// edi and y to ebx where retail pins x->ebx / y->edi (mirror pair) and defers the
// y-load to the guts-rect site; because the compare ORDER is byte-matched I cannot
// reorder to flip the pair without breaking the matched guts rect. The swap also
// spares retail's rect-field spill, so my frame drops sub esp,0x10 (the 3 arg-load
// displacements + 7 epilogue add esp shift). Pure allocator choice, no source
// lever. ~81%.
RVA(0x000ceae0, 0x268)
i32 CPlay::HandleTileClick(i32 a, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_hudSuppressed = 1;
        m_renderDisabled = 0;
        EnterMode(3);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            return 1;
        }
        m_4w()->ReportError(0x800a, 0x458);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_4w()->m_4->m_4->m_4, 0x111, 0x816e, 0);
        return 1;
    }
    if (m_overlayDrag != 0) {
        return 1;
    }
    if (((CRegSub68*)g_64556c->m_68)->m_400 == 0) { // reg->m_68 cue-sink busy gate
        return 1;
    }
    if (m_4w()->m_c != 0) {
        return 1;
    }
    if (m_overlayActive != 0 && m_guts->m_state != 2 && m_guts->m_mode != 5) {
        if (((COverlayClick*)(void*)m_overlayActive)->Probe(a, x, y)) {
            return 1;
        }
    }
    if (x < m_guts->m_rect10.right && x >= m_guts->m_rect10.left && y < m_guts->m_rect10.bottom
        && y >= m_guts->m_rect10.top) {
        return 1;
    }
    i32 idx = m_guts->HitTest3ad5(x, y);
    if (idx != -1) {
        m_guts->Apply3ebd(idx);
        CWorld::WorldTimeline* w = m_4w()->m_68;
        if (w->m_23c != 0) {
            w->m_23c->m_flags |= 0x10000;
            w->m_23c = 0;
        }
        w->m_230 = 0;
        return 1;
    }
    if (m_4w()->m_68->m_24c == 0) {
        return 1;
    }
    CWorld::RenderStateHolder::PlaneGeomHolder* ph = m_4w()->m_30->m_24;
    if (x < ph->m_rect10.right && x >= ph->m_rect10.left && y < ph->m_rect10.bottom
        && y >= ph->m_rect10.top) {
        CDrawSurface* ds = m_c->m_drawSurface;
        CDrawSurface::CameraGeom* geom = ds->m_5c;
        i32 rawX = geom->m_originX - ds->m_viewport.left + x;
        i32 rawY = geom->m_originY - ds->m_viewport.top + y;
        i32 snapX = (rawX & ~0x1f) + 0x10;
        i32 snapY = (rawY & ~0x1f) + 0x10;
        m_tileClickX = snapX;
        m_tileClickY = snapY;
        CWorld::WorldTimeline* w = m_4w()->m_68;
        if (w->m_25c != 0 && w->m_25c->m_2c != 0) {
            w->CancelMarker();
            return 1;
        }
        w->PlaceMarker(snapX, snapY, rawX, rawY, 1, 0, 1);
    }
    return 1;
}

// @confidence: low
// @source: winapi:CopyRect
// @stub
RVA(0x000d0b30, 0x200)
i32 CPlay::winapi_0d0b30_CopyRect(i32) {
    return 0;
}

// ===========================================================================
// DrawWorldPresent (0x0cefc0) - a present-only world frame: run the two camera
// sub-steps (DrawB then DrawA), guarded on the camera-geom ptr, twice - each
// pair preceded by a renderer begin-scene(1) - then push the view, present, and
// tick the manager. Migrated from engine_boundary (CPlay: the m_c draw chain +
// m_4 manager). All draw callees are out-of-line / reloc-masked.
// @early-stop
// ~99%: code structure byte-exact; residual is (a) a 1-2 byte regalloc nit in
// the first m_c->m_24->m_5c chain load (cl spreads to ecx where retail reuses
// eax) and (b) the reloc-masked callee symbols (CameraGeom DrawA/DrawB, PushView,
// ManagerTick) which pair once those engine fns are named.
RVA(0x000cefc0, 0xa2)
i32 CPlay::DrawWorldPresent() {
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawB();
    }
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawA();
    }
    m_c->m_rendererA->BeginScene(1);
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawB();
    }
    if (m_c->m_drawSurface->m_5c != 0) {
        m_c->m_drawSurface->m_5c->DrawA();
    }
    m_c->m_rendererA->BeginScene(1);
    m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
    m_c->m_rendererB->Present(m_c->m_renderState->m_14, m_c->m_renderState->m_18);
    m_4w()->ManagerTick();
    return 1;
}

// ===========================================================================
// PresentAndFlush (0x0cba10) - the overlay-frame present path: bail unless the
// state is active (Vfunc3), restore the saved display mode if it drifted, then
// either notify-visible (region-1 gate) or push+present, and flush the draw
// surface. Migrated from engine_boundary (CPlay).
// @early-stop
// reloc-masked plateau (~97%): code bytes exact; residual is the call-rel32
// operands to the unmatched engine callees (m_guts ClampApply 0x500cb0, the
// PushView 0x15dc90, surface flush 0x13e850, m_4 RestoreVideoMode 0x8df00).
RVA(0x000cba10, 0xb0)
i32 CPlay::PresentAndFlush() {
    if (Vfunc3() == 0) {
        return 0;
    }
    CWorld* w = m_4w();
    i32 savedW = w->m_94;
    i32 liveW = w->m_8c;
    i32 savedH = w->m_98;
    i32 liveH = w->m_90;
    if (savedW != liveW || savedH != liveH) {
        if (w->RestoreVideoMode(savedW, savedH, 1) == 0) {
            return 0;
        }
    }
    if (m_guts != 0) {
        m_guts->ClampApply();
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_c->m_drawSurface->PushView(m_c->m_renderState->m_14, m_c->m_rendererA);
            m_c->m_rendererB->Present(m_c->m_renderState->m_14, m_c->m_renderState->m_18);
        }
        Eng_SurfaceFlush(m_c->m_renderState->m_10->m_2c, 0);
    }
    return 1;
}

// ===========================================================================
// CPlay::EnterOverlayDrag (0x0d6440) - arm the win/lose-overlay drag mode. If
// already armed (m_overlayDrag) bail. Otherwise latch m_overlayDrag, clear the
// world-ready / drag-snap gates, run the prep sub-step, and (only when `arg`==0)
// reset the guts subsystem (state/mode resync + two configure calls). Then arm
// the guts busy words (m_busyA=1, m_busyB=arg, m_548=1) and republish the level
// clock into m_savedClock. Migrated from engine_boundary (CPlay). Returns 1.
// ===========================================================================
RVA(0x000d6440, 0xd3)
i32 CPlay::EnterOverlayDrag(i32 arg) {
    if (m_overlayDrag != 0) {
        return 1;
    }
    m_overlayDrag = 1;
    m_worldReady = 0;
    m_dragSnapActive = 0;
    Helper2c7f();
    if (arg == 0) {
        GutsSubsystem* g = m_guts;
        if (g->m_state == 2) {
            g->Guts123f();
        }
        if (g->m_mode != 5) {
            g->Guts1d61(5, 3);
        }
        g->Guts427d(0x1fb, 1);
        g->Guts125d();
    }
    m_guts->Guts35b2(1);
    GutsSubsystem* g = m_guts;
    g->m_busyA = 1;
    g->m_busyB = arg;
    g->Guts12fd(0);
    g->Guts16ea();
    g->m_548 = 1;
    g->Guts125d();
    m_savedClock = g_645588;
    return 1;
}

// CPlay::ForwardReady (0x0cee70) - a 5-byte vtable forwarder: tail-call the
// slot-3 ready gate (CState::Vfunc3) -> `mov eax,[ecx]; jmp [eax+0xc]`.
RVA(0x000cee70, 0x5)
i32 CPlay::ForwardReady() {
    return Vfunc3();
}

// CPlay::PauseGame (0x0cee90) - vtable slot 24 (shared by CDemo/CMulti). Flush
// the pending mode ops, freeze the guts subsystem (passing whether we were
// running), clear the world-ready / drag-snap gates, and save the running game
// clock into m_savedClock. Migrated from engine_boundary (CPlay).
RVA(0x000cee90, 0x49)
i32 CPlay::PauseGame() {
    Helper2c7f();
    if (m_paused) {
        m_guts->Guts35b2(0);
    } else {
        m_guts->Guts35b2(1);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    m_savedClock = g_645588;
    return 1;
}

// CPlay::ResumeGame (0x0cef00) - vtable slot 25. Step the guts subsystem, restore
// the saved game clock from m_savedClock, clear the paused flag, and (if the guts object
// is live) run its resume sub-step. Migrated from engine_boundary (CPlay).
RVA(0x000cef00, 0x39)
i32 CPlay::ResumeGame() {
    m_guts->Guts367a();
    g_645588 = m_savedClock;
    m_paused = 0;
    if (m_guts != 0) {
        m_guts->Guts125d();
    }
    return 1;
}

// LoadCursorSprites (0xd0120): select + load the on-screen cursor sprite set for a tool
// `frame`. Early-outs when the requested (frame,flag) already matches the loaded pair.
// Frame 1..0x26 = the numeric chip cursor; 0 = the plain pointer; 0x66 = the flailing-grunt
// cursor (which also fires a booty cue + arms the +0x328 one-shot timer); 0xc8..0xe8 = the
// per-tool cursor table (a dense switch, one GAME_CURSORZ_* per tool). Each path loads via
// the reloc-masked LoadCursor helper (0x39ea) and, on success, stamps the m_2fc/m_300/m_504/
// m_2f8 cursor state. Fields beyond CPlay's modeled layout are reached by a typed self-view.
SIZE_UNKNOWN(CursorFxObj);
struct CursorFxObj { // the held cursor-anim object at m_4e4 (its +0x40 flag word toggles)
    char m_pad00[0x40];
    i32 m_40; // +0x40 flag word (bit0)
};
// The on-screen cue sink (g_64556c->m_cueSink); the flailing-grunt path fires a 6-arg cue.
SIZE_UNKNOWN(CursorCueSink);
struct CursorCueSink {
    void PlayCue(i32 g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x39f4 thiscall
};
SIZE_UNKNOWN(CursorSelf);
struct CursorSelf {
    char m_pad00[0x2f8];
    i32 m_2f8; // +0x2f8 loaded frame id
    i32 m_2fc; // +0x2fc
    i32 m_300; // +0x300
    char m_pad304[0x328 - 0x304];
    i32 m_328; // +0x328 one-shot timer stamp
    i32 m_32c; // +0x32c
    i32 m_330; // +0x330 one-shot interval
    i32 m_334; // +0x334
    char m_pad338[0x368 - 0x338];
    i32 m_368; // +0x368 flailing-grunt gate
    i32 m_36c; // +0x36c chip-cursor gate
    char m_pad370[0x4e4 - 0x370];
    CursorFxObj* m_4e4; // +0x4e4 held cursor-anim object
    char m_pad4e8[0x504 - 0x4e8];
    i32 m_504; // +0x504 loaded flag
    // The named-cursor-set loader (thunk 0x39ea, __thiscall, reloc-masked).
    i32 LoadCursor(const char* name, i32 a, i32 b, i32 c, i32 d);
};

// @confidence: med
// @source: string-xref
// @early-stop
// ~93%: complete + correct (the early-out guard, all four dispatch arms - the 1..0x26
// chip range, the pointer, the flailing-grunt cue arm, and the full 33-case tool-cursor
// switch - all match, every GAME_CURSORZ_*/helper named). Residual walls: (1) the tool
// switch's range check - retail emits the signed two-bound form (cmp eax,0xc8;jl + sub;cmp
// 0x20;ja) where cl folds it to the single unsigned check; (2) the jump-table dispatch is
// the delinker's `jmp [eax*4+$L]` reloc-typing vs cl's separate DIR32 base (same bytes);
// (3) the three prefix blocks reload `frame` into a different scratch reg (edx vs eax) for
// the trailing m_2f8 store. All logic + externs/strings named.
RVA(0x000d0120, 0x5d8)
i32 CPlay::LoadCursorSprites(i32 frame, i32 flag) {
    CursorSelf* self = (CursorSelf*)this;
    if (self->m_2f8 == frame && flag == self->m_504) {
        return 1;
    }
    if (frame >= 1 && frame <= 0x26) {
        if (self->LoadCursor("GAME_INGAMEICONZ_NORMCHIPZ", frame, 0, 0x64, 0) == 0) {
            return 0;
        }
        if (self->m_4e4 != 0) {
            self->m_4e4->m_40 |= 1;
        }
        self->m_2fc = 0;
        self->m_300 = 0;
        self->m_36c = 1;
        self->m_504 = 0;
        self->m_2f8 = frame;
        return 1;
    }
    if (frame == 0) {
        if (self->LoadCursor("GAME_CURSORZ_POINTER", 1, 1, 0x64, 0) == 0) {
            return 0;
        }
        if (self->m_4e4 != 0) {
            self->m_4e4->m_40 &= ~1;
        }
        self->m_2fc = 0x10;
        self->m_300 = 0x10;
        self->m_504 = 0;
        self->m_2f8 = frame;
        return 1;
    }
    if (frame == 0x66) {
        if (self->LoadCursor("GAME_CURSORZ_FLAILINGGRUNT", 1, 1, 0x64, 1) == 0) {
            return 0;
        }
        if (self->m_4e4 != 0) {
            self->m_4e4->m_40 |= 1;
        }
        self->m_2fc = 0;
        self->m_300 = 0;
        self->m_368 = 1;
        self->m_504 = 0;
        ((CursorCueSink*)g_64556c->m_cueSink)->PlayCue(0, 0x33e, -1, 1, -1, -1);
        self->m_330 = 0x2710;
        self->m_334 = 0;
        self->m_328 = g_645588;
        self->m_32c = 0;
        self->m_2f8 = frame;
        return 1;
    }
    switch (frame) {
        case 0xc8:
            if (self->LoadCursor("GAME_CURSORZ_HANDZ", 1, flag, 0x64, 1) == 0) {
                return 0;
            }
            break;
        case 0xc9:
            if (self->LoadCursor("GAME_CURSORZ_BOMBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xca:
            if (self->LoadCursor("GAME_CURSORZ_BOOMERANGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcb:
            if (self->LoadCursor("GAME_CURSORZ_BRICKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcc:
            if (self->LoadCursor("GAME_CURSORZ_CLUBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcd:
            if (self->LoadCursor("GAME_CURSORZ_GAUNTLETZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xce:
            if (self->LoadCursor("GAME_CURSORZ_GLOVEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcf:
            if (self->LoadCursor("GAME_CURSORZ_GOOBERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd0:
            if (self->LoadCursor("GAME_CURSORZ_GRAVITYBOOTZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd1:
            if (self->LoadCursor("GAME_CURSORZ_GUNHATZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd2:
            if (self->LoadCursor("GAME_CURSORZ_NERFGUNZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd3:
            if (self->LoadCursor("GAME_CURSORZ_ROCKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd4:
            if (self->LoadCursor("GAME_CURSORZ_SHIELDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd5:
            if (self->LoadCursor("GAME_CURSORZ_SHOVELZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd6:
            if (self->LoadCursor("GAME_CURSORZ_SPRINGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd7:
            if (self->LoadCursor("GAME_CURSORZ_SPYZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd8:
            if (self->LoadCursor("GAME_CURSORZ_SWORDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd9:
            if (self->LoadCursor("GAME_CURSORZ_TIMEBOMBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xda:
            if (self->LoadCursor("GAME_CURSORZ_TOOBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdb:
            if (self->LoadCursor("GAME_CURSORZ_WANDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdc:
            if (self->LoadCursor("GAME_CURSORZ_WARPSTONEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdd:
            if (self->LoadCursor("GAME_CURSORZ_WELDERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xde:
            if (self->LoadCursor("GAME_CURSORZ_WINGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdf:
            if (self->LoadCursor("GAME_CURSORZ_BABYWALKERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe0:
            if (self->LoadCursor("GAME_CURSORZ_BEACHBALLZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe1:
            if (self->LoadCursor("GAME_CURSORZ_BIGWHEELZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe2:
            if (self->LoadCursor("GAME_CURSORZ_GOKARTZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe3:
            if (self->LoadCursor("GAME_CURSORZ_JACKINTHEBOXZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe4:
            if (self->LoadCursor("GAME_CURSORZ_JUMPROPEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe5:
            if (self->LoadCursor("GAME_CURSORZ_POGOSTICKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe6:
            if (self->LoadCursor("GAME_CURSORZ_SCROLLZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe7:
            if (self->LoadCursor("GAME_CURSORZ_SQUEAKTOYZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe8:
            if (self->LoadCursor("GAME_CURSORZ_YOYOZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (self->m_4e4 != 0) {
        self->m_4e4->m_40 |= 1;
    }
    self->m_2fc = 0;
    self->m_300 = 0;
    self->m_504 = flag;
    self->m_2f8 = frame;
    return 1;
}

// CPlay::LoadScrollSpeedOptions (0xd12b0): lazy-load the bute-configured scroll
// speed range on first use, then run the per-frame edge auto-scroll. Four edge
// zones (left/right/top/bottom); each times a mouse-at-edge dwell against
// timeGetTime and nudges the plane-geom scroll offset by (elapsed*speed/100)
// clamped to 100 px, committing the new offset when anything moved. speed =
// (int)((double)m_4->m_124 * 0.01 * range + min). The bute getter, timeGetTime
// ptr and the ApplyScroll tail are reloc-masked; only offsets + code bytes bind.
// @early-stop
// scheduling wall (85.9%, from 0% stub): logic + all four edge blocks byte-faithful.
// Residual is MSVC's interleave of the geom pointer-chase (sx/sy loads) into the
// float speed-computation FPU latency gaps (fild/fmul/fimul/fiadd/ftol) + the
// trailing nop padding - not source-steerable (zero-register-pinning family).
extern "C" u8 g_scrollLoadFlags;          // 0x64c01c  lazy-load bitset (bit0 min, bit1 max)
extern "C" i32 g_scrollMinSpeed;          // 0x64c274  cached MinScrollSpeed
extern "C" i32 g_scrollSpeedRange;        // 0x64c270  cached (Max - Min)
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650
extern CButeMgr g_buteMgr;                // 0x6453d8
extern "C" double g_scrollSpeedScale;     // 0x5eaa10  (== 0.01)

SIZE_UNKNOWN(ScrollGeom);
struct ScrollGeom {
    char p0[0x84];
    i32 m_84, m_88; // +0x84/88  live scroll x/y offset
};
SIZE_UNKNOWN(ScrollWorld);
struct ScrollWorld {
    char p0[0x30];
    struct M30 {
        char p0[0x24];
        struct M24 {
            char p0[0x5c];
            ScrollGeom* m_5c; // +0x5c  plane geom
        }* m_24;              // +0x24
    }* m_30;                  // +0x30
    char p34[0x8c - 0x34];
    i32 m_8c; // +0x8c  x-extent
    i32 m_90; // +0x90  y-extent
    char p94[0x124 - 0x94];
    i32 m_124; // +0x124  speed base (fild)
};
struct ScrollView { // CPlay view for the auto-scroll path (offset access)
    char p0[0x4];
    ScrollWorld* m_4; // +0x04  world/level
    char p8[0x150 - 0x8];
    i32 m_cursorX; // +0x150  cursor x
    i32 m_cursorY; // +0x154  cursor y
    char p158[0x4b4 - 0x158];
    i32 m_scrollEdgeActive; // +0x4b4  edge active bits
    i32 m_scrollEdgeLock;   // +0x4b8  edge lock bits
    char p4bc[0x508 - 0x4bc];
    i32 m_lastScrollTimeX;          // +0x508  last-scroll time (horizontal)
    i32 m_lastScrollTimeY;          // +0x50c  last-scroll time (vertical)
    void ApplyScroll(i32 x, i32 y); // 0x2e28 thunk (thiscall, reloc-masked)
};
RVA(0x000d12b0, 0x2d5)
i32 CPlay::LoadScrollSpeedOptions() {
    if (!(g_scrollLoadFlags & 1)) {
        g_scrollLoadFlags |= 1;
        g_scrollMinSpeed = g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    }
    if (!(g_scrollLoadFlags & 2)) {
        g_scrollLoadFlags |= 2;
        g_scrollSpeedRange = g_buteMgr.GetInt("Optionz", "MaxScrollSpeed")
                             - g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    }

    ScrollView* self = (ScrollView*)this;
    ScrollWorld* w = self->m_4;
    i32 changed = 0;
    i32 speed =
        (i32)((double)w->m_124 * g_scrollSpeedScale * g_scrollSpeedRange + g_scrollMinSpeed);
    ScrollGeom* g = w->m_30->m_24->m_5c;
    i32 sx = g->m_84;
    i32 sy = g->m_88;
    i32 extentX = w->m_8c;
    i32 extentY = w->m_90;

    // LEFT edge
    if (self->m_cursorX < 0xc || (self->m_scrollEdgeLock & 1)) {
        if (self->m_scrollEdgeActive & 1) {
            i32 d = (g_pTimeGetTime() - self->m_lastScrollTimeX) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sx -= d;
                self->m_lastScrollTimeX = g_pTimeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 1;
            self->m_lastScrollTimeX = g_pTimeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~1;
    }

    // RIGHT edge
    if (self->m_cursorX > extentX - 0xc || (self->m_scrollEdgeLock & 4)) {
        if (self->m_scrollEdgeActive & 4) {
            i32 d = (g_pTimeGetTime() - self->m_lastScrollTimeX) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sx += d;
                self->m_lastScrollTimeX = g_pTimeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 4;
            self->m_lastScrollTimeX = g_pTimeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~4;
    }

    // TOP edge
    if (self->m_cursorY < 0xf || (self->m_scrollEdgeLock & 2)) {
        if (self->m_scrollEdgeActive & 2) {
            i32 d = (g_pTimeGetTime() - self->m_lastScrollTimeY) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sy -= d;
                self->m_lastScrollTimeY = g_pTimeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 2;
            self->m_lastScrollTimeY = g_pTimeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~2;
    }

    // BOTTOM edge
    if (self->m_cursorY > extentY - 0xf || (self->m_scrollEdgeLock & 8)) {
        if (self->m_scrollEdgeActive & 8) {
            i32 d = (g_pTimeGetTime() - self->m_lastScrollTimeY) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sy += d;
                self->m_lastScrollTimeY = g_pTimeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 8;
            self->m_lastScrollTimeY = g_pTimeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~8;
    }

    if (changed) {
        self->ApplyScroll(sx, sy);
    }
    return 1;
}

// BuildGruntTypeNameTable (0xdc6d0): map a grunt-type id to its bute namespace key
// via a 58-case jump table (NORMALGRUNT default), then register it through the shared
// namespace-loader tail (BindWarlordName == the 0x2bc1 CNamespaceLoader thunk). The
// TOOB case is special: it registers TOOBGRUNT first and, only if that succeeds, ALSO
// registers TOOBWATERGRUNT (a separate `return`, not a `break`, so a2/a3/a4 stay in
// edi/ebx/ebp local to the TOOB block instead of being hoisted for the whole fn). The
// CString name temp forces the /GX EH frame.
// @early-stop
// jump-table-data-overlap wall (33.3%, from 0% stub): the full body is byte-exact vs
// retail (verified llvm-objdump -dr base vs target — prologue, dispatch, all 58 case
// pushes, the TOOB/TOOBWATER special path, and the shared destruct-tail all match). The
// residual is the 194-byte switch data (58-entry index byte-table + 34-slot jump table)
// which cl emits as separate $L symbols vs the delinker inlining it into the fn symbol
// at fn+0x218/+0x2a4; the table DATA + the 2 dispatch reloc operands never pair. Not
// source-steerable (docs/patterns/jumptable-data-overlap.md, cf. LoadPowerupIconSprites).
RVA(0x000dc6d0, 0x215)
i32 CPlay::BuildGruntTypeNameTable(i32 typeIdx, i32 a2, i32 a3, i32 a4) {
    CString name("NORMALGRUNT");
    switch (typeIdx) {
        case 1:
            name = "BOMBGRUNT";
            break;
        case 2:
            name = "BOOMERANGGRUNT";
            break;
        case 3:
            name = "BRICKGRUNT";
            break;
        case 4:
            name = "CLUBGRUNT";
            break;
        case 5:
            name = "GAUNTLETZGRUNT";
            break;
        case 6:
            name = "GLOVEZGRUNT";
            break;
        case 7:
            name = "GOOBERGRUNT";
            break;
        case 8:
            name = "GRAVITYBOOTZGRUNT";
            break;
        case 9:
            name = "GUNHATGRUNT";
            break;
        case 10:
            name = "NERFGUNGRUNT";
            break;
        case 11:
            name = "ROCKGRUNT";
            break;
        case 12:
            name = "SHIELDGRUNT";
            break;
        case 13:
            name = "SHOVELGRUNT";
            break;
        case 14:
            name = "SPRINGGRUNT";
            break;
        case 15:
            name = "SPYGRUNT";
            break;
        case 16:
            name = "SWORDGRUNT";
            break;
        case 17:
            name = "TIMEBOMBGRUNT";
            break;
        case 18:
            name = "TOOBGRUNT";
            if (this->BindWarlordName(name, a2, a3, a4) == 0) {
                return 0;
            }
            name = "TOOBWATERGRUNT";
            return this->BindWarlordName(name, a2, a3, a4);
        case 19:
            name = "WANDGRUNT";
            break;
        case 20:
            name = "WARPSTONEGRUNT";
            break;
        case 21:
            name = "WELDERGRUNT";
            break;
        case 22:
            name = "WINGZGRUNT";
            break;
        case 23:
            name = "BABYWALKERGRUNT";
            break;
        case 24:
            name = "BEACHBALLGRUNT";
            break;
        case 25:
            name = "BIGWHEELGRUNT";
            break;
        case 26:
            name = "GOKARTGRUNT";
            break;
        case 27:
            name = "JACKINTHEBOXGRUNT";
            break;
        case 28:
            name = "JUMPROPEGRUNT";
            break;
        case 29:
            name = "POGOSTICKGRUNT";
            break;
        case 30:
            name = "SCROLLGRUNT";
            break;
        case 31:
            name = "SQUEAKTOYGRUNT";
            break;
        case 32:
            name = "YOYOGRUNT";
            break;
        case 57:
            name = "HAREKRISHNAGRUNT";
            break;
        case 58:
            name = "REAPERGRUNT";
            break;
    }
    return this->BindWarlordName(name, a2, a3, a4);
}

// ===========================================================================
// Per-level resource loaders (trace-discovered CPlay __thiscall cluster).
// ===========================================================================
// A small family of asset-namespace loaders. Each reaches the game resource
// registry through this->m_c (->m_10 image/tile registry [virtual install slot
// 18 = +0x48], ->m_28 sound registry, ->m_2c animation registry) and looks the
// level's named set up off a bank source (this->m_levelBank or this->m_gameBank, both expose
// LookupSet @0x13bae0). LoadImageBanks (the parent) caches the GRUNTZ/GAME banks
// into this->m_gruntzBank/m_gameBank first; the per-type loaders then read this->m_gameBank as their
// source. Helpers are modeled NO-body so their `call`s reloc-mask; only OFFSETS +
// code bytes are load-bearing. The loaders touch sub-object faces (m_c->m_10,
// m_8->Lookup) that Render's matched member typing models differently, so each
// casts `this` to the self-contained CPlayRes view below (a struct-view-of-this
// overlay - matching-neutral, keeps Render untouched).

extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA @0x6bf37c (mangled-name match)
extern "C" char g_emptyString[];    // _g_emptyString @0x6293f4

// A level's named-set source (this->m_levelBank / this->m_gameBank, and the banks cached in
// m_gruntzBank/m_gameBank). LookupSet (0x13bae0) resolves "TILEZ"/"IMAGEZ"/"SOUNDZ"/"ANIZ".
struct CResSource {
    void* LookupSet(char* szName); // 0x13bae0 __thiscall, ret set ptr
};
// The bank manager at this->m_8: Lookup (0x13c030) resolves a "GRUNTZ"/"GAME"
// bank into a CResSource (LoadImageBanks caches the result in m_gruntzBank/m_gameBank).
struct CBankMgr {
    CResSource* Lookup(char* szName); // 0x13c030 __thiscall
};
// The loader family reaches its resource state directly through `this` (a CPlay):
// the bank manager (CState::m_8), the level/GRUNTZ/GAME banks (CState::m_levelBank/
// m_gruntzBank/m_gameBank) and the shared CView resource registries (CState::m_c->m_10/m_28/m_2c).

// LoadImageBanks (0x0cffe0) - cache the GRUNTZ + GAME asset banks off m_8 into
// m_30/m_34; the int (BOOL) return reuses the just-loaded value at each guard.
RVA(0x000cffe0, 0x3c)
i32 CPlay::LoadImageBanks() {
    CPlay* self = this;
    if (!self->m_8) {
        return 0;
    }
    self->m_gruntzBank = self->m_8->Lookup("GRUNTZ");
    if (!self->m_gruntzBank) {
        return 0;
    }
    self->m_gameBank = self->m_8->Lookup("GAME");
    return self->m_gameBank != 0;
}

// LoadActionTileSprites (0x0db600) - register the ACTION/BACK namespaces then
// install the level's TILEZ set through the +0x48 virtual slot. int (BOOL) return:
// each guard `return 0` reuses the just-zeroed eax, the success path is `mov
// eax,1`; a void return tail-merges the bare epilogues and never emits eax=1.
RVA(0x000db600, 0x8f)
i32 CPlay::LoadActionTileSprites(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && self->m_c->m_imageRegistry->Has("ACTION")) {
        return 1;
    }

    self->m_c->m_imageRegistry->Register("ACTION", g_emptyString);
    self->m_c->m_imageRegistry->Register("BACK", g_emptyString);
    g_resourceInstallActive = 0;

    void* tiles = self->m_levelBank->LookupSet("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_c->m_imageRegistry->Install(tiles, g_emptyString, "_");
    return 1;
}

// LoadLevelSounds (0x0db6c0) - register the LEVEL namespace then install the
// level's SOUNDZ set through m_c->m_28 (non-virtual). Single Register, no install-gate
// reset. Same int-return idiom as its siblings.
RVA(0x000db6c0, 0x70)
i32 CPlay::LoadLevelSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && self->m_c->m_soundRegistry->Has("LEVEL")) {
        return 1;
    }

    self->m_c->m_soundRegistry->Register("LEVEL", "_");

    void* sounds = self->m_levelBank->LookupSet("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    self->m_c->m_soundRegistry->Install(sounds, "LEVEL", "_");
    return 1;
}

// LoadLevelImages (0x0db7e0) - sibling of LoadActionTileSprites; a single Register
// (LEVEL), then install the level's IMAGEZ set through the +0x48 virtual slot.
// Brackets the install with two install-active resets.
RVA(0x000db7e0, 0x84)
i32 CPlay::LoadLevelImages(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && self->m_c->m_imageRegistry->Has("LEVEL")) {
        return 1;
    }

    self->m_c->m_imageRegistry->Register("LEVEL", "_");
    g_resourceInstallActive = 0;

    void* images = self->m_levelBank->LookupSet("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_imageRegistry->Install(images, "LEVEL", "_");
    g_resourceInstallActive = 0;
    return 1;
}

// LoadGameImages (0x0db8a0) - the GAME-namespace image loader. No force gate, no
// Register; reads m_gameBank (the cached GAME bank) for the IMAGEZ set, installs through
// the +0x48 virtual slot. Brackets the install with the install-active flag = 1 then 0.
RVA(0x000db8a0, 0x67)
i32 CPlay::LoadGameImages(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (self->m_c->m_imageRegistry->Has("GAME")) {
        return 1;
    }

    g_resourceInstallActive = 1;
    void* images = self->m_gameBank->LookupSet("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_imageRegistry->Install(images, "GAME", "_");
    g_resourceInstallActive = 0;
    return 1;
}

// LoadGameSounds (0x0db930) - the GAME-namespace sound loader. Reads m_gameBank for the
// SOUNDZ set and installs through m_c->m_28 (non-virtual). No Register, no install-gate.
RVA(0x000db930, 0x53)
i32 CPlay::LoadGameSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (self->m_c->m_soundRegistry->Has("GAME")) {
        return 1;
    }

    void* sounds = self->m_gameBank->LookupSet("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    self->m_c->m_soundRegistry->Install(sounds, "GAME", "_");
    return 1;
}

// LoadGameAnims (0x0db9b0) - the GAME-namespace animation loader. Reads m_gameBank for
// the ANIZ set and installs through m_c->m_2c (non-virtual). No Register/install-gate.
RVA(0x000db9b0, 0x53)
i32 CPlay::LoadGameAnims(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (self->m_c->m_animRegistry->Has("GAME")) {
        return 1;
    }

    void* anims = self->m_gameBank->LookupSet("ANIZ");
    if (!anims) {
        return 0;
    }
    self->m_c->m_animRegistry->Install(anims, "GAME", "_");
    return 1;
}

// ===========================================================================
// CPlay::BuildMusicCategoryTable (0x0dba30) - install the level's + game's XMI music
// categories into the sound manager's category table (m_4->m_48). For each of the
// two MIDIZ sources (level m_levelBank, game m_gameBank) it resolves a fixed list of category
// names ("AMBIENT0/1", "INTRO0/1" for the level; "POWERUP", "CURSE", "MONOLITH" for
// the game) as 'XMI' resources and, if the resolved entry loads, installs it under
// its name. The arg (force flag) is unused. __thiscall, ret 4.
// ===========================================================================
// A resolved music-category entry (Resolve result): +0xc field + a Load() accessor.
struct CMusicEntry {
    void* Load(); // 0x539960 (thiscall) -> resource ptr (null if absent)
    char p0[0xc];
    void* m_c; // +0xc  install key
};
// A named MIDIZ category set (LookupSet result).
struct CMusicSet {
    CMusicEntry* Resolve(char* name, i32 tag); // 0x53a000 (thiscall)
};
// A music source (level m_levelBank / game m_gameBank).
struct CMusicSource {
    CMusicSet* LookupSet(char* name); // 0x53bae0 (thiscall)
};
// The destination category table (the sound manager at m_4->m_48).
struct CMusicCatTable {
    void Clear();                                   // 0x538530 (thiscall, no arg)
    void Install(void* res, void* key, char* name); // 0x538670 (thiscall)
};
// Typed view of `this` for this builder: m_4->m_48 = sound manager, m_28/m_34 sources.
struct CMusicOwner {
    char p0[0x4];
    struct SndHolder {
        char p0[0x48];
        CMusicCatTable* m_48; // +0x48
    }* m_4;                   // +0x04
    char p8[0x28 - 0x8];
    CMusicSource* m_levelBank; // +0x28  level MIDIZ bank (== CState::m_levelBank)
    char p2c[0x34 - 0x2c];
    CMusicSource* m_gameBank; // +0x34  game MIDIZ bank (== CState::m_gameBank)
};

#define MUSIC_TAG_XMI 0x584d49 // 'XMI'

RVA(0x000dba30, 0x1ca)
i32 CPlay::BuildMusicCategoryTable(i32) {
    CMusicOwner* self = (CMusicOwner*)this;
    self->m_4->m_48->Clear();

    CMusicSet* levelSet = self->m_levelBank->LookupSet("MIDIZ");
    if (levelSet) {
        CMusicEntry* e = levelSet->Resolve("AMBIENT0", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "AMBIENT0");
            }
        }
        e = levelSet->Resolve("AMBIENT1", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "AMBIENT1");
            }
        }
        e = levelSet->Resolve("INTRO0", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "INTRO0");
            }
        }
        e = levelSet->Resolve("INTRO1", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "INTRO1");
            }
        }
    }

    CMusicSource* gameSrc = self->m_gameBank;
    CMusicSet* gameSet = gameSrc->LookupSet("MIDIZ");
    if (gameSet) {
        CMusicEntry* e = gameSet->Resolve("POWERUP", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "POWERUP");
            }
        }
        e = gameSet->Resolve("CURSE", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "CURSE");
            }
        }
        e = gameSet->Resolve("MONOLITH", MUSIC_TAG_XMI);
        if (e) {
            void* res = e->Load();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "MONOLITH");
            }
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::LoadGruntSoundNamespaces (0x0dd830) - register the per-grunt sound
// namespaces (GRUNTZ_<X>) into the level sound registry (m_c->m_28), sourcing each
// SOUNDZ_<X> set off the GRUNTZ bank (m_gruntzBank). The first three namespaces load silently;
// the last four also tick the load-notify object (arg) when present. __thiscall.
// ===========================================================================
// The per-namespace load-notify sink (arg): OnLoaded() probes its ready flags and
// posts a progress message. External/reloc-masked.
struct CLoadNotify {
    void OnLoaded(); // 0x4bc420 (thiscall, no arg)
};

RVA(0x000dd830, 0x1e3)
i32 CPlay::LoadGruntSoundNamespaces(CLoadNotify* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }

    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_NORMALGRUNT")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_NORMALGRUNT");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_NORMALGRUNT", "_");
        }
    }
    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_DEATHZ")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_DEATHZ");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_DEATHZ", "_");
        }
    }
    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_ENTRANCEZ")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_ENTRANCEZ");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_ENTRANCEZ", "_");
        }
    }
    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_EXITZ")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_EXITZ");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_EXITZ", "_");
        }
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_GRUNTPUDDLE")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_GRUNTPUDDLE");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_GRUNTPUDDLE", "_");
        }
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_PICKUPS")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_PICKUPS");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_PICKUPS", "_");
        }
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_soundRegistry->Has("GRUNTZ_BOMBGRUNT")) {
        void* s = self->m_gruntzBank->LookupSet("SOUNDZ_BOMBGRUNT");
        if (s) {
            self->m_c->m_soundRegistry->Install(s, "GRUNTZ_BOMBGRUNT", "_");
        }
        if (notify) {
            notify->OnLoaded();
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::BuildSpriteImageKeyTable (0x0dd540) - install the per-grunt IMAGE namespaces
// (GRUNTZ_<X>) into the image registry (m_c->m_10, virtual install at +0x48), each
// sourced from the GRUNTZ bank's IMAGEZ_<X> set; brackets the run with the install-active
// counter (1 .. 0) and ticks the notify object after each install. A missing source
// set aborts (return 0, install-active flag left set). __thiscall.
// ===========================================================================
RVA(0x000dd540, 0x241)
i32 CPlay::BuildSpriteImageKeyTable(CLoadNotify* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    g_resourceInstallActive = 1;
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_NORMALGRUNT")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_DEATHZ")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_ENTRANCEZ")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_EXITZ")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_GRUNTPUDDLE")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_PICKUPS")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_imageRegistry->Has("GRUNTZ_BOMBGRUNT")) {
        void* s = self->m_gruntzBank->LookupSet("IMAGEZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_imageRegistry->Install(s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    g_resourceInstallActive = 0;
    return 1;
}

// ===========================================================================
// CPlay::BuildAnizKeyTable (0x0ddaa0) - install the per-grunt ANIM namespaces
// (GRUNTZ_<X>) into the animation registry (m_c->m_2c), each sourced from the GRUNTZ
// bank's ANIZ_<X> set; ticks the notify object after each install. A missing source
// set aborts (return 0). __thiscall.
// ===========================================================================
RVA(0x000ddaa0, 0x228)
i32 CPlay::BuildAnizKeyTable(CLoadNotify* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_NORMALGRUNT")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_DEATHZ")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_ENTRANCEZ")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_EXITZ")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_GRUNTPUDDLE")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_PICKUPS")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    if (!self->m_c->m_animRegistry->Has("GRUNTZ_BOMBGRUNT")) {
        void* s = self->m_gruntzBank->LookupSet("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_animRegistry->Install(s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->OnLoaded();
        }
    }
    return 1;
}

// ===========================================================================
// The trace-discovered CPlay __thiscall cluster (analyzed; this TU).
// ===========================================================================

// The "ShowCursor" Win32 import slot, reused verbatim from the engine
// (?g_ShowCursor@@3P6GHH@ZA, an indirect `i32 __stdcall(i32)` fn-ptr global).
typedef i32(WINAPI* ShowCursorFn)(i32);
DATA(0x002c44c4)
extern ShowCursorFn g_ShowCursor;

// ResetForMode (0x0c8a10) - capture the live cursor position into the
// BeginFrameClear args (m_cursorX/m_cursorY), force the cursor hidden, enter the
// requested mode (publishing the level-start clock for mode 9), then reset the
// per-frame drag/world-ready latches and the three world sub-objects.
RVA(0x000c8a10, 0x119)
i32 CPlay::ResetForMode(i32 mode) {
    POINT pt;
    GetCursorPos(&pt);
    ShowCursorFn showCursor = g_ShowCursor;
    m_cursorX = pt.x;
    m_cursorY = pt.y;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    if (mode == 9) {
        g_645588 = m_savedClock;
        if (!EnterMode(9)) {
            return 0;
        }
        m_stepCountdown = 2;
    } else if (m_renderDisabled == 0 || m_4w()->m_134 == 2) {
        if (!EnterMode(mode)) {
            return 0;
        }
    }
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    m_dragSnapActive = 0;
    m_dragInProgress = 0;
    m_dragInhibit1 = 0;
    m_dragInhibit2 = 0;
    m_dragEndNotify = 0;
    m_worldReady = 0;
    if (m_renderDisabled == 0) {
        if (mode != 9) {
            m_4w()->m_54->Reset();
        }
        m_4w()->m_68->Reset();
        m_4w()->m_60->Reset();
    }
    return 1;
}

// FindStartPointAt (0x0d5f90) - a registry-gated hit-test over this->m_markerData[]
// markers: bail unless the active config slot's per-slot counter is below its
// cap, then return the first marker whose +-0x20 box contains (x, y), reporting
// its coords. __thiscall(x, y, outX, outY), ret 0x10.
struct CHitMarker {
    i32 m_0; // x
    i32 m_4; // y
};
// g_64556c view: a per-active-slot config array at +0x150 (stride 0x238) plus the
// +0x68 sub-object's per-slot value table at +0x10c (gated against slot->m_228).
struct CRegSlot {
    char p0[0x228];
    i32 m_228; // threshold cap
    char p22c[0x238 - 0x22c];
};
struct CRegHitGate {
    char p0[0x10c];
    i32 m_10c[1]; // +0x10c  per-slot value table (indexed by active id)
};
struct CRegHitView {
    char p0[0x68];
    CRegHitGate* m_68; // +0x68
    char p6c[0x150 - 0x6c];
    CRegSlot m_slots[1]; // +0x150  stride 0x238
};

// @early-stop
// ~83% regalloc-coloring wall: logic + all relocs pair, but MSVC5 colors the
// registry base into edx and the id*0x238 slot-index into ecx (we get the
// opposite swap), cascading through the whole gate; plus a return-0 epilogue
// tail-merge difference. Not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x000d5f90, 0xd7)
i32 CPlay::FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY) {
    i32 id = g_644c54;
    CRegHitView* reg = (CRegHitView*)g_64556c;
    CRegSlot* slot = &reg->m_slots[id];
    if (slot == 0) {
        return 0;
    }
    if (reg->m_68->m_10c[id] >= slot->m_228) {
        return 0;
    }
    for (i32 i = 0; i < markerCount(); i++) {
        CHitMarker* m = markerData()[i];
        if (m != 0) {
            RECT rc;
            SetRect(&rc, m->m_0 - 0x20, m->m_4 - 0x20, m->m_0 + 0x20, m->m_4 + 0x20);
            if (x < rc.right && x >= rc.left && y < rc.bottom && y >= rc.top) {
                *outX = m->m_0;
                *outY = m->m_4;
                return 1;
            }
        }
    }
    return 0;
}

// ResetPlayState (0x0d60b0) - the per-frame play-state reset OnKeyCommand runs to
// (re)prime the level: pick the INTRO vs AMBIENT ambient-sound cue (formatting
// "INTRO%d"/"AMBIENT%d" off GetAmbientId), re-seed the ambient timer, reset the
// goal geometry / per-slot config rows, clear the win-lose/in-game latches and the
// frame-marker timeline. __thiscall, no args, ret 1 (0 if PrepareReset bails).
// Self-contained views keep Render's CPlay/CWorld typing untouched. The sound
// manager (m_4->m_48) is the real CGruntzSoundZ (<Dsndmgr/CGruntzSoundZ.h>,
// included above): FindBank (0x138730), PlayByName (0x138840, discarded here) and
// m_pCurrent (+0x1c); the found bank is a CGruntzSoundInnerZ and Play(1) is its
// SetLoop (0x139030).
struct CRpGeom { // m_4->m_30->m_24 (the goal-geometry block)
    char p0[0x3b0];
    i32 m_3b0; // +0x3b0
    i32 m_3b4; // +0x3b4
};
struct CRpM30 {
    char p0[0x24];
    CRpGeom* m_24; // +0x24
};
struct CRpWho7c {                  // m_4->m_7c
    void Notify(i32 id, i32 flag); // 0x1c8f thunk __thiscall(id, flag)
};
struct CRpTimeline { // m_4->m_68 (the per-frame world timeline)
    char p0[0x288];
    i32 m_288; // +0x288
    char p28c[0x2a4 - 0x28c];
    i32 m_2a4; // +0x2a4
    i32 m_2a8; // +0x2a8
    char p2ac[0x2b0 - 0x2ac];
    i32 m_2b0; // +0x2b0
    i32 m_2b4; // +0x2b4
    i32 m_2b8; // +0x2b8
    i32 m_2bc; // +0x2bc
    i32 m_2c0; // +0x2c0
    i32 m_2c4; // +0x2c4
    i32 m_2c8; // +0x2c8
    i32 m_2cc; // +0x2cc
    char p2d0[0x3ec - 0x2d0];
    i32 m_3ec; // +0x3ec
    char p3f0[0x3f8 - 0x3f0];
    i32 m_3f8; // +0x3f8
    i32 m_3fc; // +0x3fc
    i32 m_400; // +0x400
};
struct CRpWorld { // this->m_4
    char p0[0x14];
    i32 m_14; // +0x14  view gate
    char p18[0x30 - 0x18];
    CRpM30* m_30; // +0x30
    char p34[0x48 - 0x34];
    CGruntzSoundZ* m_48; // +0x48  the zoned sound-bank manager
    char p4c[0x68 - 0x4c];
    CRpTimeline* m_68; // +0x68
    char p6c[0x7c - 0x6c];
    CRpWho7c* m_7c; // +0x7c
    char p80[0x134 - 0x80];
    i32 m_134; // +0x134  mode word
};
struct CRpFrame { // this->m_frameMarker (the frame-marker timeline)
    char p0[0x28];
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
};
struct CRpScroll { // this->m_4e4
    char p0[0x40];
    i32 m_40; // +0x40  drag/select flags (bit0 cleared)
};
struct CRpThis { // view-of-this
    char p0[0x4];
    CRpWorld* m_4; // +0x04
    char p8[0x1c - 0x8];
    i32 m_1c; // +0x1c  cue id
    char p20[0x338 - 0x20];
    i32 m_ambientTimerLo;    // +0x338  ambient timer base lo
    i32 m_ambientTimerHi;    // +0x33c  base hi
    i32 m_ambientInterval;   // +0x340  interval lo
    i32 m_ambientIntervalHi; // +0x344  interval hi
    i32 m_ambientInitDone;   // +0x348  ambient-init done
    char p34c[0x3f4 - 0x34c];
    CRpFrame* m_frameMarker; // +0x3f4
    char p3f8[0x4e4 - 0x3f8];
    CRpScroll* m_4e4; // +0x4e4
    char p4e8[0x4f4 - 0x4e8];
    i32 m_winLoseBanner; // +0x4f4  win/lose banner
    i32 m_inGame;        // +0x4f8  in-game
};
// reg = g_64556c view.
struct CRpReg58 {
    void CueA(i32 id);       // 0x1c53 thunk __thiscall(id)
    void CueB(i32 a, i32 b); // 0x2d97 thunk __thiscall(a, b)
};
struct CRpReg44 {
    char p0[0x124];
    i32 m_124; // +0x124
};
struct CRpRow {   // reg + 0x188 + i*0x238 (the per-slot config row)
    void Reset(); // 0x1055 thunk __thiscall
};
struct CRpSlot {
    char p0[0x220];
    i32 m_220; // +0x220
    i32 m_224; // +0x224
    char p228[0x238 - 0x228];
};
struct CRpReg {
    char p0[0x14];
    i32 m_14; // +0x14
    char p18[0x44 - 0x18];
    CRpReg44* m_44; // +0x44
    char p48[0x58 - 0x48];
    CRpReg58* m_58; // +0x58
    char p5c[0xc8 - 0x5c];
    i32* m_c8; // +0xc8  (reads m_c8[-2])
    char pcc[0x134 - 0xcc];
    i32 m_134; // +0x134
    char p138[0x150 - 0x138];
    CRpSlot m_slots[1]; // +0x150  stride 0x238
};

// @early-stop
// ~92% regalloc-coloring cascade (large fn): every branch + call + store is
// byte-faithful and all relocs pair, but MSVC5 colors several reloaded base
// pointers (m_4/reg + the cue-block id) into different registers than retail,
// plus a couple of store-scheduling/tail-merge offsets in the frame-marker
// block. Not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x000d60b0, 0x2cd)
i32 CPlay::ResetPlayState() {
    CRpThis* self = (CRpThis*)this;
    char buf[0x40];
    if (self->m_4->m_14 != 0 && ((CRpReg*)g_64556c)->m_134 == 1) {
        self->m_ambientInterval = 0x1f40;
        self->m_ambientIntervalHi = 0;
        self->m_ambientTimerLo = g_645588;
        self->m_ambientTimerHi = 0;
        wsprintfA(buf, "INTRO%d", GetAmbientId());
        if (((CRpReg*)g_64556c)->m_14 != 0) {
            self->m_4->m_48->PlayByName(buf, 0);
        }
        self->m_ambientInitDone = 0;
    } else {
        wsprintfA(buf, "AMBIENT%d", GetAmbientId());
        CGruntzSoundZ* snd = self->m_4->m_48;
        CGruntzSoundInnerZ* h = snd->FindBank(buf);
        if (h != 0) {
            snd->m_pCurrent = h;
        }
        if (self->m_4->m_48->m_pCurrent != 0) {
            self->m_4->m_48->m_pCurrent->SetLoop(1);
        }
        CRpReg* reg = (CRpReg*)g_64556c;
        if (reg->m_14 != 0 && reg->m_134 == 3) {
            self->m_4->m_48->PlayByName(buf, 1);
        }
        self->m_ambientTimerLo = 0;
        self->m_ambientInterval = 0;
        self->m_ambientTimerHi = 0;
        self->m_ambientIntervalHi = 0;
        self->m_ambientInitDone = 1;
    }
    if (self->m_4->m_134 == 1) {
        CRpReg* reg = (CRpReg*)g_64556c;
        if (reg->m_c8[-2] == 0) {
            self->m_4->m_7c->Notify(self->m_1c, 1);
            reg = (CRpReg*)g_64556c;
            if (reg->m_44->m_124 == 0) {
                i32 id = self->m_1c;
                if (id > 0x24 || id == 1) {
                    reg->m_58->CueA(id);
                    reg = (CRpReg*)g_64556c;
                }
            }
            reg->m_58->CueB(0, 0x81a6);
        }
        CRpGeom* g = self->m_4->m_30->m_24;
        ResetGoalGeom(g->m_3b0, g->m_3b4);
    } else {
        CRpSlot* slot = &((CRpReg*)g_64556c)->m_slots[g_644c54];
        if (slot != 0) {
            ResetGoalGeom(slot->m_220, slot->m_224);
        } else {
            CRpGeom* g = self->m_4->m_30->m_24;
            ResetGoalGeom(g->m_3b0, g->m_3b4);
        }
    }
    if (self->m_4e4 != 0) {
        self->m_4e4->m_40 &= ~1;
    }
    self->m_inGame = 0;
    if (!PrepareReset()) {
        return 0;
    }
    for (i32 off = 0; off < 0x8e0; off += 0x238) {
        ((CRpRow*)((char*)g_64556c + 0x188 + off))->Reset();
    }
    self->m_winLoseBanner = 0;
    CRpFrame* fm = self->m_frameMarker;
    if (fm != 0) {
        fm->m_40 = -1;
        fm->m_44 = 0;
        if (fm->m_4c != 0) {
            fm->m_38 = g_645588;
            fm->m_3c = 0;
            fm->m_30 = fm->m_4c;
            fm->m_34 = 0;
            fm->m_28 = g_645588;
            fm->m_2c = 0;
            fm->m_48 = 1;
        } else {
            fm->m_38 = g_645588;
            fm->m_3c = 0;
        }
    }
    CRpTimeline* tl = self->m_4->m_68;
    tl->m_2a4 = 1;
    tl->m_288 = 0;
    tl->m_2a8 = 0;
    tl->m_2b0 = 0;
    tl->m_2b8 = 0;
    tl->m_2b4 = 0;
    tl->m_2bc = 0;
    tl->m_2c0 = 0;
    tl->m_2c8 = 0;
    tl->m_2c4 = 0;
    tl->m_2cc = 0;
    tl->m_3ec = 0;
    tl->m_3f8 = 0;
    tl->m_3fc = 0;
    tl->m_400 = 1;
    return 1;
}

// FreeListTeardown (0x0cb480) - the per-level teardown OnExit runs: flush the
// world timeline, reset the resource sub-registries / world sub-objects, release
// every per-level allocation (the m_startMarkers/m_3a4[4]/m_488 pointer arrays) back onto
// the global node free list, then reset the per-grunt-type config rows.
// __thiscall, no args, no return. Self-contained view.
DATA(0x00245544)
extern void* g_freeList;
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

struct CRtArr {                    // a CPtrArray subset (m_data @+4, m_count @+8); 0x14 stride
    void SetSize(i32 n, i32 grow); // 0x1b4f75 __thiscall(n, grow)
    char p0[0x4];
    void** m_data; // +0x4
    i32 m_count;   // +0x8
    char pc[0x14 - 0xc];
};
struct CRtArr2 {                   // the m_68+0x260 array variant
    void SetSize(i32 n, i32 grow); // 0x1b52e8 __thiscall(n, grow)
};
struct CRtRow {    // m_4 + 0x188 + i*0x238  (per-grunt-type config row)
    void ResetA(); // 0x29a5 thunk
    void ResetB(); // 0x40c5 thunk
};
struct CRtTimeline {      // m_4->m_68 (also g_64556c->m_68)
    void Flush(i32 mode); // 0x41b0 thunk(mode)
    void Reset1514();     // 0x1514 thunk
    void Reset15c3();     // 0x15c3 thunk (reg->m_68 reset)
    void Reset1b48a6();   // 0x1b48a6 thunk
    char p0[0x260];
    CRtArr2 m_260; // +0x260
    char p264[0x284 - 0x264];
    i32 m_284; // +0x284
    char p288[0x2a0 - 0x288];
    i32 m_2a0; // +0x2a0
};
struct CRtSound {       // m_4->m_48
    void Reset138530(); // 0x138530 thunk
};
struct CRtWorldDraw { // m_4->m_54
    void Reset28ab(); // 0x28ab thunk
};
struct CRtSub60 {     // m_4->m_60
    void Reset244b(); // 0x244b thunk
};
struct CRtWorld { // this->m_4
    char p0[0x48];
    CRtSound* m_48; // +0x48
    char p4c[0x54 - 0x4c];
    CRtWorldDraw* m_54; // +0x54
    char p58[0x60 - 0x58];
    CRtSub60* m_60; // +0x60
    char p64[0x68 - 0x64];
    CRtTimeline* m_68; // +0x68
};
struct CRtImageReg { // m_c->m_24 (virtual; the +0x44 slot 17 teardown)
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void Teardown(); // slot 17 (+0x44)
};
struct CRtSoundReg2c {  // m_c->m_28->m_2c
    void Reset137a80(); // 0x137a80 thunk
};
struct CRtSoundReg { // m_c->m_28
    char p0[0x2c];
    CRtSoundReg2c* m_2c; // +0x2c
};
struct CRtRendererA {   // m_c->m_8
    void Reset15aa90(); // 0x15aa90 thunk
};
struct CRtRendererB {   // m_c->m_c
    void Reset163c60(); // 0x163c60 thunk
};
struct CRtResMgr { // this->m_c
    char p0[0x8];
    CRtRendererA* m_8; // +0x08
    CRtRendererB* m_c; // +0x0c
    char p10[0x24 - 0x10];
    CRtImageReg* m_24; // +0x24
    CRtSoundReg* m_28; // +0x28
};
struct CRtGuts {          // this->m_guts
    void Guts12fd(i32 a); // 0x12fd thunk(a)
};
struct CRtMarker {     // a no-arg-reset leaf (begin/frame markers)
    void ResetBegin(); // 0x1d7f thunk (this->m_beginMarker)
    void ResetFrame(); // 0x14ce thunk (this->m_frameMarker)
};
struct CRtReg { // g_64556c (only its +0x68 timeline is touched here)
    char p0[0x68];
    CRtTimeline* m_68; // +0x68
};
struct CRtThis { // view-of-this
    char p0[0x4];
    CRtWorld* m_4; // +0x04
    char p8[0xc - 0x8];
    CRtResMgr* m_c; // +0x0c
    char p10[0x2dc - 0x10];
    CRtGuts* m_guts; // +0x2dc  guts subsystem
    char p2e0[0x2e4 - 0x2e0];
    CRtMarker* m_beginMarker; // +0x2e4  begin marker
    char p2e8[0x370 - 0x2e8];
    CRtArr m_startMarkers; // +0x370  (m_markerData data / m_markerCount count)
    char p384[0x3a4 - 0x384];
    CRtArr m_3a4[4];          // +0x3a4  stride 0x14
    CRtMarker* m_frameMarker; // +0x3f4  frame marker
    char p3f8[0x488 - 0x3f8];
    CRtArr m_488; // +0x488  (m_48c data / m_490 count)
    i32 m_49c;    // +0x49c
    char p4a0[0x4e4 - 0x4a0];
    i32 m_4e4; // +0x4e4
};

// @early-stop
// ~99% register-coloring plateau: logic + all relocs pair; the only residual is
// MSVC5 coloring the reloaded m_4/m_c base pointers into eax/edx/ecx differently
// than retail across the six teardown-call setups. Not source-steerable.
// docs/patterns/zero-register-pinning.md.
RVA(0x000cb480, 0x22c)
void CPlay::FreeListTeardown() {
    CRtThis* self = (CRtThis*)this;
    i32 i;
    i32 k;
    if (self->m_c == 0) {
        return;
    }
    if (self->m_4 == 0) {
        return;
    }
    if (self->m_4->m_68 != 0) {
        self->m_4->m_68->Flush(5);
    }
    Teardown1780();
    if (self->m_c->m_28->m_2c != 0) {
        self->m_c->m_28->m_2c->Reset137a80();
    }
    self->m_4->m_48->Reset138530();
    self->m_4->m_54->Reset28ab();
    self->m_4->m_60->Reset244b();
    ((CRtReg*)g_64556c)->m_68->Reset15c3();
    self->m_c->m_24->Teardown();
    self->m_c->m_8->Reset15aa90();
    if (self->m_guts != 0) {
        self->m_guts->Guts12fd(0);
    }
    if (self->m_beginMarker != 0) {
        self->m_beginMarker->ResetBegin();
    }
    if (self->m_frameMarker != 0) {
        self->m_frameMarker->ResetFrame();
    }
    self->m_4e4 = 0;
    self->m_4->m_68->Reset1514();
    CRtTimeline* tl68 = self->m_4->m_68;
    tl68->m_260.SetSize(0, -1);
    tl68->m_284 = 0;
    self->m_4->m_68->Reset1b48a6();
    self->m_4->m_68->m_2a0 = 0;
    self->m_c->m_c->Reset163c60();
    for (i = 0; i < self->m_startMarkers.m_count; i++) {
        void* node = self->m_startMarkers.m_data[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_freeListNodeBias);
            *p = g_freeList;
            g_freeList = p;
        }
    }
    self->m_startMarkers.SetSize(0, -1);
    for (k = 0; k < 4; k++) {
        for (i = 0; i < self->m_3a4[k].m_count; i++) {
            void* node = self->m_3a4[k].m_data[i];
            if (node != 0) {
                void** p = (void**)((char*)node - g_freeListNodeBias);
                *p = g_freeList;
                g_freeList = p;
            }
        }
        self->m_3a4[k].SetSize(0, -1);
    }
    for (i = 0; i < self->m_488.m_count; i++) {
        void* node = self->m_488.m_data[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_freeListNodeBias);
            *p = g_freeList;
            g_freeList = p;
        }
    }
    self->m_488.SetSize(0, -1);
    for (i32 off = 0; off < 0x8e0; off += 0x238) {
        ((CRtRow*)((char*)self->m_4 + 0x188 + off))->ResetA();
        ((CRtRow*)((char*)self->m_4 + 0x188 + off))->ResetB();
    }
    self->m_49c = -1;
}

// ---------------------------------------------------------------------------
// CPlayDtorBody (0x0c8700): the ~CPlay teardown body. Frees the per-frame
// workers, clears the four g_mgrSettings config rows, flushes the free-list
// arrays, then chains the base (CState) dtor. Same free-list idiom as
// FreeListTeardown (the m_markerData/m_3a4[4]/m_48c flush).
// ---------------------------------------------------------------------------
struct DtorWorkerA {
    void Dtor(); // 0x10be thunk  (m_320)
};
struct DtorWorkerB {
    void Dtor(); // 0x1438 thunk  (m_guts)
};
struct DtorWorkerC {
    void Dtor(); // 0x285b thunk  (m_hitTest)
};
struct DtorWorkerD {
    void Dtor(); // 0x1cad thunk  (m_beginMarker)
};
struct DtorWorkerE {
    void Dtor(); // 0x14ce thunk  (m_frameMarker)
};
struct DtorObList {
    void Dtor(); // 0x1b9c69 thunk  (m_4 + 0xc8 CObList)
};
struct DtorSub5c {
    void Dtor(); // 0x128a thunk  (m_4->m_5c)
};
struct DtorWorld { // this->m_4
    char p0[0x5c];
    DtorSub5c* m_5c; // +0x5c
    char p60[0x128 - 0x60];
    i32 m_128; // +0x128
};

struct CDtorThis {
    // Slot +0x80 (index 32) is the only virtual dispatched; declare the leading
    // 32 slots so `mov eax,[this]; call [eax+0x80]` falls out as __thiscall.
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21();
    virtual void v22();
    virtual void v23();
    virtual void v24();
    virtual void v25();
    virtual void v26();
    virtual void v27();
    virtual void v28();
    virtual void v29();
    virtual void v30();
    virtual void v31();
    virtual void Vfunc80(); // +0x80
    void BaseDtor();        // 0x3f53 thunk  (base CState dtor)

    char p0[0x4];
    DtorWorld* m_4; // +0x04
    char p8[0x1d0 - 0x8];
    i32 m_1d0; // +0x1d0
    char p1d4[0x2dc - 0x1d4];
    DtorWorkerB* m_guts;        // +0x2dc
    DtorWorkerC* m_hitTest;     // +0x2e0
    DtorWorkerD* m_beginMarker; // +0x2e4
    char p2e8[0x320 - 0x2e8];
    DtorWorkerA* m_320; // +0x320
    char p324[0x370 - 0x324];
    CRtArr m_startMarkers; // +0x370
    char p384[0x3a4 - 0x384];
    CRtArr m_3a4[4];            // +0x3a4
    DtorWorkerE* m_frameMarker; // +0x3f4
    char p3f8[0x488 - 0x3f8];
    CRtArr m_488; // +0x488
    i32 m_49c;    // +0x49c
};

// @early-stop
// hard-regalloc wall: ebp pinned to the zero-const + the cached free-list head
// in edx across the m_markerData/m_3a4/m_48c flush loops are not source-steerable
// (same coloring plateau as FreeListTeardown 0xcb480, ~99%).
RVA(0x000c8700, 0x1f4)
void CPlay::CPlayDtorBody() {
    CDtorThis* self = (CDtorThis*)this;
    i32 i;
    if (self->m_320) {
        self->m_320->Dtor();
        ::operator delete(self->m_320);
        self->m_320 = 0;
    }
    self->Vfunc80();
    if (self->m_4) {
        self->m_4->m_128 = 0;
        ((DtorObList*)((char*)self->m_4 + 0xc8))->Dtor();
    }
    self->m_1d0 = 0;
    i32 off = 0;
    do {
        off += 0x238;
        *(i32*)((char*)g_64556c + off - 0xc8) = 0;
    } while (off < 0x8e0);
    if (self->m_4 && self->m_4->m_5c) {
        self->m_4->m_5c->Dtor();
    }
    if (self->m_guts) {
        self->m_guts->Dtor();
        ::operator delete(self->m_guts);
        self->m_guts = 0;
    }
    if (self->m_hitTest) {
        self->m_hitTest->Dtor();
        ::operator delete(self->m_hitTest);
        self->m_hitTest = 0;
    }
    if (self->m_beginMarker) {
        self->m_beginMarker->Dtor();
        ::operator delete(self->m_beginMarker);
        self->m_beginMarker = 0;
    }
    if (self->m_frameMarker) {
        self->m_frameMarker->Dtor();
        ::operator delete(self->m_frameMarker);
        self->m_frameMarker = 0;
    }
    for (i = 0; i < self->m_startMarkers.m_count; i++) {
        void* node = self->m_startMarkers.m_data[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_freeListNodeBias);
            *p = g_freeList;
            g_freeList = p;
        }
    }
    self->m_startMarkers.SetSize(0, -1);
    for (i32 k = 0; k < 4; k++) {
        for (i = 0; i < self->m_3a4[k].m_count; i++) {
            void* node = self->m_3a4[k].m_data[i];
            if (node != 0) {
                void** p = (void**)((char*)node - g_freeListNodeBias);
                *p = g_freeList;
                g_freeList = p;
            }
        }
        self->m_3a4[k].SetSize(0, -1);
    }
    for (i = 0; i < self->m_488.m_count; i++) {
        void* node = self->m_488.m_data[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_freeListNodeBias);
            *p = g_freeList;
            g_freeList = p;
        }
    }
    self->m_49c = -1;
    self->m_488.SetSize(0, -1);
    self->BaseDtor();
}

// ---------------------------------------------------------------------------
// EnterMode (0x0d6fa0): the mode-enter gate. Pause the guts + world, optionally
// run the deferred draw, (re)install the renderer view, then re-arm the guts and
// (mode 9) latch the saved game clock. A dense chain of CPlay/registry thunks.
// ---------------------------------------------------------------------------
struct EmGuts {      // this->m_guts
    void Pause();    // 0x125d
    void StepZ(i32); // 0x34bd
    void Resume();   // 0x21b7
};
struct EmRegN {    // g_64556c
    void Notify(); // 0x12ee
};
struct EmHdr2c {      // m_c->m_4->m_14->m_2c
    void Recede(i32); // 0x13e760
};
struct EmHdr14 {
    char p0[0x2c];
    EmHdr2c* m_2c; // +0x2c
};
struct EmCWorld {            // m_c->m_4
    i32 Sub158d20();         // 0x158d20
    i32 Sub158cb0(i32, i32); // 0x158cb0
    void Sub158e90();        // 0x158e90
    char p0[0x14];
    EmHdr14* m_14; // +0x14
    i32 m_18;      // +0x18
};
struct EmRendVtbl {
    void* s[0x34 / 4];
    void (*Present)(void*, EmHdr14*, i32);
};
struct EmRendC { // m_c->m_c
    EmRendVtbl* vtbl;
};
struct EmReg24Sub {                  // m_c->m_24
    void BuildView(EmHdr14*, void*); // 0x15dc90
    char p0[0x5c];
    void* m_5c; // +0x5c
};
struct EmResMgr { // this->m_c
    char p0[0x4];
    EmCWorld* m_4; // +0x04
    void* m_8;     // +0x08
    EmRendC* m_c;  // +0x0c
    char p10[0x24 - 0x10];
    EmReg24Sub* m_24; // +0x24
};
struct EmWorld {    // this->m_4
    void Refresh(); // 0x3d23
    char p0[0x10];
    i32 m_10; // +0x10
    char p14[0x54 - 0x14];
    void* m_54; // +0x54
};
struct EmSink5c {  // m_c->m_24->m_5c
    void Notify(); // 0x163370
};
struct EmSub54 {  // m_4->m_54
    void Reset(); // 0x18e8
};

struct EmThis {
    void DeferredDraw();               // 0x1ae6  (this)
    void ArmTimer(i32, i32, i32, i32); // 0x1843  (this)
    void FinishMode();                 // 0x3a71  (this)

    char p0[0x4];
    EmWorld* m_4; // +0x04
    char p8[0xc - 0x8];
    EmResMgr* m_c; // +0x0c
    char p10[0x1a8 - 0x10];
    i32 m_inputWarmup1, m_inputWarmup2, m_inputHalfSel; // +0x1a8..
    char p1b4[0x1c4 - 0x1b4];
    i32 m_1c4; // +0x1c4
    char p1c8[0x1cc - 0x1c8];
    i32 m_savedClock; // +0x1cc
    char p1d0[0x2dc - 0x1d0];
    EmGuts* m_guts; // +0x2dc
    char p2e0[0x470 - 0x2e0];
    i32 m_470; // +0x470
    i32 m_474; // +0x474
    char p478[0x484 - 0x478];
    i32 m_484; // +0x484
};

extern "C" i32 g_645588_clk; // 0x645588 (game clock latch)

// Reuse the registry's external WorldSubstep thunk (0x2356).
void EmRegWorldStep(EmRegN* reg, EmGuts* guts, i32 a); // 0x2356  cdecl(reg, guts, a)

// @early-stop
// large state-machine wall: the mode dispatch + renderer reinstall + guts re-arm
// are faithful, but the whole ILT-thunk referent set (~15 unnamed CPlay/registry
// leaves) keeps it reloc-fuzzy; codegen plateau, not source-steerable.
RVA(0x000d6fa0, 0x1fa)
i32 CPlay::EnterMode(i32 mode) {
    EmThis* self = (EmThis*)this;
    ((EmRegN*)g_64556c)->Notify();
    self->m_guts->Pause();
    self->m_guts->StepZ(0);
    self->m_4->Refresh();

    if (self->m_1c4 != 0) {
        self->m_1c4 = 0;
        self->m_c->m_4->m_14->m_2c->Recede(0);
        EmRegWorldStep((EmRegN*)g_64556c, self->m_guts, self->m_470);
        if (self->m_474 != 0) {
            self->DeferredDraw();
        } else {
            self->m_c->m_24->BuildView(self->m_c->m_4->m_14, self->m_c->m_8);
            self->m_c->m_c->vtbl
                ->Present(self->m_c->m_c, self->m_c->m_4->m_14, self->m_c->m_4->m_18);
        }
        self->m_guts->Pause();
        self->m_guts->Resume();
    } else {
        if (self->m_474 != 0) {
            self->DeferredDraw();
        } else {
            self->m_c->m_24->BuildView(self->m_c->m_4->m_14, self->m_c->m_8);
            self->m_c->m_c->vtbl
                ->Present(self->m_c->m_c, self->m_c->m_4->m_14, self->m_c->m_4->m_18);
        }
        self->m_guts->Pause();
        self->m_guts->Resume();
        if (mode == 9) {
            if (self->m_c->m_4->Sub158d20() != 0) {
                goto finish;
            }
            if (self->m_c->m_4->Sub158cb0(0, 0x30000) != 0) {
                goto finish;
            }
            return 0;
        }
        self->m_c->m_4->m_14->m_2c->Recede(0);
    }

finish:
    self->m_c->m_4->Sub158e90();
    self->ArmTimer(0x50, 0x3e8, 0, 1);
    if (self->m_c->m_24->m_5c != 0) {
        ((EmSink5c*)self->m_c->m_24->m_5c)->Notify();
    }
    self->m_4->Refresh();
    self->m_inputWarmup1 = 0;
    self->m_inputWarmup2 = 0;
    self->m_inputHalfSel = 0;
    if (self->m_4->m_10 != 0 && mode != 9) {
        ((EmSub54*)self->m_4->m_54)->Reset();
    }
    if (mode == 9) {
        g_645588_clk = self->m_savedClock;
    }
    self->m_guts->Pause();
    self->FinishMode();
    self->m_484 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// AddLevelGruntz (0x0d5960): walk the registry's object list; for each valid
// grunt object (vtable-id 0x4024a5, not the placeholder m_124) register it with
// the session via the world's +0x68 sink. On a -1 failure, format + log the
// "Could not add Grunt: Player=%d" message (a CString temp -> /GX frame).
// ---------------------------------------------------------------------------
struct AgGrunt { // node->m_8
    char p0[0x8];
    u32 m_08; // +0x08  flag bits
    char p0c[0x5c - 0xc];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    char p64[0x7c - 0x64];
    i32* m_7c; // +0x7c  type record (m_7c[4]==id, m_7c[0xb]/[0xc] params)
    char p80[0x114 - 0x80];
    i32 m_114; // +0x114
    i32 m_118; // +0x118
    i32 m_11c; // +0x11c
    i32 m_120; // +0x120
    i32 m_124; // +0x124  placeholder/type token
    i32 m_128; // +0x128
    i32 m_12c; // +0x12c
    char p130[0x134 - 0x130];
    i32 m_134; // +0x134
};
struct AgNode {
    AgNode* m_next; // +0x00
    char p4[0x8 - 0x4];
    AgGrunt* m_8; // +0x08
};
struct AgListHdr {
    char p0[0x4];
    AgNode* m_head; // +0x04
};
struct AgWorldSink { // this->m_4->m_68
    // 0x40bb: register one grunt (13 args). __thiscall.
    i32 AddGrunt(
        i32 token,
        i32 x,
        i32 y,
        i32 cap,
        i32 z,
        i32 a,
        i32 b,
        i32 c,
        i32 d,
        i32 cfg2,
        i32 recA,
        i32 recB,
        i32* rec
    );
};
struct AgWorld { // this->m_4
    char p0[0x68];
    AgWorldSink* m_68; // +0x68
};
struct AgResMgr { // this->m_c
    char p0[0x8];
    void* m_8; // +0x08  -> list owner (+0x10 = embedded list)
};
struct AgThis {
    char p0[0x4];
    AgWorld* m_4; // +0x04
    char p8[0xc - 0x8];
    AgResMgr* m_c; // +0x0c
};

extern i32 g_644c54;                             // 0x644c54 placeholder token
void AgFormat(CString* s, const char* fmt, ...); // 0x1b2cf5 CString::Format
void AgLog(CGameRegistry* reg, const char* msg); // 0x417e

// @early-stop
// /GX list-walk wall: the registration loop + CString error log are faithful, but
// the 13-arg AddGrunt + CString-temp EH frame keep it reloc-fuzzy.
RVA(0x000d5960, 0x160)
i32 CPlay::AddLevelGruntz() {
    AgThis* self = (AgThis*)this;
    AgListHdr* lst = (AgListHdr*)((char*)self->m_c->m_8 + 0x10);
    AgNode* node = lst->m_head;
    while (node != 0) {
        AgGrunt* g = node->m_8;
        node = node->m_next;
        if (g == 0) {
            continue;
        }
        if (g->m_7c[4] != 0x4024a5) {
            continue;
        }
        if (g->m_124 == g_644c54) {
            continue;
        }
        i32 x = ((g->m_5c & ~0x1f) + 0x10);
        i32 y = ((g->m_60 & ~0x1f) + 0x10);
        i32 r = self->m_4->m_68->AddGrunt(
            g->m_124,
            y,
            x,
            0x186a0,
            0,
            g->m_114,
            g->m_11c,
            g->m_120,
            g->m_118,
            g->m_12c,
            g->m_7c[0xb],
            g->m_7c[0xc],
            &g->m_134
        );
        if (r == -1) {
            CString msg;
            AgFormat(&msg, "Could not add Grunt: Player=%d", g->m_124, y, x);
            AgLog(g_64556c, msg); // CString -> LPCTSTR (implicit)
            return 0;
        }
        g->m_08 |= 0x10000;
    }
    return 1;
}

// BuildGruntNamespaceList (0x0dd050) - register the seven core grunt asset
// namespaces (NORMALGRUNT/DEATHZ/ENTRANCEZ/EXITZ/GRUNTPUDDLE/PICKUPS/BOMBGRUNT)
// in order; bail (return 0) on the first that fails to bind. The CString name temp
// forces the /GX EH frame. __thiscall(arg), ret 4.
RVA(0x000dd050, 0x24b)
i32 CPlay::BuildGruntNamespaceList(i32 arg) {
    CString s;
    s = "NORMALGRUNT";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    s = "DEATHZ";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    s = "ENTRANCEZ";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    s = "EXITZ";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    s = "GRUNTPUDDLE";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    s = "PICKUPS";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    s = "BOMBGRUNT";
    if (!RegisterNamespace(s, 1, 0, arg)) {
        return 0;
    }
    return 1;
}

// BuildWarlordNameTable (0x0dd340) - verify warlord ids 2..0x20 are present, then
// the two boss ids 0x39/0x3a, then bind the three named warlord sprite sets
// (NAPOLEAN/VIKING/PATTON). The CString name temp forces the /GX EH frame.
// __thiscall(arg), ret 4.
RVA(0x000dd340, 0x189)
i32 CPlay::BuildWarlordNameTable(i32 arg) {
    for (i32 id = 2; id <= 0x20; id++) {
        if (!ProbeWarlord(id, 0, 0, 0)) {
            return 0;
        }
    }
    if (!ProbeWarlord(0x39, 0, 0, arg)) {
        return 0;
    }
    if (!ProbeWarlord(0x3a, 0, 0, arg)) {
        return 0;
    }
    CString s("WARLORDZ_NAPOLEAN");
    if (!BindWarlordName(s, 0, 0, arg)) {
        return 0;
    }
    s = "WARLORDZ_VIKING";
    if (!BindWarlordName(s, 0, 0, arg)) {
        return 0;
    }
    s = "WARLORDZ_PATTON";
    if (!BindWarlordName(s, 0, 0, arg)) {
        return 0;
    }
    return 1;
}

// The four placed-object dynamic-type markers LoadWarlordSprites tests obj->m_7c->m_10
// against: each is the address of that object class's vtable slot-4 (+0x10) method
// thunk. Named so the `cmp eax,<thunk>` emits its DIR32 reloc (retail relocates the
// immediate) instead of a bare number - the reloc is what the match needs.
DATA(0x000024a5)
extern char g_objVtblThunk_24a5[]; // "multi-sprite warlord" object (m_11c/m_120 + m_118 switch)
DATA(0x0000288d)
extern char g_objVtblThunk_288d[]; // counted object keyed on m_124
DATA(0x00003d0f)
extern char g_objVtblThunk_3d0f[]; // counted object keyed on m_11c
DATA(0x0000137a)
extern char g_objVtblThunk_137a[]; // counted object keyed on m_11c (sibling of 3d0f)

// A placed object walked in the in-level branch: its +0x7c dynamic-type vtable
// (whose +0x10 slot carries the type marker) + its sprite/type ids.
SIZE_UNKNOWN(CWarlordVtbl);
struct CWarlordVtbl {
    char p0[0x10];
    void* m_10; // +0x10  dynamic-type marker (a vtable slot method thunk)
};
SIZE_UNKNOWN(CWarlordObj);
struct CWarlordObj {
    char p0[0x7c];
    CWarlordVtbl* m_7c; // +0x7c
    char p80[0x118 - 0x80];
    i32 m_118; // +0x118  primary sprite/type id
    i32 m_11c; // +0x11c
    i32 m_120; // +0x120
    i32 m_124; // +0x124
};
SIZE_UNKNOWN(CWarlordListNode);
struct CWarlordListNode {
    CWarlordListNode* m_0; // +0x00  next
    char p4[0x8 - 0x4];
    CWarlordObj* m_8; // +0x08  object
};

// LoadWarlordSprites (0x0d65d0) - ensure every sprite set a placed warlord needs is
// loaded. Two modes: the full campaign preload (registry m_134 != 1) loads sets
// 2..0x20 + 0x39/0x3a + the three named warlord banks; the in-level mode walks the
// placed-object display list (renderer A's m_10) and, per object type (its vtable
// marker), loads the sets that object uses + bumps the world's per-kind counters
// (m_4->m_7c). `loaded[]` guards each set so its progress tick fires once. Reuses
// ProbeWarlord (0x12da) + BindWarlordName (0x2bc1), like BuildWarlordNameTable above.
// @early-stop  (0.x% -> 91.06%)
// EH/frame wall: retail reuses the dead incoming-arg slots for the CString `s` (no
// `sub esp` for locals) while the recompile allocates a fresh frame, so the
// cleanup/dtor tail shifts + a few esi/edx/ecx spill recolors. Code shape + every
// data/marker reloc match.
RVA(0x000d65d0, 0x7a4)
i32 CPlay::LoadWarlordSprites(i32 ctx, i32* loaded) {
    if (g_64556c->m_134 != 1) {
        for (i32 id = 2; id <= 0x20; id++) {
            if (loaded[id] == 0) {
                WarlordLoadTick(0);
                loaded[id] = 1;
            }
            if (!ProbeWarlord(id, 1, 0, ctx)) {
                return 0;
            }
        }
        if (!ProbeWarlord(0x39, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x21] == 0) {
            WarlordLoadTick(0);
            loaded[0x21] = 1;
        }
        if (!ProbeWarlord(0x3a, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x22] == 0) {
            WarlordLoadTick(0);
            loaded[0x22] = 1;
        }
        CString s("WARLORDZ_NAPOLEAN");
        if (!BindWarlordName(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x23] == 0) {
            WarlordLoadTick(0);
            loaded[0x23] = 1;
        }
        s = "WARLORDZ_VIKING";
        if (!BindWarlordName(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x24] == 0) {
            WarlordLoadTick(0);
            loaded[0x24] = 1;
        }
        s = "WARLORDZ_PATTON";
        if (!BindWarlordName(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x25] == 0) {
            WarlordLoadTick(0);
            loaded[0x25] = 1;
        }
        return 1;
    }
    CWarlordListHead* head = &this->m_c->m_rendererA->m_10;
    if (!head) {
        return 0;
    }
    CWarlordListNode* node = head->m_4;
    while (node) {
        CWarlordObj* obj = node->m_8;
        CWarlordListNode* nxt = node->m_0;
        if (obj) {
            void* marker = obj->m_7c->m_10;
            if (marker == (void*)g_objVtblThunk_24a5) {
                i32 v = obj->m_11c;
                if (v) {
                    if (!ProbeWarlord(v, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        WarlordLoadTick(0);
                        loaded[v] = 1;
                    }
                }
                v = obj->m_120;
                if (v) {
                    if (!ProbeWarlord(v, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        WarlordLoadTick(0);
                        loaded[v] = 1;
                    }
                }
                switch (obj->m_118) {
                    case 0x7:
                        if (!ProbeWarlord(1, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[1] == 0) {
                            WarlordLoadTick(0);
                            loaded[1] = 1;
                        }
                        break;
                    case 0x8:
                        if (!ProbeWarlord(3, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[3] == 0) {
                            WarlordLoadTick(0);
                            loaded[3] = 1;
                        }
                        break;
                    case 0x9:
                        if (!ProbeWarlord(5, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[5] == 0) {
                            WarlordLoadTick(0);
                            loaded[5] = 1;
                        }
                        break;
                    case 0xa:
                        if (!ProbeWarlord(7, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[7] == 0) {
                            WarlordLoadTick(0);
                            loaded[7] = 1;
                        }
                        break;
                    case 0xb:
                        if (!ProbeWarlord(0xd, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0xd] == 0) {
                            WarlordLoadTick(0);
                            loaded[0xd] = 1;
                        }
                        break;
                    case 0xc:
                        if (!ProbeWarlord(0x11, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x11] == 0) {
                            WarlordLoadTick(0);
                            loaded[0x11] = 1;
                        }
                        break;
                    case 0xf:
                        if (!ProbeWarlord(0x13, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x13] == 0) {
                            WarlordLoadTick(0);
                            loaded[0x13] = 1;
                        }
                        break;
                    case 0x10:
                        if (!ProbeWarlord(0x1e, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x1e] == 0) {
                            WarlordLoadTick(0);
                            loaded[0x1e] = 1;
                        }
                        break;
                }
            } else if (marker == (void*)g_objVtblThunk_288d) {
                i32 cv = obj->m_124 == 0x32 ? obj->m_118 : obj->m_124;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_4w()->m_7c->m_34++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_4w()->m_7c->m_30++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_4w()->m_7c->m_38++;
                } else if (cv == 0x50) {
                    m_4w()->m_7c->m_40++;
                }
                i32 d = obj->m_124;
                if (d <= 0x20) {
                    if (!ProbeWarlord(d, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_124] == 0) {
                        WarlordLoadTick(0);
                        loaded[obj->m_124] = 1;
                    }
                } else if (d == 0x39) {
                    if (!ProbeWarlord(0x39, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        WarlordLoadTick(0);
                        loaded[0x21] = 1;
                    }
                } else if (d == 0x3a) {
                    if (!ProbeWarlord(0x3a, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        WarlordLoadTick(0);
                        loaded[0x22] = 1;
                    }
                } else if (d == 0x55 || d == 0x32) {
                    if (!ProbeWarlord(obj->m_118, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_118] == 0) {
                        WarlordLoadTick(0);
                        loaded[obj->m_118] = 1;
                    }
                }
            } else if (marker == (void*)g_objVtblThunk_3d0f
                       || marker == (void*)g_objVtblThunk_137a) {
                i32 cv = obj->m_11c == 0x32 ? obj->m_118 : obj->m_11c;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_4w()->m_7c->m_34++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_4w()->m_7c->m_30++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_4w()->m_7c->m_38++;
                } else if (cv == 0x50) {
                    m_4w()->m_7c->m_40++;
                }
                i32 e = obj->m_11c;
                if (e <= 0x20) {
                    if (!ProbeWarlord(e, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_11c] == 0) {
                        WarlordLoadTick(0);
                        loaded[obj->m_11c] = 1;
                    }
                } else if (obj->m_124 == 0x39) {
                    if (!ProbeWarlord(0x39, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        WarlordLoadTick(0);
                        loaded[0x21] = 1;
                    }
                } else if (obj->m_124 == 0x3a) {
                    if (!ProbeWarlord(0x3a, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        WarlordLoadTick(0);
                        loaded[0x22] = 1;
                    }
                } else if (e == 0x55 || e == 0x32) {
                    if (!ProbeWarlord(obj->m_118, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_118] == 0) {
                        WarlordLoadTick(0);
                        loaded[obj->m_118] = 1;
                    }
                }
            }
        }
        node = nxt;
    }
    return 1;
}

// SetEffectSpriteDurations (0x0dc060) - stamp the per-effect display duration
// (+0x18) onto each named effect-sound descriptor looked up by name in the sound
// registry's embedded CMapStringToOb. __thiscall, no args, ret 1. Self-contained
// view (an unrolled run of Lookup-then-store).
struct CEffDesc {
    char p0[0x18];
    i32 m_18; // +0x18  display duration (ms)
};
struct CEffMap {                           // m_c->m_28 + 0x10 (CMapStringToOb)
    i32 Lookup(char* key, CEffDesc** out); // 0x1b8438 __thiscall (ret 8)
};
struct CEffResMgr { // m_c->m_28
    char p0[0x10];
    CEffMap m_10; // +0x10  embedded name map
};
struct CEffMgr { // this->m_c
    char p0[0x28];
    CEffResMgr* m_28; // +0x28
};
struct CPlayEff { // view-of-this
    char p0[0xc];
    CEffMgr* m_c; // +0x0c
};

// @early-stop
// ~67% Lookup out-param zero-init scheduling wall (large unrolled fn): logic is
// complete and every name string + duration is byte-exact (all relocs pair), but
// MSVC5 permutes each block's `lea &out` / `out = 0` init / m_c->m_28 load vs
// retail (retail hoists the first `lea &out` into the prologue, shifting the
// out-slot operand 0xc<->0x10), repeating across all 32 blocks. Documented wall;
// deferred to the final sweep. docs/patterns/outparam-zeroinit-scheduling.md.
RVA(0x000dc060, 0x51b)
i32 CPlay::SetEffectSpriteDurations() {
    CPlayEff* self = (CPlayEff*)this;
    CEffDesc* d;
    d = 0;
    self->m_c->m_28->m_10.Lookup("GAME_PYRAMIDMOVE", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GAME_TELEPORTEROPEN", &d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GAME_TELEPORTERCLOSE", &d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GAME_TELEPORTERALL", &d);
    if (d != 0) {
        d->m_18 = 4000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GAME_BRICKBREAK", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_DEATHBRIDGEMOVE", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_WATERBRIDGEMOVE", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_ROCKBREAK", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_LAVAGEYSER", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_TRAPDOORCLOSE", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_TRAPDOOROPEN", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_CANDLEIGNITE", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_CANDLEUP", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_CANDLEDOWN", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_GOLFBALLAIR2", &d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_GOLFBALLHOLE", &d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_GOLFBALLSINK", &d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GAME_EXPLOSION1", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_OUTLETHAZARD", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE1A", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE2A", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_RESSURECT", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZSQUASH1A", &d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_CLOUDHAZARDMOVE", &d);
    if (d != 0) {
        d->m_18 = 10000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_CLOUDHAZARDKILL", &d);
    if (d != 0) {
        d->m_18 = 3000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A", &d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1", &d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1", &d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1", &d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    self->m_c->m_28->m_10.Lookup("LEVEL_PLANEHAZARDFLY", &d);
    if (d != 0) {
        d->m_18 = 5000;
    }
    return 1;
}

// The two low-RVA 5-byte ILT `jmp rel32` islands for CPlay::RegionEnter/RegionLeave
// (0x1b9a / 0x19f1) are linker-emitted incremental-link thunks, not game code; they
// are carved to config/library_labels.csv (asm-carveout) rather than reconstructed
// as naked jmp bodies.

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(AgGrunt);
SIZE_UNKNOWN(AgListHdr);
SIZE_UNKNOWN(AgNode);
SIZE_UNKNOWN(AgResMgr);
SIZE_UNKNOWN(AgThis);
SIZE_UNKNOWN(AgWorld);
SIZE_UNKNOWN(AgWorldSink);
SIZE_UNKNOWN(CAnimRegistry);
SIZE_UNKNOWN(CBankMgr);
SIZE_UNKNOWN(CCueState);
SIZE_UNKNOWN(CDrawSurface);
SIZE_UNKNOWN(CDtorThis);
SIZE_UNKNOWN(CEffDesc);
SIZE_UNKNOWN(CEffMap);
SIZE_UNKNOWN(CEffMgr);
SIZE_UNKNOWN(CEffResMgr);
SIZE_UNKNOWN(CExitV44);
SIZE_UNKNOWN(CExitV58);
SIZE_UNKNOWN(CExitView);
SIZE_UNKNOWN(CExitWorld);
SIZE_UNKNOWN(CHitMarker);
SIZE_UNKNOWN(CImageRegistry);
SIZE_UNKNOWN(CInputDispatch);
SIZE_UNKNOWN(CLoadNotify);
SIZE_UNKNOWN(CMusicCatTable);
SIZE_UNKNOWN(CMusicEntry);
SIZE_UNKNOWN(CMusicOwner);
SIZE_UNKNOWN(CMusicSet);
SIZE_UNKNOWN(CMusicSource);
SIZE_UNKNOWN(CPlayPlaneGeom);
SIZE_UNKNOWN(CPlay);
SIZE_UNKNOWN(CPlayEff);
SIZE_UNKNOWN(CProfFlush);
SIZE_UNKNOWN(CRegExit);
SIZE_UNKNOWN(CRegHitGate);
SIZE_UNKNOWN(CRegHitView);
SIZE_UNKNOWN(CRegSink);
SIZE_UNKNOWN(CRegSlot);
SIZE_UNKNOWN(CRegSub68);
SIZE_UNKNOWN(CRenderState);
SIZE_UNKNOWN(CRenderer);
SIZE_UNKNOWN(CResSource);
SIZE_UNKNOWN(CRpFrame);
SIZE_UNKNOWN(CRpGeom);
SIZE_UNKNOWN(CRpM30);
SIZE_UNKNOWN(CRpReg);
SIZE_UNKNOWN(CRpReg44);
SIZE_UNKNOWN(CRpReg58);
SIZE_UNKNOWN(CRpRow);
SIZE_UNKNOWN(CRpScroll);
SIZE_UNKNOWN(CRpSlot);
SIZE_UNKNOWN(CRpThis);
SIZE_UNKNOWN(CRpTimeline);
SIZE_UNKNOWN(CRpWho7c);
SIZE_UNKNOWN(CRpWorld);
SIZE_UNKNOWN(CRtArr);
SIZE_UNKNOWN(CRtArr2);
SIZE_UNKNOWN(CRtGuts);
SIZE_UNKNOWN(CRtImageReg);
SIZE_UNKNOWN(CRtMarker);
SIZE_UNKNOWN(CRtReg);
SIZE_UNKNOWN(CRtRendererA);
SIZE_UNKNOWN(CRtRendererB);
SIZE_UNKNOWN(CRtResMgr);
SIZE_UNKNOWN(CRtRow);
SIZE_UNKNOWN(CRtSound);
SIZE_UNKNOWN(CRtSoundReg);
SIZE_UNKNOWN(CRtSoundReg2c);
SIZE_UNKNOWN(CRtSub60);
SIZE_UNKNOWN(CRtThis);
SIZE_UNKNOWN(CRtTimeline);
SIZE_UNKNOWN(CRtWorld);
SIZE_UNKNOWN(CRtWorldDraw);
SIZE_UNKNOWN(CSoundRegistry);
SIZE_UNKNOWN(CState);
SIZE_UNKNOWN(CView);
SIZE_UNKNOWN(CVisEntity);
SIZE_UNKNOWN(CVisEntityType);
SIZE_UNKNOWN(CVisEntityVtbl);
SIZE_UNKNOWN(CVisNode);
SIZE_UNKNOWN(CWorld);
SIZE_UNKNOWN(CWorldDraw);
SIZE_UNKNOWN(CWorldLayer);
SIZE_UNKNOWN(CWorldSub60);
SIZE_UNKNOWN(DtorObList);
SIZE_UNKNOWN(DtorSub5c);
SIZE_UNKNOWN(DtorWorkerA);
SIZE_UNKNOWN(DtorWorkerB);
SIZE_UNKNOWN(DtorWorkerC);
SIZE_UNKNOWN(DtorWorkerD);
SIZE_UNKNOWN(DtorWorkerE);
SIZE_UNKNOWN(DtorWorld);
SIZE_UNKNOWN(Edge);
SIZE_UNKNOWN(EmCWorld);
SIZE_UNKNOWN(EmGuts);
SIZE_UNKNOWN(EmHdr14);
SIZE_UNKNOWN(EmHdr2c);
SIZE_UNKNOWN(EmReg24Sub);
SIZE_UNKNOWN(EmRegN);
SIZE_UNKNOWN(EmRendC);
SIZE_UNKNOWN(EmRendVtbl);
SIZE_UNKNOWN(EmResMgr);
SIZE_UNKNOWN(EmSink5c);
SIZE_UNKNOWN(EmSub54);
SIZE_UNKNOWN(EmThis);
SIZE_UNKNOWN(EmWorld);
