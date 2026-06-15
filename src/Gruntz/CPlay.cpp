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

// ---- The global CButeMgr text-config tree (the @0x6453d8 singleton). Modeled as
//      a minimal class so PlayCueAt's `ecx=0x6453d8; call GetInt` reloc-masks
//      against the already-matched ?GetInt@CButeMgr@@QAEHPAD0@Z (butemgr unit). --
class CButeMgr {
public:
    int GetInt(char *tag, char *key);   // @0x171af0 (thiscall ret 8)
};
// @data: 0x2453d8
extern CButeMgr g_buteMgr;
#define g_buteText (&g_buteMgr)

// ---- StepInputA / PlayCueAt leaf engine callees (free fns / reloc-masked). ----
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
    // --- StepInputA @0xd11e0 ---
    int   __stdcall Eng_InputProbe(void *a, void *b, void *axis, void *edge, int n); // 0x13ef90
    void  Eng_InputDispatch(int z0, int z1, int probe);       // 0x141400 (cdecl, 3)
    // --- OnRegion3/4 leaf cues ---
    void  Eng_RegionCueA(int a, int b, int c, int d, int e);  // 0xec1c0 (cdecl, 5)
    // --- PlayCueAt @0xd1890 cue renderers (cdecl, 9 args each) ---
    void  Eng_CueRenderTop(void *cueObj, void *cueState, RECT *r,
                           int a2, int one, int a4, int a5, int a6, int a7); // 0x115440
    void  Eng_CueRenderDef(void *cueObj, void *cueState, RECT *r,
                           int a2, int one, int a4, int a5, int a6, int a7); // 0x115520
}

// A reg->m_68 sink that OnRegion4 posts to (thiscall, 2 args, ret 8). @0x7c2e0.
struct CRegSink { void Post(int a, int b); };
// PlayCueAt's per-cue de-dupe object at this+0x410 (thiscall compare, ret 4). @0x1bedde
struct CCueState { int Probe(int wParam); };

// ===========================================================================
// CPlay::Render  @0xc8cf0  (vtable slot +0x14)
// ===========================================================================
// @address: 0x0c8cf0
// @size:    0xc14
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
                PlayCueAt(0x8128, 0x78, 0, 0xff, 0xff, 0, 1, 0);  // @0x1e4c cue
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
                PlayCueAt(0x8129, 0x78, 0, 0xff, 0xff, 0, 1, 0);  // @0x1e4c
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
            if (e >= (unsigned)m_448) OnRegion1((int)g_645588);  // @0x3792
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
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0);  // @0x1e4c win/lose
            FrameTimerEnd(1, 0);                           // @0x27a2
        } else {
            // ---- the active short frame (@0xc9701): entity step + cues ----
            if (m_510 > 0) {
                m_510 = m_510 - 1;
                m_c->m_24->PushView(m_c->m_4->m_14, m_c->m_8);    // @0x15dc90
                m_c->m_c->Present((int)m_c->m_4->m_14, (int)m_c->m_4->m_18);   // present
                GutsStep();                                     // @0x21b7
                Eng_FrameTimerStep(m_2dc, 0x32);                // @0x13f460
                PlayCueAt(m_40c, 0x78, 0, 0xff, 0xff, 0, 1, 0); // @0x1e4c (cueId=m_40c)
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

// ===========================================================================
// CPlay::StepC  @0xd8d90  (30 B, thiscall, void) - the menu/overlay-frame view
// sub-step. A 3-way switch on the view-mode discriminator m_480: 0 = idle (no
// view yet, bail), 1 = mode-A sub-step, 2(+) = mode-B sub-step. MSVC hoists the
// shared `push 4` out of the if/else (both helpers take the same constant).
// ===========================================================================
// @address: 0x0d8d90
// @size:    0x1e
void CPlay::StepC()
{
    int mode = m_480;
    if (mode == 0)
        return;
    if (mode == 1)
        StepC_ModeA(4);
    else
        StepC_ModeB(4);
}

// ===========================================================================
// The four screen-region scroll one-shots @0xd8a00/0xd8aa0/0xd8b20/0xd8bc0.
// Each: thiscall(int z), set its region-active gate to bool(z), call the shared
// enter/leave sub-step, (re)arm its 64-bit countdown timer (interval 0x7530 ms,
// lo = g_645588, hi = 0), and return 1. They share ONE shape; OnRegion2 also
// pins the view discriminator m_480, OnRegion3/4 fire an extra cue on enter/leave.
// ===========================================================================
// @address: 0x0d8a00
// @size:    0x73
int CPlay::OnRegion2(int z)         // @0xd8a00  (region-0 / gate m_470, timer +0x430)
{
    if (z != 0) { m_470 = 1; RegionEnter(); m_480 = 1; }
    else        { m_470 = 0; RegionLeave(); m_480 = 2; }
    m_438 = 0x7530;
    m_43c = 0;
    *(unsigned __int64 *)&m_430 = g_645588;   // 64-bit store: lo=g_645588, hi=0
    return 1;
}

// @address: 0x0d8aa0
// @size:    0x5f
int CPlay::OnRegion1(int z)         // @0xd8aa0  (region-1 / gate m_474, timer +0x440)
{
    if (z != 0) { m_474 = 1; RegionEnter(); }
    else        { m_474 = 0; RegionLeave(); }
    m_448 = 0x7530;
    m_44c = 0;
    *(unsigned __int64 *)&m_440 = g_645588;   // 64-bit store: lo=g_645588, hi=0
    return 1;
}

// @address: 0x0d8b20
// @size:    0x74
int CPlay::OnRegion3(int z)         // @0xd8b20  (region-2 / gate m_478, timer +0x450)
{
    if (z != 0) {
        m_478 = 1;
        RegionEnter();
        Eng_RegionCueA(0x7530, 6, 6, 0, 0x2d);
    } else {
        m_478 = 0;
        RegionLeave();
    }
    m_458 = 0x7530;
    m_45c = 0;
    *(unsigned __int64 *)&m_450 = g_645588;   // 64-bit store: lo=g_645588, hi=0
    return 1;
}

// @address: 0x0d8bc0
// @size:    0x71
int CPlay::OnRegion4(int z)         // @0xd8bc0  (region-3 / gate m_47c, timer +0x460)
{
    if (z != 0) {
        m_47c = 1;
        RegionEnter();
    } else {
        m_47c = 0;
        RegionLeave();
        ((CRegSink *)g_64556c->m_68)->Post(-1, 0);   // reg->m_68->Post(0xffffffff, 0)
    }
    m_468 = 0x7530;
    m_46c = 0;
    *(unsigned __int64 *)&m_460 = g_645588;   // 64-bit store: lo=g_645588, hi=0
    return 1;
}

// ===========================================================================
// CPlay::StepScroll  @0xd1ac0  (79 B, thiscall, void) - per-frame scroll-offset
// recompute. Reads the draw-surface (m_c->m_24) scroll origin (+0x10/+0x14) and
// its geom block (+0x5c -> +0x40.{x,y}), adds the BeginFrameClear extents
// (m_150/m_154), aligns each axis DOWN to a 0x20 boundary (+0x10 bias) and
// stores the result into the scroll-offset sink m_4e4 (+0x5c X, +0x60 Y).
// ===========================================================================
// @address: 0x0d1ac0
// @size:    0x4f
void CPlay::StepScroll()
{
    CDrawSurface *v = m_c->m_24;
    int *geom = (int *)((char *)v->m_5c + 0x40);     // edx = (m_5c+0x40)

    int y = m_154 + (geom[1] - v->m_14);             // edi=m_154 held; eax=[edx+4]-m_14; +=edi
    int x = geom[0] + (m_150 - v->m_10);             // esi=[edx] held; edi=m_150-m_10; +=esi

    y = (y & ~0x1f) + 0x10;                           // align down 0x20 (and al,0xe0); + 0x10
    x = (x & ~0x1f) + 0x10;                           // align down 0x20 (and edi,~0x1f); + 0x10

    m_4e4->m_5c = x;
    m_4e4->m_60 = y;
}

// ===========================================================================
// CPlay::StepInputA  @0xd11e0  (155 B, thiscall, int) - per-frame input poll.
// Two boot-time one-shot latches (m_1a8/m_1ac fire on the first two frames),
// then a per-frame edge/axis probe over one of two mirrored half-blocks selected
// by m_1b0. Probes the draw-surface (m_c->m_4->m_14->m_2c); if absent returns 0.
// On a hit it dispatches the probed control. Returns 1.
// ===========================================================================
// @address: 0x0d11e0
// @size:    0x9b
int CPlay::StepInputA()
{
    if (m_1a8 == 0) { m_1a8 = 1; return 1; }
    if (m_1ac == 0) { m_1ac = 1; return 1; }

    int   axisVal;
    Edge *edge;
    void *halfPtr;
    if (m_1b0 == 0) { axisVal = m_160; edge = &m_188; halfPtr = m_168; }
    else            { axisVal = m_164; edge = &m_198; halfPtr = m_178; }

    // null-check the draw surface m_c->m_4->m_14->m_2c (walks through the this reg).
    void *probeTarget = ((void **)m_c->m_4->m_14)[0xb];   // [+0x2c] = index 0xb
    if (probeTarget == 0)
        return 0;

    int r = Eng_InputProbe((void *)edge->m_0, (void *)edge->m_4,
                           (void *)axisVal, halfPtr, 0x10);
    if (r != 0)
        Eng_InputDispatch(0, 0, r);
    return 1;
}

// ===========================================================================
// CPlay::PlayCueAt  @0xd1890  (442 B, thiscall, ret 0x20 = 8 args) - the ambient/
// positional on-screen text-cue. Args: (cueId, a2, a3, a4, a5, a6, a7, rectSrc).
// De-dupes via the per-cue state object this+0x410 (skip if the same cueId is
// already showing AND its Probe says still-live). Builds the cue RECT from the
// font text-margins (CButeMgr GetInt "Font"/"Text{Left,Top,Right,Bottom}Edge")
// applied to either the caller's rect (rectSrc != 0) or the active viewport
// (this->m_c->m_24+0x10). a3 selects the Top vs Default cue renderer.
// ===========================================================================
// @address: 0x0d1890
// @size:    0x1ba
void CPlay::PlayCueAt(int cueId, int a2, int a3, int a4, int a5,
                      int a6, int a7, int rectSrc)
{
    RECT rect;

    if (cueId != m_40c) {
        if (((CCueState *)&m_410)->Probe(cueId) == 0)
            return;                          // still-live other cue -> skip
        m_40c = cueId;
    }

    if (rectSrc != 0) {
        int *src = (int *)rectSrc;
        int bottom = src[3] - g_buteText->GetInt("Font", "TextBottomEdge");
        int right  = src[2] - g_buteText->GetInt("Font", "TextRightEdge");
        int top    = src[1] + g_buteText->GetInt("Font", "TextTopEdge");
        int left   = src[0] + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    } else {
        // the viewport rect lives at m_c->m_24 + 0x10; that ptr (edx) does not
        // survive the GetInt calls, so all 4 corners are read up front.
        int *vp = (int *)((char *)m_c->m_24 + 0x10);
        int l = vp[0], t = vp[1], r = vp[2], b = vp[3];
        int bottom = b - g_buteText->GetInt("Font", "TextBottomEdge");
        int right  = r - g_buteText->GetInt("Font", "TextRightEdge");
        int top    = t + g_buteText->GetInt("Font", "TextTopEdge");
        int left   = l + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    if (a3 != 0)
        Eng_CueRenderTop(m_c, &m_410, &rect, a2, 1, a4, a5, a6, a7);   // @0x115440
    else
        Eng_CueRenderDef(m_c, &m_410, &rect, a2, 1, a4, a5, a6, a7);   // @0x115520
}
