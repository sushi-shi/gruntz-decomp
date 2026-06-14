// CPlay.cpp - CPlay::Render @0xc8cf0 (3092 B): the in-game PLAY state's per-frame
// step+draw, the heart of the running game. CARCASS reconstruction: the control
// flow, the CPlay/CState/CGameRegistry member offsets, and the ordered per-frame
// call sequence are faithful; field/callee names are placeholders and unmatched
// engine callees are external no-body fns (reloc-masked). See CPlay.h for the
// layout + helper-method map.
//
// =====================  THE PER-FRAME CARCASS (the deliverable)  ============
// CPlay::Render runs once per game frame (RezMgr::PerFrameTick -> m_mode->
// vtbl[+0x14]). It is a 3-way dispatch on two state words (m_4ec the hard
// early-out, m_4f8 the primary mode) wrapped in a C++ SEH/EH frame (a stack
// CString temp in the alt path -> /GX). The three paths share one WORLD-DRAW
// block and a common return-1 tail.
//
//   this->vtbl[+0x7c](0, m_150, m_154);          // BeginFrameClear (a virtual)
//   m_414 = 0;                                    // per-frame "drew" flag
//   if (m_4ec) return 1;                          // hard early-out
//
//   if (m_4f8) {                  // ---- MAIN in-game frame (@0xc8d44) ----
//       StepInputA(); StepWorldB(); ViewPreStep(m_c->m_24);
//       g_6bf3c0=g_645580; g_6bf3bc=g_645584;     // mirror the draw clock
//       DRAW_WORLD();                             // shared world-draw block
//       <AMBIENT-cue timer +0x3f8, 0x1f4ms, toggles m_408 -> PlayCueAt 0x8128>
//       MarkerBegin(now); GutsStep();             // m_2e4 marker + m_2dc step
//       if (m_c->m_4->m_14 == 0) return 1;        // no view -> bail
//       FrameTimerEnd; DrawSurfaceFlush; GutsStepX; ViewPostStep; return 1;
//   } else if (m_4->m_c==0 && !(reg->m_134!=2 && m_4fc)) {
//                                 // ---- MENU/PAUSE-OVERLAY frame (@0xc8f52) ----
//       FrameTimerBegin(now); Eng_FrameTimerStep(m_4->m_6c,0);
//       if (m_2f8==0x66) <booty-region one-shot +0x328 0x2710ms -> RegCue 0x33e>;
//       StepInputA(); StepC();
//       if (m_348==0) <AMBIENT level-init: wsprintf "AMBIENT%d" -> PlaySound/
//                      FindSound -> latch m_348=1>;
//       if (m_470) { Eng_BeginScene(m_c->m_4->m_14->m_2c); GutsStepB(); }
//       if (m_30c==0) { if (m_4->m_68->m_230) WorldSubstep(); StepWorldB(); }
//       StepScroll();
//       if (0x12<dt && dt<0xc8) RenderFast() else RenderSlow();   // UNSIGNED gate
//       DRAW_WORLD();                             // same shared world-draw block
//       InputSubStep(m_4->m_70);
//       if (m_320 && m_2dc->state-ok) ShowOverlay();   // on-screen banner
//       WorldBlit(g_645584);
//       if (m_c->m_4->m_14==0) return 1;
//       if (m_4b0) SnapshotStep();                // screenshot countdown +0x4a0
//       <four scroll-region one-shots @0x430/0x440/0x450/0x460>; return 1;
//   } else {                      // ---- m_4->m_c != 0 short path (@0xc96b7) ----
//       StepInputA();
//       if (m_c->m_4->m_14==0) return 1;
//       <cursor profiler m_c->m_20: timeGetTime x2>;
//       if (m_500) DRAW_ONLY()                    // paused: present + win/lose FX
//       else { if (--m_510>0) <entity step + level cue>; if (m_348==0) AmbientInit(); }
//       MarkerBegin(now); PostHud(0); DrawSurfaceFlushTail(); return 1;
//   }
//
// THE SHARED WORLD-DRAW BLOCK (verbatim @0xc8d75 and @0xc910d):
//   m_c->m_8->vtbl[+0x24](0);                     // renderer A: begin scene
//   Eng_PushView(m_c->m_24, m_c->m_4->m_14, m_c->m_8);             // @0x15dc90
//   m_c->m_c->vtbl[+0x34](m_c->m_4->m_14, m_c->m_4->m_18);          // present
//   WorldBlit(m_c->m_24->m_5c->m_84, ->m_88) on m_4->m_54;          // @0x1a7d
//   if (m_c->m_20) { t=timeGetTime(); profiler(m_c->m_20,t) x2; }   // frame profiler
//   MarkerBegin(g_645584); GutsStep();                             // marker + guts
//
// The per-ENTITY layer is one indirection down: g_entityList @0x645574 is walked
// by the world object (m_4->m_54) inside the reloc-masked @0x1a7d call. The
// next targets are CPlay's own sub-steps (0xd0000-range) + the world-draw helpers
// (0x15dc90 push-view, 0x13e850 surface-flush, 0x1a7d camera blit, 0x163f40 HUD).
// ============================================================================

#include "CPlay.h"

// ---- MFC primitives reused verbatim from the engine (reloc-masked). ----
class CString {
public:
    CString();              // @0x1b9b93
    ~CString();             // @0x1b9cde
    void Format(const char *, ...);   // @0x1b2cf5
};
extern int MapLookup(void *map, void *key, void *&out);   // @0x1b8760 CMapPtrToPtr::Lookup

// ---- Unmatched engine callees (the larger direct-call RVAs). External no-body
//      so the `call rel32` reloc-masks; shapes picked to reproduce the pushes. --
extern "C" {
    void  Eng_SurfaceFlush(void *surf, int z);                // 0x13e850 (begin, 0)
    void  Eng_BeginScene(void *surf, int z);                  // 0x13e760
    void  Eng_Profiler1(void *timer, unsigned long t);        // 0x136e20
    void  Eng_Profiler2(void *timer, unsigned long t);        // 0x137ac0
    void  Eng_HudDraw(void *hud, RECT *r, int c);             // 0x163f40
    void  Eng_HudFlush(void *hud);                            // 0x163370
    int   Eng_PlaySound(void *snd, const char *name, int flag); // 0x138840
    void *Eng_FindSound(void *snd, const char *name);         // 0x138730
    void  Eng_StopSound(void *snd, int flag);                 // 0x139030
    void  Eng_FrameTimerStep(void *t, int now);               // 0x13f460
}

// ===========================================================================
// CPlay::Render  @0xc8cf0  (vtable slot +0x14)
// ===========================================================================
int CPlay::Render()
{
    // --- frame entry: clear the per-frame flag, then a `this`-virtual begin. ---
    // (the m_414=0 store is scheduled INTO the BeginFrameClear arg setup.)
    m_414 = 0;
    BeginFrameClear(0, m_150, m_154);     // this->vtbl[+0x7c](0, m_150, m_154)

    if (m_4ec != 0)
        return 1;                         // @0xc8d2c hard early-out

    if (m_4f8 != 0) {
        // =================================================================
        // ---- MAIN in-game frame (@0xc8d44) ----
        // =================================================================
        StepInputA();                     // @0x259a poll/sim sub-step A
        StepWorldB();                     // @0x434a world/camera sub-step B
        m_c->m_24->PreStep();             // @0x28d3 on m_c->m_24 (view pre-step)

        g_6bf3c0 = g_645580;              // mirror the draw clock
        g_6bf3bc = g_645584;

        // --- shared world-draw block #1 (@0xc8d75) ---
        m_c->m_8->BeginScene(0);                     // m_c->m_8->vtbl[+0x24](0)
        m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);   // @0x15dc90 (thiscall on m_24)
        m_c->m_c->Present((int)m_c->m_4->m_14, (int)m_c->m_4->m_18);  // vtbl[+0x34]
        m_4w()->m_54->Blit(m_c->m_24->m_5c->m_84, m_c->m_24->m_5c->m_88);  // @0x1a7d
        if (m_c->m_20 != 0) {                        // frame profiler
            unsigned long t = timeGetTime();
            Eng_Profiler1(m_c->m_20, t);
            Eng_Profiler2(m_c->m_20, t);
        }
        MarkerBegin((int)g_645584);                  // @0x2cc0 m_2e4 begin-marker
        GutsStep();                                  // @0x21b7 m_2dc step

        // --- periodic AMBIENT-cue timer (+0x3f8, 0x1f4 ms; toggles m_408) ---
        {
            unsigned int elapsed = g_645588 - (unsigned)m_3f8;
            if (elapsed >= (unsigned)m_400) {
                m_408 = (m_408 == 0);
                m_400 = 0x1f4;
                m_404 = 0;
                m_3f8 = (int)g_645588;
                m_3fc = 0;
            }
            if (m_408 != 0)
                PlayCueAt(0, 0x8128, 0x78, 0, 0xff, 0xff, 1);  // @0x1e4c cue
        }

        if (m_c->m_4->m_14 == 0)
            return 1;                                // @0xc8ea1 no view -> bail

        FrameTimerBegin((int)g_645584);              // @0x3710 m_3f4 begin
        FrameTimerEnd(0, (int)g_645584);             // wait: end takes (this,flag)
        Eng_SurfaceFlush(m_c->m_4->m_10->m_2c, 0);         // @0x13e850 surface flush
        // GutsStepX(m_470, m_2dc, reg) -> the post-draw step @0x2356 (modeled via
        // the same external; here as the marker/guts step):
        GutsStep();                                  // @0x2356 (post-draw guts)
        m_c->m_24->PostStep();                       // @0x3783 on m_c->m_24
        return 1;                                    // @0xc8f26 -> draw tail
    }

    // m_4f8 == 0
    if (((CWorld *)m_4)->m_c != 0)
        goto alt2;                                   // @0xc8f31
    {
        CGameRegistry *reg = g_64556c;
        if (reg->m_134 != 2 && m_4fc != 0)
            goto alt2;                               // @0xc8f4c
    }

    // =================================================================
    // ---- MENU / PAUSE-OVERLAY frame (@0xc8f52) ----
    // =================================================================
    {
        CWorld *w = (CWorld *)m_4;
        FrameTimerBegin((int)g_645584);              // @0x3710 m_3f4 begin
        Eng_FrameTimerStep(w->m_6c, 0);              // @0x20b3 m_4->m_6c step

        if (m_2f8 == 0x66) {                         // booty-region one-shot
            unsigned int elapsed = g_645588 - (unsigned)m_328;
            if (elapsed >= (unsigned)m_330) {
                RegCue(g_64556c->m_60, 0x33e);       // @0x39f4 reg->m_60 cue
                m_330 = 0x2710;
                m_334 = 0;
                m_328 = (int)g_645588;
                m_32c = 0;
            }
        }

        StepInputA();                                // @0x259a
        StepC();                                     // @0x1da7

        // --- AMBIENT level-init one-shot (+0x348) ---
        if (m_348 == 0) {
            unsigned int elapsed = g_645588 - (unsigned)m_338;
            if (elapsed >= (unsigned)m_340) {
                int id = GetAmbientId();             // @0x1df2
                CString name; (void)name;            // [esp+0x10] CString temp (/GX)
                char buf[0x80];
                wsprintfA(buf, "AMBIENT%d", id);     // s_AMBIENT%d @0x610888
                if (g_64556c->m_14 != 0) {
                    Eng_PlaySound(w->m_48, buf, 1);  // @0x138840
                } else {
                    void *out = 0;
                    void *snd = Eng_FindSound(w->m_48, buf);  // @0x138730
                    if (snd != 0) out = snd;
                    if (out != 0) Eng_StopSound(out, 1);      // @0x139030
                }
                m_348 = 1;
            }
        }

        if (m_470 != 0) {                            // extra HUD/overlay layer
            Eng_BeginScene(m_c->m_4->m_10->m_2c, 0);       // @0x13e760
            GutsStepB();                             // @0x125d m_2dc
        }

        if (m_30c == 0) {                            // world-ready init
            if (w->m_68->m_230 != 0)
                WorldSubstep();                      // @0x1398
            StepWorldB();                            // @0x434a
        }

        StepScroll();                                // @0x3850

        // per-frame timing gate (UNSIGNED clamp: 0x12 < dt < 0xc8):
        {
            unsigned int dt = g_645584;
            if (dt > 0x12 && dt < 0xc8)
                RenderFast();                        // call [eax+0xa0]
            else
                RenderSlow();                        // call [edx+0x9c]
        }

        // --- shared world-draw block #2 (@0xc910d) ---
        m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8); // @0x15dc90
        m_c->m_c->Present((int)m_c->m_4->m_14, (int)m_c->m_4->m_18);// present
        if (m_474 != 0) {
            StepC();                                 // @0x1ae6 alt-input draw
        } else {
            m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);
            m_c->m_c->Present((int)m_c->m_4->m_14, (int)m_c->m_4->m_18);
        }
        MarkerBegin((int)g_645584);                  // @0x2cc0
        GutsStep();                                  // @0x21b7
        InputSubStep(w->m_70);                       // @0x3562 m_4->m_70

        if (m_320 != 0 && m_2dc->m_10c != 5) {       // on-screen overlay/banner
            Overlay1(0, (int)g_645584);              // @0x1fa0
            Overlay2(m_c, 0);                        // @0x14dd
        }

        WorldBlit((int)g_645584);                    // @0x441c on m_4->m_5c (thiscall)
        if (m_c->m_4->m_14 == 0)
            return 1;                                // @0xc927d

        // --- snapshot/screenshot countdown (+0x4a0/+0x4a8) ---
        if (m_4b0 != 0) {
            unsigned int now = g_645588;
            unsigned int dur = (unsigned)(m_4a8 + m_4a0) - now;
            if ((int)dur >= 0) {
                // duration elapsed: post a message + reset the marker block + walk.
                if (m_2dc->m_574 != 0)
                    SnapPostMessage(5);              // @0x18e3 reg->m_68 (5)
                else
                    SnapPostMessage(g_644c54);
                // reset the m_3f4 marker block (+0x30..0x4c):
                FrameTimerEnd(0, 0);
                GutsStepC();                         // @0x14ba m_2dc
                m_4b0 = 0;
                // walk the level tree (CMapPtrToPtr::Lookup @0x1b8760):
                if (g_64556c->m_15c != 0) {
                    void *out = 0;
                    MapLookup(g_64556c->m_30->m_8, g_64556c->m_15c, out);
                    if (out != 0) SnapWalk();        // @0x3a1c
                }
            } else {
                // not yet: build a CString temp, CopyRect the viewport, HudDraw.
                CString tmp; (void)tmp;              // [esp+0x10] CString temp
                tmp.Format("%s", "");                // @0x1b2cf5 (s @0x60bdd4)
                CopyRect(&m_310, (const RECT *)g_64556c->m_30->m_24);  // @0x6c44bc
                Eng_HudDraw(g_64556c->m_30, &m_310, 1);                // @0x31d9
            }
            // (CString temp dtor @0x1b9cde runs here under the EH frame)
        }

        // --- the four scroll-region one-shots (@0x430/0x440/0x450/0x460) ---
        FrameTimerEnd(0, m_3f4 != 0);                // @0x27a2 / @0x2bfd reset
        OnRegion5();                                 // @0x3797 -> 0xcf770
        Eng_FrameTimerStep(w->m_68, 0);              // @0x2b85 m_4->m_68

        if (m_4f4 != 0 && m_2dc->m_550 == 0 && m_2dc->m_554 == 0) {
            // win/lose banner timer (+0x3f8 again, 0x1f4 ms):
            unsigned int elapsed = g_645588 - (unsigned)m_3f8;
            if (elapsed >= (unsigned)m_400) {
                m_408 = (m_408 == 0);
                m_400 = 0x1f4;
                m_404 = 0;
                m_3f8 = (int)g_645588;
                m_3fc = 0;
            }
            if (m_408 != 0)
                PlayCueAt(0, 0x8129, 0x78, 0, 0xff, 0xff, 1);  // @0x1e4c
        }

        MarkerBegin((int)g_645584);                  // @0x2e2d
        PostHud(0);                                  // @0x1519
        if (m_30c != 0) {                            // optional HUD overlay draw
            Eng_HudDraw(m_c->m_4->m_10->m_2c, &m_310, 0xff);  // @0x163f40 (this=m_4->m_10->m_2c)
        }
        Eng_SurfaceFlush(m_c->m_4->m_10->m_2c, 0);         // @0x13e850

        // --- the four screen-region scroll one-shots (@0x95b2..0x96b2): each is
        // a 64-bit "inside region" elapsed test that fires its OnRegion handler. ---
        if (m_470 != 0) {                            // region-1 (+0x430)
            unsigned int e = g_645588 - (unsigned)m_430;
            if (e >= (unsigned)m_438) OnRegion2((int)g_645588);  // @0x3a85
        }
        if (m_474 != 0) {                            // region-2 (+0x440)
            unsigned int e = g_645588 - (unsigned)m_440;
            if (e >= (unsigned)m_448) OnRegion1();   // @0x3792
        }
        if (m_478 != 0) {                            // region-3 (+0x450)
            unsigned int e = g_645588 - (unsigned)m_450;
            if (e >= (unsigned)m_458) OnRegion3((int)g_645588);  // @0x3904
        }
        if (m_47c != 0) {                            // region-4 (+0x460)
            unsigned int e = g_645588 - (unsigned)m_460;
            if (e >= (unsigned)m_468) OnRegion4((int)g_645588);  // @0x3e45
        }
        return 1;                                    // -> draw tail
    }

alt2:
    // =================================================================
    // ---- the m_4->m_c != 0 short path (@0xc96b7) ----
    // =================================================================
    StepInputA();                                    // @0x259a
    if (m_c->m_4->m_14 == 0)
        return 1;                                    // @0xc96cb
    {
        if (m_c->m_20 != 0) {                        // cursor/frame profiler
            unsigned long t = timeGetTime();
            Eng_Profiler1(m_c->m_20, t);
            Eng_Profiler2(m_c->m_20, t);
        }
        if (m_500 != 0) {
            // ---- the paused frame (@0xc984c): draw-only ----
            m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);   // @0x15dc90
            m_c->m_c->Present((int)m_c->m_4->m_14, (int)m_c->m_4->m_18);  // present
            GutsStep();                                    // @0x21b7
            if (m_2dc->m_550 == 0 && m_2dc->m_554 == 0)
                PlayCueAt(0, 0x812c, 0x78, 0, 0xff, 0xff, 1);  // @0x1e4c win/lose
            FrameTimerEnd(1, 0);                           // @0x27a2
        } else {
            // ---- the active short frame (@0xc9701): entity step + cues ----
            if (m_510 > 0) {
                m_510 = m_510 - 1;
                m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);    // @0x15dc90
                m_c->m_c->Present((int)m_c->m_4->m_14, (int)m_c->m_4->m_18);   // present
                GutsStep();                                     // @0x21b7
                Eng_FrameTimerStep(m_2dc, 0x32);                // @0x13f460
                PlayCueAt(0, 0x78, 0, 0xff, 0xff, 0, 1);        // @0x1e4c
                FrameTimerEnd(1, 0);                            // @0x27a2
            }
            if (m_348 == 0) {
                // the same AMBIENT level-init one-shot (+0x348):
                unsigned int elapsed = g_645588 - (unsigned)m_338;
                if (elapsed >= (unsigned)m_340) {
                    int id = GetAmbientId();
                    CString name; (void)name;
                    char buf[0x80];
                    wsprintfA(buf, "AMBIENT%d", id);
                    if (g_64556c->m_14 != 0) {
                        Eng_PlaySound(((CWorld *)m_4)->m_48, buf, 1);
                    } else {
                        void *out = 0;
                        void *snd = Eng_FindSound(((CWorld *)m_4)->m_48, buf);
                        if (snd != 0) out = snd;
                        if (out != 0) Eng_StopSound(out, 1);
                    }
                    m_348 = 1;
                }
            }
        }
        MarkerBegin((int)g_645584);                  // @0x2e2d
        PostHud(0);                                  // @0x1519
        Eng_SurfaceFlush(m_c->m_4->m_10->m_2c, 0);         // @0x13e850
    }
    return 1;                                        // @0xc98ec draw tail
}
