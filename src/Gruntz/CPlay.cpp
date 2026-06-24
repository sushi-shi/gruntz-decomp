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
//   this->vtbl[+0x7c](0, m_150, m_154);          // BeginFrameClear (a virtual)
//   m_drewThisFrame = 0;                                    // per-frame "drew" flag
//   if (m_renderDisabled) return 1;                          // hard early-out
//
//   if (m_inGame) {                  // ---- MAIN in-game frame ----
//       StepInputA(); StepWorldB(); ViewPreStep(m_c->m_24);
//       g_6bf3c0=g_645580; g_6bf3bc=g_645584;     // mirror the draw clock
//       DRAW_WORLD();                             // shared world-draw block
//       <AMBIENT-cue timer +0x3f8, 0x1f4ms, toggles m_cueToggle -> PlayCueAt 0x8128>
//       MarkerBegin(now); GutsStep();             // m_beginMarker marker + m_guts step
//       if (m_c->m_4->m_14 == 0) return 1;        // no view -> bail
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
//       <cursor profiler m_c->m_20: timeGetTime x2>;
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
//   if (m_c->m_20) { t=timeGetTime(); profiler(m_c->m_20,t) x2; }   // frame profiler
//   MarkerBegin(g_645584); GutsStep();                             // marker + guts
//
// The per-ENTITY layer is one indirection down: g_entityList is walked
// by the world object (m_4->m_54) inside the reloc-masked camera-blit call. The
// next targets are CPlay's own sub-steps + the world-draw helpers
// (push-view, surface-flush, camera blit, HUD).
// ============================================================================

#include <Gruntz/CPlay.h>
#include <rva.h>

// ---- MFC primitives reused verbatim from the engine (reloc-masked). ----
#include <Gruntz/CString.h>
extern i32 MapLookup(void* map, void* key, void*& out); // CMapPtrToPtr::Lookup

// ---- The global CButeMgr text-config tree (the singleton). Modeled as
//      a minimal class so PlayCueAt's `mov ecx,<singleton>; call GetInt`
//      reloc-masks against the already-matched GetInt (butemgr unit). --
#include <Bute/ButeMgr.h>
DATA(0x002453d8)
extern CButeMgr g_buteMgr;
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

// CPlay::Update() (slot 4): the PLAY state's ID = 3.
RVA(0x0008c910, 0x6)
i32 CPlay::Update() {
    return 3;
}

// ===========================================================================
// CPlay::Render  (vtable slot +0x14)
// ===========================================================================
RVA(0x000c8cf0, 0xc14)
i32 CPlay::Render() {
    // --- frame entry: clear the per-frame flag, then a `this`-virtual begin. ---
    // (the m_drewThisFrame=0 store is scheduled INTO the BeginFrameClear arg setup.)
    m_drewThisFrame = 0;
    BeginFrameClear(0, m_150, m_154); // this->vtbl[+0x7c](0, m_150, m_154)

    if (m_renderDisabled != 0) {
        return 1; // hard early-out
    }

    if (m_inGame != 0) {
        // =================================================================
        // ---- MAIN in-game frame ----
        // =================================================================
        StepInputA();         // poll/sim sub-step A
        StepWorldB();         // world/camera sub-step B
        m_c->m_24->PreStep(); // on m_c->m_24 (view pre-step)

        g_6bf3c0 = g_645580; // mirror the draw clock
        g_6bf3bc = g_645584;

        // --- shared world-draw block #1 ---
        m_c->m_8->BeginScene(0);                                     // m_c->m_8->vtbl[+0x24](0)
        m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);               // (thiscall on m_24)
        m_c->m_c->Present((i32)m_c->m_4->m_14, (i32)m_c->m_4->m_18); // vtbl[+0x34]
        m_4w()->m_54->Blit(m_c->m_24->m_5c->m_84, m_c->m_24->m_5c->m_88);
        if (m_c->m_20 != 0) { // frame profiler
            u32 t = timeGetTime();
            Eng_Profiler1(m_c->m_20, t);
            Eng_Profiler2(m_c->m_20, t);
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

        if (m_c->m_4->m_14 == 0) {
            return 1; // no view -> bail
        }

        FrameTimerBegin((i32)g_645584);            // m_frameMarker begin
        FrameTimerEnd(0, (i32)g_645584);           // wait: end takes (this,flag)
        Eng_SurfaceFlush(m_c->m_4->m_10->m_2c, 0); // surface flush
        // GutsStepX(m_region0Gate, m_guts, reg) -> the post-draw step (modeled via
        // the same external; here as the marker/guts step):
        GutsStep();            // (post-draw guts)
        m_c->m_24->PostStep(); // on m_c->m_24
        return 1;              // -> draw tail
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
                RegCue(g_64556c->m_60, 0x33e); // reg->m_60 cue
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
            Eng_BeginScene(m_c->m_4->m_10->m_2c, 0);
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
        m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);
        m_c->m_c->Present((i32)m_c->m_4->m_14, (i32)m_c->m_4->m_18); // present
        if (m_region1Gate != 0) {
            StepC(); // alt-input draw
        } else {
            m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);
            m_c->m_c->Present((i32)m_c->m_4->m_14, (i32)m_c->m_4->m_18);
        }
        MarkerBegin((i32)g_645584);
        GutsStep();
        InputSubStep(w->m_70); // m_4->m_70

        if (m_overlayActive != 0 && m_guts->m_mode != 5) { // on-screen overlay/banner
            Overlay1(0, (i32)g_645584);
            Overlay2(m_c, 0);
        }

        WorldBlit((i32)g_645584); // on m_4->m_5c (thiscall)
        if (m_c->m_4->m_14 == 0) {
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
                    MapLookup(g_64556c->m_30->m_8, g_64556c->m_15c, out);
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
                CopyRect(&m_hudRect, (const RECT*)((char*)g_64556c->m_30 + 0x24));
                Eng_HudDraw(g_64556c->m_30, &m_hudRect, 1);
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
        if (m_worldReady != 0) {                                 // optional HUD overlay draw
            Eng_HudDraw(m_c->m_4->m_10->m_2c, &m_hudRect, 0xff); // (this=m_4->m_10->m_2c)
        }
        Eng_SurfaceFlush(m_c->m_4->m_10->m_2c, 0);

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
    if (m_c->m_4->m_14 == 0) {
        return 1;
    }
    {
        if (m_c->m_20 != 0) { // cursor/frame profiler
            u32 t = timeGetTime();
            Eng_Profiler1(m_c->m_20, t);
            Eng_Profiler2(m_c->m_20, t);
        }
        if (m_paused != 0) {
            // ---- the paused frame: draw-only ----
            m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);
            m_c->m_c->Present((i32)m_c->m_4->m_14, (i32)m_c->m_4->m_18); // present
            GutsStep();
            if (m_guts->m_busyA == 0 && m_guts->m_busyB == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0); // win/lose
            }
            FrameTimerEnd(1, 0);
        } else {
            // ---- the active short frame: entity step + cues ----
            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);
                m_c->m_c->Present((i32)m_c->m_4->m_14, (i32)m_c->m_4->m_18); // present
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
        Eng_SurfaceFlush(m_c->m_4->m_10->m_2c, 0);
    }
    return 1; // draw tail
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
// CPlay::StepScroll - per-frame scroll-offset
// recompute. Reads the draw-surface (m_c->m_24) scroll origin (+0x10/+0x14) and
// its geom block (+0x5c -> +0x40.{x,y}), adds the BeginFrameClear extents
// (m_150/m_154), aligns each axis DOWN to a 0x20 boundary (+0x10 bias) and
// stores the result into the scroll-offset sink m_scrollSink (+0x5c X, +0x60 Y).
// ===========================================================================
RVA(0x000d1ac0, 0x4f)
void CPlay::StepScroll() {
    CDrawSurface* v = m_c->m_24;
    CDrawSurface::CameraGeom* geom = v->m_5c;

    i32 y = m_154 + (geom->m_originY - v->m_viewport.top);  // [edx+4]-m_14; +=m_154
    i32 x = geom->m_originX + (m_150 - v->m_viewport.left); // [edx]; +=m_150-m_10

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
    void* probeTarget = m_c->m_4->m_14->m_2c;
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
        if (((CCueState*)&m_cueState)->Probe(cueId) == 0) {
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
        // the viewport rect (m_c->m_24->m_viewport) ptr (edx) does not survive
        // the GetInt calls, so all 4 corners are read up front.
        RECT& vp = m_c->m_24->m_viewport;
        i32 l = vp.left, t = vp.top, r = vp.right, b = vp.bottom;
        i32 bottom = b - g_buteText->GetInt("Font", "TextBottomEdge");
        i32 right = r - g_buteText->GetInt("Font", "TextRightEdge");
        i32 top = t + g_buteText->GetInt("Font", "TextTopEdge");
        i32 left = l + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    if (a3 != 0) {
        Eng_CueRenderTop(m_c, &m_cueState, &rect, a2, 1, a4, a5, a6, a7);
    } else {
        Eng_CueRenderDef(m_c, &m_cueState, &rect, a2, 1, a4, a5, a6, a7);
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
    if (m_c->m_24->m_5c != 0) {
        m_c->m_24->m_5c->DrawA();
    }
    g_6bf3c0 = g_645580;
    g_6bf3bc = g_645584;
    m_c->m_8->BeginScene(0);           // m_c->m_8->vtbl[+0x24](0)
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
                if (m_c->m_24->m_5c != 0) {
                    m_c->m_24->m_5c->DrawB();
                }
            }
            Vslot26(); // this->vtbl[+0x98]()
            if (m_c->m_24->m_5c != 0) {
                m_c->m_24->m_5c->DrawA();
            }
            m_c->m_8->BeginScene(0);
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
    CPlaneGeom* pg = m_4w()->m_30->m_24->m_5c;
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
    RECT& vp = m_c->m_24->m_viewport;
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
    m_c->m_10->m_10.Lookup(key, grid);
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
//   m_dragSnapActive  -> a drag is being snapped: snap to (m_158+x, m_15c+y) and re-arm m_scrollSink.
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
        DragSnapTo(m_158 + x, m_15c + y);
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
    RECT box = m_c->m_24->m_viewport;
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
            // normalize {m_150,m_154}..{m_dragClampMaxX,m_dragClampMaxY} into min/max:
            m_hudRect.left = m_150 < m_dragClampMaxX ? m_150 : m_dragClampMaxX;
            m_hudRect.right = m_150 > m_dragClampMaxX ? m_150 : m_dragClampMaxX;
            m_hudRect.top = m_154 < m_dragClampMaxY ? m_154 : m_dragClampMaxY;
            m_hudRect.bottom = m_154 > m_dragClampMaxY ? m_154 : m_dragClampMaxY;
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
        CDrawSurface* v = m_c->m_24;
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
        i32 lo = m_150 > left ? m_150 : left;
        m_hudRect.left = lo;
        if (lo >= m_dragClampMaxX) {
            m_hudRect.left = m_dragClampMaxX;
        }
        i32 hi = m_150 < right ? m_150 : right;
        m_hudRect.right = hi;
        if (hi > m_dragClampMaxX) {
            m_hudRect.right = m_dragClampMaxX;
        }
        i32 tlo = m_154 <= top ? m_154 : top;
        m_hudRect.top = tlo;
        if (tlo < m_dragClampMaxY) {
            m_hudRect.top = m_dragClampMaxY;
        }
        i32 thi = m_154 < bottom ? m_154 : bottom;
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
    void* view = m_c->m_4->m_14;
    if (view == 0) {
        return 0;
    }
    if (m_revealFrame == 1) {
        Eng_HudStrip(view, (void*)m_revealCapStart, 0x140, 0x1a6, 1, 0);
        Eng_HudStrip(m_c, (void*)m_revealCapMid, 0xe0, 0x1a6, 1, 0);
    }

    i32 counter = m_revealFrame;
    i32 col = (i32)((float)counter * 3.7857143878936768f);
    if (counter < 0x37) {
        i32 i = counter;
        do {
            i32 x = 0xe0 - (i32)((float)i * -3.7857143878936768f);
            Eng_HudStrip(m_c, (void*)m_revealCapMid, x, 0x1a6, 1, 0);
            i++;
        } while (i < 0x37);
    } else {
        Eng_HudStrip(m_c, (void*)m_revealCapMid, col + 0xe0, 0x1a6, 1, 0);
    }

    Eng_HudStrip(m_c, (void*)m_revealCapEnd, 0x1b4, 0x1a6, 1, 0);
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
    m_150 = arg2;
    m_154 = arg3;
    return 1;
}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x000cdb10, 0x80c)
i32 CPlay::winapi_0cdb10_PostMessageA(i32, i32, i32) {
    return 0;
}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x000ceae0, 0x268)
i32 CPlay::winapi_0ceae0_PostMessageA(i32, i32, i32) {
    return 0;
}

// @confidence: low
// @source: winapi:CopyRect
// @stub
RVA(0x000d0b30, 0x200)
i32 CPlay::winapi_0d0b30_CopyRect(i32) {
    return 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000d0120, 0x5d8)
void CPlay::LoadCursorSprites(i32, i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000d12b0, 0x2d5)
void CPlay::LoadScrollSpeedOptions() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dc6d0, 0x215)
void CPlay::BuildGruntTypeNameTable(i32, i32, i32, i32) {}
