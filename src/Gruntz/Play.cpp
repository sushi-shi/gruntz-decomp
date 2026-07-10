// Play.cpp - CPlay::Render: the in-game PLAY state's per-frame
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
//       if (m_c->m_drawTarget->m_14 == 0) return 1;        // no view -> bail
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

#include <Gruntz/Play.h>
class CDDrawWorkerRegistry {
public:
    i32 HasKeyEqual_155550(const char* k);
    i32 RemoveKeysEqual_155360(const char* a, const char* b);
}; // 0x155550/0x155360
class
    CSymTab; // ScanTree_152ad0's real 1st arg (the anim namespace tree); cast at the void* call sites
class CDDrawSubMgrAni {
public:
    i32 ScanTree_152ad0(
        CSymTab* n,
        const char* a,
        const char* b
    ); // 0x152ad0 (real: i32 ret, CSymTab* arg)
}; // 0x152ad0
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/FontConfig.h>
#include <Io/SaveGame.h>
#include <Gruntz/GameLevel.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Gruntz/WwdObjMgr.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Timer.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (m_74/m_spriteFactory @+0x74; LoadSprite)
#include <rva.h>
#include <Gruntz/ResMgr.h>      // CResMgr + its image/sound/anim registries (m_10/m_28/m_2c)
#include <Gruntz/SoundCue.h>    // CSndHost (m_c->m_28) + SoundStream (m_2c; Vslot15 quiesce stop)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (render-flip surface: Fill/Restore)

// The zoned sound-bank manager (CWorld::m_48); RegionEnter/RegionLeave pause +
// resume the currently-playing zoned sound via its real (named) methods.
#include <Dsndmgr/GruntzSoundZ.h>
#include <Globals.h>

// The grid cell / selectable object picked at a screen point (OnMouseUp's marker/
// select dispatch) - the facet of the real CTriggerMgr::CTmCell returned by
// FindGruntAt (0x32ce) and the timeline ScreenToCell (0x3cb0). +0x1ec is its
// owner/team id (compared to g_644c54), +0x1fc a live flag. (CTmCell is fwd-only in
// TriggerMgr.h; fold this into it when its full layout is modeled.)
SIZE_UNKNOWN(CPickedObj);
struct CPickedObj {
    void OnStruck1f4b(i32 flag); // 0x1f4b  select/deselect
    char p0[0x1ec];
    i32 m_1ec; // +0x1ec  owner/team id
    char p1f0[0x1fc - 0x1f0];
    i32 m_1fc; // +0x1fc  live/present flag
};

// The registry's +0x60 on-screen cue sink (CGruntCueSink facet): OnMouseUp posts a
// 6-arg cancel/curse cue (0x39f4) on select/marker changes.
SIZE_UNKNOWN(CCueSink6);
struct CCueSink6 {
    void SpawnVoiceDriver39f4(i32 f, i32 id, i32 a, i32 b, i32 c, i32 d); // 0x39f4
};

// The world's +0x6c marker/waypoint placer (m_4->m_6c): OnMouseUp queues an 8-arg
// place order for the click.
SIZE_UNKNOWN(CMarkerPlacer);
struct CMarkerPlacer {
    void Place(i32, char, i32, i32, i32, i32, i32, i32); // 0x2095
};

// ---- MFC primitives reused verbatim from the engine (reloc-masked). ----
#include <Gruntz/String.h>
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
    // Probe @0x12da IS CPlay::BuildGruntTypeNameTable (extra args reloc-masked); cast at the call.
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

// CPlay::ApplyGameOptions (0x036be0) lives in its home TU per the interval
// dossier (#10c seam): src/Gruntz/VideoConfig.cpp (the options-dialogs TU).
// Its former CGameMgrSettings local view dissolved with it (the canonical
// CGruntzMgr shape serves there).

// CPlay::Update (0x0008c910) is now an inline member in the header.

// The shared HUD message-sprite helper (0x1154b0, __cdecl): pushes a transient
// text sprite carrying `text` into `rect` with the given duration/colour flags
// (idiom shared with BootyMessages.cpp / DrawBattleStats.cpp). reloc-masked.
void ShowHudMessage(
    void* sink,
    CString* text,
    RECT* rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0

// ===========================================================================
// CPlay::FrameSlot28  (vtable slot 10 / +0x28) - the HUD status/pause overlay.
// BYTE-IDENTICAL to CMulti::FrameSlot28 (Multi.cpp).  Freeze m_60, stash the
// game clock, (m_40) run the notify, clear the present surface, then draw the
// LoadString(0x81a9) banner + tick the status message.
// ===========================================================================
// @early-stop
// /GX EH-frame wall (92.75%): full+correct logic, all externs typed/named. Residual
// is (1) the SEH scope-table representation (retail push Unwind@005dde38 / state 8 vs
// cl's push $L.. / state 0 - reloc-masked immediates, docs/seh-eh.md) and (2) cl
// allocates the RECT+CString locals in a 0x14-byte frame vs retail's 0x10, a 4-byte
// /GX frame-packing residue that shifts the [esp+N] stack offsets (not statement-order
// or decl-order steerable - tried both). Permuter candidate for the final sweep.
RVA(0x000c8b80, 0x11b)
i32 CPlay::FrameSlot28(i32 arg) {
    m_4w()->m_60->Method_11c7b0();
    m_savedClock = (i32)g_645588;
    if (m_40) {
        Method_cef50();
    }
    if (arg == 9) {
        return 1;
    }
    RECT r;
    m_c->m_drawTarget->m_18->m_2c->Fill(0);
    CString s;
    s.LoadString(0x81a9);
    r.right = m_4w()->m_8c;
    r.bottom = m_4w()->m_90;
    r.left = 0;
    r.top = 0;
    ShowHudMessage(m_c, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
    Method_fa8f0(0x50, 0x3e8, 0, 1);
    if (m_4w() && m_4w()->m_68) {
        m_4w()->m_68->Method_6bd40(5);
    }
    return 1;
}

// ===========================================================================
// CPlay::Render  (vtable slot +0x14)
// ===========================================================================
// @early-stop
// DIVERGING CARCASS (0%): the control flow + member offsets are faithful but the
// codegen is NOT byte-exact yet - retail is 897 instrs, this lowers to ~737. A
// dedicated rewrite (final sweep) must apply these traced structural fixes before
// the codegen residue is even reachable:
//   * zero-register wall: retail pins the 0-constant in EBP (xor ebp,ebp; ebp free
//     because it is NOT a frame pointer here); cl picks EDI, so every `push 0`/
//     `cmp x,0` differs. Pervasive; caps the score. Not source-steerable.
//   * the per-frame profiler is a __thiscall on m_c->m_frameProfiler (+0x20):
//     `p->Prof1(t); p->Prof2(t)` (0x136e20/0x137ac0), NOT the cdecl Eng_Profiler1/2.
//   * MarkerBegin(now) -> m_beginMarker->Begin(now) (ecx=[esi+0x2e4], call 0x2cc0);
//     GutsStep() -> m_guts->FrameStep() (ecx=[esi+0x2dc], call 0x21b7) - both are
//     sub-object thiscalls, not CPlay-this methods.
//   * FrameTimerBegin(now) -> m_frameMarker->Begin(now) (ecx=[esi+0x3f4], 0x3710);
//     FrameTimerEnd -> m_frameMarker->End(view, 1) (0x27a2), 2 args.
//   * ALL the interval one-shots (cue +0x3f8, booty +0x328, ambient +0x338,
//     win/lose, snapshot +0x4a0) are 64-BIT elapsed tests: retail does
//     `sub ecx,lo; sbb eax,hi; cmp eax,intervalHi; jl/jg; cmp ecx,intervalLo; jb`
//     ((i64)(u32)g_645588 - *(i64*)&m_timerLo >= *(i64*)&m_intervalLo), not 32-bit.
//   * the in-game tail (after the view check) has 2 CPlay this-calls the carcass
//     omits (0x2e2d(clock), 0x1519(view)) + a 3-arg cdecl(g_64556c, m_guts,
//     m_region0Gate) before m_c->m_24->PostStep(); surface flush is a __thiscall on
//     m_2c, not cdecl Eng_SurfaceFlush.
// Also a /GX EH-frame + frame-size residue (0x84 vs 0x6c) once the above land.
// Left as carcass (not touched this pass): a partial rewrite ripples the tight play
// unit's regalloc (StepScroll-style) for no % gain until ALL of the above are done.
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
        StepInputA();         // poll/sim sub-step A
        StepWorldB();         // world/camera sub-step B
        m_c->m_24->PreStep(); // on m_c->m_24 (view pre-step)

        g_6bf3c0 = g_645580; // mirror the draw clock
        g_6bf3bc = g_645584;

        // --- shared world-draw block #1 ---
        ((CRenderer*)m_c->m_8)->BeginScene(0); // m_c->m_8->vtbl[+0x24](0)
        m_c->m_24->PushView(m_c->m_drawTarget->m_14,
                            m_c->m_8); // (thiscall on m_24)
        m_c->m_rendererB->Present(m_c->m_drawTarget->m_14,
                                  m_c->m_drawTarget->m_18); // vtbl[+0x34]
        m_4w()->m_54->Blit(
            ((CGameViewport::CameraGeom*)m_c->m_24->m_5c)->m_84,
            ((CGameViewport::CameraGeom*)m_c->m_24->m_5c)->m_88
        );
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

        if (m_c->m_drawTarget->m_14 == 0) {
            return 1; // no view -> bail
        }

        FrameTimerBegin((i32)g_645584);                     // m_frameMarker begin
        FrameTimerEnd(0, (i32)g_645584);                    // wait: end takes (this,flag)
        Eng_SurfaceFlush(m_c->m_drawTarget->m_10->m_2c, 0); // surface flush
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
            Eng_BeginScene(m_c->m_drawTarget->m_10->m_2c, 0);
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
        m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
        m_c->m_rendererB->Present(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18); // present
        if (m_region1Gate != 0) {
            StepC(); // alt-input draw
        } else {
            m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
            m_c->m_rendererB->Present(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
        }
        MarkerBegin((i32)g_645584);
        GutsStep();
        InputSubStep(w->m_70); // m_4->m_70

        if (m_overlayActive != 0 && m_guts->m_mode != 5) { // on-screen overlay/banner
            Overlay1(0, (i32)g_645584);
            Overlay2(m_c, 0);
        }

        WorldBlit((i32)g_645584); // on m_4->m_5c (thiscall)
        if (m_c->m_drawTarget->m_14 == 0) {
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
                if (g_64556c->m_focusSlots[0].m_0c != 0) {
                    void* out = 0;
                    MapLookup(g_64556c->m_world->m_8, (void*)g_64556c->m_focusSlots[0].m_0c, out);
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
            Eng_HudDraw(m_c->m_drawTarget->m_10->m_2c, &m_hudRect, 0xff); // (this=m_4->m_10->m_2c)
        }
        Eng_SurfaceFlush(m_c->m_drawTarget->m_10->m_2c, 0);

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
    if (m_c->m_drawTarget->m_14 == 0) {
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
            m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
            m_c->m_rendererB->Present(m_c->m_drawTarget->m_14,
                                      m_c->m_drawTarget->m_18); // present
            GutsStep();
            if (m_guts->m_busyA == 0 && m_guts->m_busyB == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0); // win/lose
            }
            FrameTimerEnd(1, 0);
        } else {
            // ---- the active short frame: entity step + cues ----
            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
                m_c->m_rendererB->Present(
                    m_c->m_drawTarget->m_14,
                    m_c->m_drawTarget->m_18
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
        Eng_SurfaceFlush(m_c->m_drawTarget->m_10->m_2c, 0);
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
        ((CDDrawSubMgrPages*)m_c->m_8)->Method_159ef0();
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
// sub-object offsets here differ from Render's CSpriteFactoryHolder typing, so a local view-cast
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
// The worker-list leaf (ClearWorkers @0x163c60); TU-local method view of the real
// ddrawworkerlist-unit class (no header yet).
class CDDrawWorkerList {
public:
    void ClearWorkers();
};
// The submgr leaf (FreeAll_152720 @0x152720); TU-local method view (header-less unit).
class CDDrawSubMgrLeaf {
public:
    void FreeAll_152720();
};
// The view holder (this->m_c) as the exit walk reads it.
struct CExitView {
    char p0[0x8];
    CDDrawSubMgrPages* m_8; // +0x8  (Method_159ef0)
    CDDrawWorkerList* m_c;  // +0xc  (ClearWorkers @0x163c60)
    CExitV58* m_10;         // +0x10  virtual slot 0x58
    char p14[0x24 - 0x14];
    CExitV44* m_24; // +0x24  virtual slot 0x44
    struct M28 {
        char p0[0x2c];
        SoundStream* m_2c; // +0x2c  (Stop @0x137a80)
        // Release @0x157bc0 IS CDDrawSubMgrLeafScan::ClearMap; cast at the call.
    }* m_28;                // +0x28
    CDDrawSubMgrLeaf* m_2c; // +0x2c  (FreeAll_152720 @0x152720)
};
// The world (this->m_4) as the exit walk reads it.
struct CExitWorld {
    char p0[0x48];
    CGruntzSoundZ* m_48; // +0x48  (StopAndFlush @0x138530)
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
            ((CExitView*)m_c)->m_28->m_2c->Stop();
        }
        ((CDDrawSubMgrLeafScan*)((CExitView*)m_c)->m_28)->ClearMap();
    }
    if (m_4) {
        ((CExitWorld*)m_4)->m_48->StopAndFlush();
        ((CWorldSoundSet*)((CExitWorld*)m_4)->m_54)->Resume();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_10->Teardown();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_2c->FreeAll_152720();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_24->Teardown();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_8->Method_159ef0();
    }
    if (m_c) {
        ((CExitView*)m_c)->m_c->ClearWorkers();
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
        m_4->ReportError(0x800a, 0x456);
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
        m_hitTest->Configure(m_guts->m_state == 1 ? 2 : 1);
        return 1;
    }
    return 0;
}

// CPlay::Vslot1c (0xd0050, slot 28 / +0x70) - counts the sprite factory's live objects
// whose collision-category word (CGameObject +0xe8) equals `category`. The factory's
// live-object list container sits at m_c->m_8+0x10 (its head pointer at +0x14 IS
// CSpriteFactory::m_liveObjects); retail walks it through the container, so the head +
// node fields are read via documented offsets (this TU does not pull SpriteFactory.h/
// UserLogic.h) to reproduce the `add 0x10; [+4]` node walk. Node: next@+0, sprite@+8.
RVA(0x000d0050, 0x3a)
i32 CPlay::Vslot1c(i32 category) {
    char* container = (char*)m_c->m_8 + 0x10;
    if (container == 0) {
        return 0;
    }
    char* node = *(char**)(container + 4);
    i32 count = 0;
    while (node != 0) {
        char* p = node;
        node = *(char**)node;
        char* sprite = *(char**)(p + 8);
        if (sprite != 0 && *(u32*)(sprite + 0xe8) == (u32)category) {
            count++;
        }
    }
    return count;
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
    m_c->m_24->SetClipRect(&r);
    m_4->RecomputeViewScale();
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
    CSpriteFactoryHolder* v = m_c;
    CGameViewport::SViewRect* vp = &v->m_24->m_viewport;
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

    m_c->m_24->SetClipRect(&r);
    m_c->m_drawTarget->m_14->m_2c->Fill(0);
    m_guts->ClampApply();
    m_4->RecomputeViewScale();
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
    CSpriteFactoryHolder* v = m_c;
    CWorld* w = m_4w();
    GutsSubsystem* guts = m_guts;

    i32* rp = (i32*)&v->m_24->m_viewport;
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

    m_c->m_24->SetClipRect(&r);
    m_c->m_drawTarget->m_14->m_2c->Fill(0);
    m_guts->ClampApply();
    m_4->RecomputeViewScale();
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
        ((CRegSink*)g_64556c->m_cmdGrid)->Post(-1, 0); // reg->m_68->Post(0xffffffff, 0)
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
// Real polymorphic view: Notify is slot 11 (+0x2c), a real virtual (11 fillers).
struct CVisEntity {
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual void Slot7();
    virtual void Slot8();
    virtual void Slot9();
    virtual void Slot10();
    virtual void Notify(void* held); // slot 11 (+0x2c)
    char p4[0x7c - 0x04];
    CVisEntityType* m_7c; // +0x7c
};
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
    CSpriteFactoryHolder* v = m_c;
    i32* vp = (i32*)&v->m_24->m_viewport;
    CDrawTarget::SurfaceB* held = v->m_drawTarget->m_14;
    CVisNode* node = *(CVisNode**)((char*)((CRenderer*)v->m_8) + 0x14);

    RECT r;
    r.left = vp[0];
    r.top = vp[1];
    r.right = vp[2] + 1;
    r.bottom = vp[3] + 1;
    held->m_2c->Restore(&r, 0);

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
    CGameViewport* v = m_c->m_24;
    CGameViewport::CameraGeom* geom = (CGameViewport::CameraGeom*)v->m_5c;

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
    void* probeTarget = m_c->m_drawTarget->m_14->m_2c;
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
    CGameViewport::SViewRect& vp = m_c->m_24->m_viewport;
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
        if (((CPlay*)&m_cueText)->BuildGruntTypeNameTable(cueId, 0, 0, 0) == 0) {
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
        CGameViewport::SViewRect& vp = m_c->m_24->m_viewport;
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
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollA();
    }
    g_6bf3c0 = g_645580;
    g_6bf3bc = g_645584;
    ((CRenderer*)m_c->m_8)->BeginScene(0); // m_c->m_8->vtbl[+0x24](0)
    m_4w()->m_68->Step((i32)g_645584);     // m_4->m_68 frame-timer step
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
                    ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollB();
                }
            }
            Vslot26(); // this->vtbl[+0x98]()
            if (m_c->m_24->m_5c != 0) {
                ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollA();
            }
            ((CRenderer*)m_c->m_8)->BeginScene(0);
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

// The draw-surface flush sink (m_c->m_drawTarget->m_10->m_2c) torn through a thiscall flush.

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
    m_4w()->m_54->Blit(
        ((CGameViewport::CameraGeom*)m_c->m_24->m_5c)->m_84,
        ((CGameViewport::CameraGeom*)m_c->m_24->m_5c)->m_88
    );
    u32 t2 = tg();
    m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
    m_c->m_rendererB->Present(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
    i32 presentMs = (i32)(tg() - t2);
    ProfLog(
        &g_profSink,
        "Delta=%i, Update=%i, Draw=%i, NumUpdates=%i    ",
        (i32)g_645584,
        renderMs,
        presentMs,
        updates
    );
    DrawDebugStats();
    ((CDDSurface*)m_c->m_drawTarget->m_10->m_2c)->Flip(0);
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollB();
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
    m_4w()->m_54->Blit(
        ((CGameViewport::CameraGeom*)m_c->m_24->m_5c)->m_84,
        ((CGameViewport::CameraGeom*)m_c->m_24->m_5c)->m_88
    ); // untimed
    u32(WINAPI * tg)() = g_pTimeGetTime;

    u32 t1 = tg();
    Vslot26(); // this->vtbl[+0x98]
    i32 activateMs = (i32)(tg() - t1);

    u32 t3 = tg();
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollA();
    }
    i32 deactMs = (i32)(tg() - t3);

    u32 t5 = tg();
    ((CRenderer*)m_c->m_8)->BeginScene(1);
    m_4w()->m_68->Step((i32)g_645584);
    m_guts->Step((i32)g_645584);
    i32 updateMs = (i32)(tg() - t5);

    u32 t7 = tg();
    i32 hitTestMs = (i32)(tg() - t7);

    u32 t9 = tg();
    m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
    i32 drawMs = (i32)(tg() - t9);

    u32 t11 = tg();
    m_c->m_rendererB->Present(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
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

    DrawDebugStats();
    g_profAccB = (i32)tg();
    ((CDDSurface*)m_c->m_drawTarget->m_10->m_2c)->Flip(0);
    g_profAccB = (i32)(tg() - (u32)g_profAccB);
    g_profAccA = (i32)tg();
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollB();
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
    ((CLevelPlane*)pg)->RecomputePlaneCoords();
    return 1;
}

// ===========================================================================
// CPlay::RegisterInputBindings (0x0d9160) - register the nine keyboard control
// bindings (the WM_KEYDOWN-style codes 0x100-0x102 / 0x200-0x206, each with flag
// 0x40) on the world input dispatcher (m_4->m_4).
// ===========================================================================
RVA(0x000d9160, 0xac)
i32 CPlay::RegisterInputBindings() {
    m_4w()->Wnd()->PumpMessages(0x102, 0x40);
    m_4w()->Wnd()->PumpMessages(0x100, 0x40);
    m_4w()->Wnd()->PumpMessages(0x200, 0x40);
    m_4w()->Wnd()->PumpMessages(0x201, 0x40);
    m_4w()->Wnd()->PumpMessages(0x202, 0x40);
    m_4w()->Wnd()->PumpMessages(0x203, 0x40);
    m_4w()->Wnd()->PumpMessages(0x204, 0x40);
    m_4w()->Wnd()->PumpMessages(0x205, 0x40);
    m_4w()->Wnd()->PumpMessages(0x206, 0x40);
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
        && g_64556c->m_frameGate == 0 && g_64556c->m_cmdGrid->m_groupFlag != 0) {
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

// -------------------------------------------------------------------------
// CWorld::WorldTimeline::HudRect (0x78060) - the combat-region scan PostHudRect (above)
// and CPlay::DispatchHudClick invoke on m_4w()->m_68. Re-homed from src/Stub/ApiWrappers
// .cpp (was GruntCombatMgr::CheckCombatRegion): screen-transform the world rect via the
// active viewport, then for each occupied grunt slot whose 30x30 screen box hits the rect,
// either re-arm the local player's grunt (Method_36ed/ResetCell29cd on g_curPlayer) or arm a
// foe's combat state (health sprite + CombatTimeout clock). The GruntCombatMgr placeholder
// view folded onto CWorld::WorldTimeline; its sub-objects are the nested combat structs.
DATA(0x00244c54)
extern i32 g_curPlayer; // 0x644c54  local-player index
// @early-stop
// regalloc/CSE wall (~80% - and 0x78060 is not play's .obj, so the frame is re-scored):
// logic + instruction selection match, but cl pins `this`->ebx (retail ebp) and CSEs
// view->m_viewport once where retail reloads it per rect pair (a symmetric ebx<->ebp swap).
RVA(0x00078060, 0x18d)
void CWorld::WorldTimeline::HudRect(RECT r, i32 flag) {
    CombatView* view = m_viewHost->m_view;
    r.left += view->m_viewport->m_rect.left - view->m_originX;
    r.top += view->m_viewport->m_rect.top - view->m_originY;
    r.right += view->m_viewport->m_rect.left - view->m_originX;
    r.bottom += view->m_viewport->m_rect.top - view->m_originY;
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++) {
            CombatGrunt* g = m_grunts[j];
            if (g) {
                i32 cx = g->m_pos->m_screenX;
                i32 cy = g->m_pos->m_screenY;
                RECT box;
                SetRect(&box, cx - 0xf, cy - 0xf, cx + 0xf, cy + 0xf);
                if (r.left <= box.right && r.right >= box.left && r.top <= box.bottom
                    && r.bottom >= box.top) {
                    if (i == g_curPlayer) {
                        if (flag == 0 && g->m_1fc != 0) {
                            Method_36ed();
                            flag = 1;
                        }
                        ResetCell29cd(g_curPlayer, j, 1, 1);
                    } else {
                        g->CreateHealthSprite();
                        g->m_combatTimeout =
                            g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
                        g->m_88c = 0;
                        g->m_880 = g_645588;
                        g->m_884 = 0;
                    }
                }
            }
        }
    }
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
    CGameViewport::SViewRect& vp = m_c->m_24->m_viewport;
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
i32 CPlay::BeginGridWalk(const char* key, i32 index, i32 e8, i32 delay, i32 hasGrid) {
    if (m_c == 0) {
        return 1;
    }
    CFrameGrid* grid = 0;
    // frame-grid probe into the image registry's name->object map (frame-grid Lookup overload).
    ((CMapStringToOb*)&m_c->m_10->m_10map)->Lookup(key, (CObject*&)grid);
    m_grid = grid;
    if (grid == 0) {
        return 1;
    }
    m_gridHasSprite = hasGrid;
    if (hasGrid != 0) {
        CWorld* w = m_4w();
        i32 id = g_644c54;
        void* spr = w->m_74->LoadSprite(*(void**)(w->m_158 + (id * 0x47) * 8), 0);
        if (spr == 0) {
            spr = g_64556c->m_spriteFactory->LoadSprite(spr, 1);
        }
        ((CImageSet*)m_grid)->SetAllTypes(0xa);
        ((CImageSet*)m_grid)->SetAllFormats((i32)spr);
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
    CGameViewport::SViewRect box = m_c->m_24->m_viewport;
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
        CGameViewport* v = m_c->m_24;
        i32 wx = ((CGameViewport::CameraGeom*)v->m_5c)->m_originX - v->m_viewport.left + x;
        i32 wy = ((CGameViewport::CameraGeom*)v->m_5c)->m_originY - v->m_viewport.top + y;
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
    void* view = m_c->m_drawTarget->m_14;
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
void CPlay::PlayBacklog08c9d0() {}

// CState::SetBeginClearParams (0x8c970) - seed the begin-clear params.
RVA(0x0008c970, 0x1c)
i32 CState::SetBeginClearParams(i32 unused, i32 arg2, i32 arg3) {
    m_cursorX = arg2;
    m_cursorY = arg3;
    return 1;
}

// The cached PostMessageA fn-ptr (bare 0x6c44c8; the paused-frame unpause posts
// WM_COMMAND 0x816e through it). Same decl used by the Dispatch cluster below.
extern i32(WINAPI* g_pPostMessageA)(HWND, UINT, WPARAM, LPARAM);

// ===========================================================================
// CPlay::OnMouseUp (0x0cdb10, Ghidra-named winapi_0cdb10_PostMessageA after the
// PostMessageA it fires) - the menu/pause-state mouse button-UP / drag-release
// handler, sibling of OnKeyCommand/HandleTileClick: the same hudSuppressed /
// renderDisabled(resume) / inGame(reset-or-report 0x457) / paused(unpause) /
// overlayDrag priority chain, then (no active grunt) the marker/waypoint place,
// drag-box and grunt-pick dispatch. Non-EH; __thiscall(a, x, y), ret 0xc; every
// path returns 1 except the overlay-drag guts dispatch tail (returns its result).
// ===========================================================================
// @early-stop
// full+correct reconstruction (was a bare `return 0` stub; 0.29% -> 34.9%). Six
// steerable codegen fixes landed on the correct structure:
//   1. nest the no-active-grunt body deepest so the guts-dispatch cold block floats
//      to the tail (nested-if-success-deepest-error-tail.md): 27 -> 30
//   2. byte-widen the g_644c54 marker arg via a `char` Place param (mov cl,[g] +
//      store-byte/read-dword): 30 -> 32.2
//   3. order waypoint_cancel before drag_path per retail (cdf36 < cdf6c)
//   4. route the drag/pick/reset `return 1`s to a shared `ret1:` tail (retail's
//      0xce2e9), matching its 23 vs my former 25 epilogues: 32.2 -> 33.3
//   5. cache arg1 x in a pure-read local `xr` for the pre-snap value reads so cl
//      promotes it to EBX like retail (the arg slot is modified in place by the &x
//      snap/place calls): 33.3 -> 34.6  (drag_box's first rect-test x stays stack)
//   6. size FindGruntAt's span-output arg as an 8-byte buffer (2 ints), not a 4-byte
//      void* - this makes cl reserve retail's 0x20 frame (was 0x1c), so every
//      [esp+arg] aligns: 34.6 -> 34.9. The "frame-coalesce wall" was a sizing bug.
// Residual (regalloc, not source-steerable): the marker/drag blocks assign sx/sy to
// edi/ecx where retail uses ebp/edi, and cl spills the cached CWorld* to [esp+0x10]
// where retail re-reads m_4 (removing the cache regressed 34.9->33.3 - the re-read
// cascades worse); plus m_5c `add 0x40`/`[+0x40]` addressing-mode coin-flips.
// Final-sweep/permuter candidate.
RVA(0x000cdb10, 0x80c)
i32 CPlay::winapi_0cdb10_PostMessageA(i32 a, i32 x, i32 y) {
    // retail keeps the ORIGINAL click x in EBX for the pre-snap value reads while
    // the arg slot [esp+0x38] is modified in place by the snap/place calls (they
    // take &x). Declared before any goto so no init is skipped.
    i32 xr = x;
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
            goto ret1;
        }
        m_4->ReportError(0x800a, 0x457);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        g_pPostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x816e, 0);
        return 1;
    }
    // The overlay-drag / command-grid-idle gates route to the guts dispatch tail
    // (a forward je all the way down); nest the no-active-grunt main body deepest
    // so retail's cold-block-at-tail layout falls out (nested-if-success-deepest).
    if (m_overlayDrag == 0 && g_64556c->m_cmdGrid->m_groupFlag != 0) {
        if (m_4w()->m_c != 0) {
            goto drag_path;
        }

        // ---- no active grunt: overlay probe, then marker/waypoint place ----
        if (m_overlayActive != 0 && m_guts->m_state != 2 && m_guts->m_mode != 5) {
            if (((CPlay*)(void*)m_overlayActive)->ApplyOverlay3e59(a, xr, y)) {
                return 1;
            }
        }
        CWorld* w = m_4w();
        CWorld::RenderStateHolder::PlaneGeomHolder* geom = w->m_30->m_24;
        CGameViewport::CameraGeom* cam = (CGameViewport::CameraGeom*)geom->m_5c;
        i32 sx = cam->m_originX - geom->m_rect10.left + xr;
        i32 sy = cam->m_originY - geom->m_rect10.top + y;
        if (m_dragInhibit1 == 0) {
            goto mode_36c;
        }
        if (m_4f0 != 0) {
            goto mode_36c;
        }
        i32 placed = 0;
        RECT* gr = &m_guts->m_rect10;
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            // inside the guts HUD rect -> finalize with placed == 0
        } else {
            RECT* wr = &geom->m_rect10;
            if (xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top) {
                if (FindStartPointAt309e(sx, sy, &x, &y)) {
                    char tok = *(char*)&g_644c54;
                    ((CMarkerPlacer*)w->m_6c)->Place(1, tok, 0, 0, x, y, 0, 0);
                    placed = 1;
                }
            }
        }
        if (placed == 0) {
            ((CCueSink6*)g_64556c->m_cueSink)->SpawnVoiceDriver39f4(placed, 0x340, -1, 1, -1, -1);
        }
        m_dragInhibit1 = 0;
        m_guts->CommitSlot142e(placed);
        SetCursorFrame(0);
        return 1;
    } else {
        goto guts_dispatch;
    }

mode_36c:
    if (m_dragInhibit2 == 0) {
        goto drag_path;
    }
    if (m_4f0 != 0) {
        goto drag_path;
    }
    {
        RECT* gr = &m_guts->m_rect10;
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            if (m_guts->SetFallRect1442(xr, y, *(char*)((char*)this + 0x2f4))) {
                m_dragInhibit2 = 0;
                SetCursorFrame(0);
                return 1;
            }
            goto waypoint_cancel;
        }
        CWorld* w = m_4w();
        CWorld::RenderStateHolder::PlaneGeomHolder* geom = w->m_30->m_24;
        RECT* wr = &geom->m_rect10;
        if (!(xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top)) {
            goto waypoint_cancel;
        }
        // inside the world rect: place a waypoint through the trigger grid
        CGameViewport* ds = m_c->m_24;
        CGameViewport::CameraGeom* cam = (CGameViewport::CameraGeom*)ds->m_5c;
        i32 wx = cam->m_originX - ds->m_viewport.left + xr;
        i32 wy = cam->m_originY - ds->m_viewport.top + y;
        i32 tok = *(char*)(0x2f4 + (char*)this);
        if (g_64556c->m_cmdGrid->CellHitTest(wx, wy, &x, &y, tok) != 0) {
            ((CMarkerPlacer*)w->m_6c)->Place(1, a, y, 8, 0, 0, tok, 0);
            m_4f0 = 1;
            return 1;
        }
        // cdea2: nothing placed -> pick a grunt in a 30x30 world box via the grid
        RECT box;
        box.left = wx - 0xf;
        box.top = wy - 0xf;
        box.right = wx + 0xf;
        box.bottom = wy + 0xf;
        i32 out28[2] = {0, 0};
        i32 col = 0;
        CTmCell* p = g_64556c->m_cmdGrid->FindGruntAt(wx, wy, (RECT*)out28, &col, &y, &box);
        if (p == 0 || g_644c54 != ((CPickedObj*)p)->m_1ec) {
            goto waypoint_cancel;
        }
        ((CMarkerPlacer*)w->m_6c)->Place(1, a, y, 8, 0, 0, tok, 0);
        return 1;
    }

waypoint_cancel:
    m_dragInhibit2 = 0;
    m_guts->EnterHlRow213f(0, *(char*)((char*)this + 0x2f4));
    SetCursorFrame(0);
    return 1;

drag_path: {
    // m_4w()->m_c != 0: an active grunt is selected -> drag / guts dispatch
    (void)y;
    if (m_guts == 0) {
        return 1;
    }
    if (m_guts->m_state == 2) {
        if (m_guts->HitTestLayer1c44(xr, y)) {
            m_dragSnapActive = 1;
            void* g8 = *(void**)((char*)m_guts + 8);
            i32 dx = 0;
            if (g8 != 0) {
                dx = *(i32*)((char*)g8 + 0x5c) - xr;
            }
            *(i32*)((char*)this + 0x158) = dx;
            void* g8b = *(void**)((char*)m_guts + 8);
            if (g8b == 0) {
                *(i32*)((char*)this + 0x15c) = 0;
                return 1;
            }
            *(i32*)((char*)this + 0x15c) = *(i32*)((char*)g8b + 0x60) - y;
            return 1;
        }
        goto drag_box;
    }
    // m_guts->m_state != 2: guts-rect dispatch
    RECT* gr = &m_guts->m_rect10;
    if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
        Helper2c7f();
        return m_guts->UpdateStatusBarTabHl428c(a, xr, y);
    }
    if (m_hitTest->HitTest43e0(xr, y)) {
        return 1;
    }
}

drag_box: {
    if (m_4w()->m_c != 0) {
        goto ret1;
    }
    CWorld* w = m_4w();
    RECT* wr = &w->m_30->m_24->m_rect10;
    if (!(x < wr->right && x >= wr->left && y < wr->bottom)) {
        goto ret1;
    }
    if (y < wr->top) {
        return 1;
    }
    // inside the world rect
    if (m_dragEndNotify != 0) {
        i32 ex = (y & ~0x1f) + 0x10;
        i32 ey = (y & ~0x1f) + 0x10;
        i32 lv = m_levelId - 0xc8;
        if (lv <= 0x16) {
            g_64556c->m_cmdGrid->ResetGroup(ex, ey, 0, 0, 0, 2, 1);
        } else if (lv >= 0x17 && lv <= 0x20) {
            g_64556c->m_cmdGrid->ResetGroup(ex, ey, 0, 0, 0, 3, 1);
        }
        g_64556c->m_cmdGrid->m_pendingFxKind = 0;
        ClearDragBoxes(0, 0);
        m_dragClampMaxX = xr;
        m_dragClampMaxY = y;
        m_hudRect.left = xr;
        m_hudRect.top = y;
        m_hudRect.right = xr;
        m_hudRect.bottom = y;
        m_worldReady = 1;
        return 1;
    }
    {
        i32 ex = (y & ~0x1f) + 0x10;
        i32 ey = (y & ~0x1f) + 0x10;
        if (g_64556c->m_cmdGrid->TriggerCell(ex, ey)) {
            return 1;
        }
    }
    // ce191: level-gated curse cue + waypoint queue
    if (m_levelId >= 0xc8) {
        CTriggerMgr* cg = g_64556c->m_cmdGrid;
        CTmCell* slot = 0;
        if (1 == cg->m_recList.m_count) { // exactly one record node
            i32* sel = *(i32**)(*(char**)((char*)&cg->m_recList + 4) + 8);
            slot = cg->m_grid[sel[1] * 15 + sel[0]];
        }
        if (slot != 0 && ((CPickedObj*)slot)->m_1fc != 0) {
            ((CCueSink6*)g_64556c->m_cueSink)
                ->SpawnVoiceDriver39f4((i32)slot, 0x324, -1, 0, -1, -1);
        }
    }
    ClearDragBoxes(0, 0);
    i32 hit = m_guts->HitTest3ad5(xr, y);
    if (hit != -1) {
        m_guts->PlaceCursorTarget20b8(hit, 0);
        return 1;
    }
    // ce22f: pick a grunt at the point
    i32 slot38 = 0;
    CPickedObj* picked = (CPickedObj*)m_4w()->m_68->ScreenToCell3cb0(xr, y, &slot38, &slot38, 5);
    if (picked == 0) {
        m_dragClampMaxX = xr;
        m_dragClampMaxY = y;
        m_hudRect.left = xr;
        m_hudRect.right = xr;
        m_hudRect.top = y;
        m_hudRect.bottom = y;
        m_worldReady = 1;
        goto ret1;
    }
    m_4w()->m_68->ResetCell29cd(slot38, slot38, g_645578->m_18 & 0x20, 0);
    if (a == g_644c54) {
        if (0 != (g_645578->m_18 & 0x20)) {
            goto ret1;
        }
        picked->OnStruck1f4b(1);
        return 1;
    }
    picked->OnStruck1f4b(0);
    return 1;
}

ret1:
    return 1;

guts_dispatch:
    return m_guts->UpdateStatusBarTabHl428c(a, x, y);
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
    // Probe @0x12da IS CPlay::BuildGruntTypeNameTable (4th arg reloc-masked); cast at the call.
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
        m_4->ReportError(0x800a, 0x458);
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
    if (g_64556c->m_cmdGrid->m_groupFlag == 0) { // reg->m_68 cue-sink busy gate
        return 1;
    }
    if (m_4w()->m_c != 0) {
        return 1;
    }
    if (m_overlayActive != 0 && m_guts->m_state != 2 && m_guts->m_mode != 5) {
        if (((CPlay*)(void*)m_overlayActive)->BuildGruntTypeNameTable(a, x, y, 0)) {
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
        CGameViewport* ds = m_c->m_24;
        CGameViewport::CameraGeom* geom = (CGameViewport::CameraGeom*)ds->m_5c;
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
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollB();
    }
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollA();
    }
    ((CRenderer*)m_c->m_8)->BeginScene(1);
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollB();
    }
    if (m_c->m_24->m_5c != 0) {
        ((CPlaneRender*)m_c->m_24->m_5c)->CenterScrollA();
    }
    ((CRenderer*)m_c->m_8)->BeginScene(1);
    m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
    m_c->m_rendererB->Present(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
    m_4->PerFrameTick();
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
        if (((CGruntzMgr*)w)->SetVideoMode(savedW, savedH, 1) == 0) {
            return 0;
        }
    }
    if (m_guts != 0) {
        m_guts->ClampApply();
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_c->m_24->PushView(m_c->m_drawTarget->m_14, m_c->m_8);
            m_c->m_rendererB->Present(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
        }
        Eng_SurfaceFlush(m_c->m_drawTarget->m_10->m_2c, 0);
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

// ForwardReady (0x0cee70): tiny vtable forwarder that tail-calls the slot-3 ready
// gate (Vfunc3). Out-of-line (retail emits it standalone; the inline member folded
// into its callers and never emitted).
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

// ---------------------------------------------------------------------------
// 0x0cef50 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). Teardown:
// destruct the +0x04 owner's +0xc8 CObList; when +0x1c0 is live, run the +0x0c
// worker-mgr close (Method_158d20 -> Method_158e40) and dispatch the manager's
// ChangeState(3). Returns 1. Uses this TU's real CDDrawSubMgrPages/CGruntzMgr/
// CObList; only the receiver + its +0x0c holder are orphan views (owner unrecovered).
struct CMid_cef50 {
    char pad0[4];
    CDDrawSubMgrPages* m_4; // +0x04 the worker manager (Method_158d20/158e40)
};
struct Ccef50 {
    char pad0[4];
    char* m_4; // +0x04 owner (its +0xc8 CObList + a CGruntzMgr view)
    char pad8[0xc - 8];
    CMid_cef50* m_c; // +0x0c
    char pad10[0x1c0 - 0x10];
    i32 m_1c0; // +0x1c0
    i32 Teardown();
};
RVA(0x000cef50, 0x46)
i32 Ccef50::Teardown() {
    ((CObList*)(m_4 + 0xc8))->~CObList();
    if (m_1c0 != 0) {
        if (m_c->m_4->Method_158d20() != 0) {
            m_c->m_4->Method_158e40();
        }
        ((CGruntzMgr*)m_4)->ChangeState_8fab0(3);
    }
    return 1;
}
SIZE_UNKNOWN(CMid_cef50);
SIZE_UNKNOWN(Ccef50);

// CPlay::Vslot15 (0x0cfbd0) - vtable slot 21 (override of CState), the level-quiesce
// dispatch. On level index 0x20: latch the quiesce flags (m_1c0/m_40), stop the current
// zoned sound stream (m_c->m_28->m_2c, SoundStream::Stop) + flush the sound bank
// (m_4->m_48, CGruntzSoundZ::StopAndFlush), reset the two world teardown sub-objects
// (m_4->m_54/m_60), then PostMessageA WM_COMMAND 0x8023. Otherwise re-post 0x8023 while
// the m_1bc gate is set, else advance via the manager (m_4->Post, level index + 1). The
// PostMessageA calls go through the cached g_pPostMessageA fn-ptr (bare 0x6c44c8, no
// import symbol). Re-homed from the ApiCaller stubs (was Dispatcher_0cfbd0::Dispatch).
DATA(0x002c44c8)
extern i32(WINAPI* g_pPostMessageA)(HWND, UINT, WPARAM, LPARAM);
// @early-stop
// regalloc-rotation wall (98.4%): logic, instruction selection, hop counts and
// order are byte-identical (llvm-objdump -dr / sema disasm --diff). The only
// residual is the scratch register that holds the reloaded m_4 (CWorld) pointer
// across the three quiesce sub-calls - retail colors it ecx/edx/eax, cl picks
// edx/eax/ecx (a symmetric reload coin-flip); the same rotation shifts the
// PostMessageA HWND chain's registers. Not source-steerable.
RVA(0x000cfbd0, 0x8f)
i32 CPlay::Vslot15() {
    if (m_levelIndex == 0x20) {
        m_1c0 = 1;
        m_40 = 1;
        SoundStream* stream = m_c->m_28->m_2c;
        if (stream) {
            stream->Stop();
        }
        m_4w()->m_48->StopAndFlush();
        m_4w()->m_54->Reset();
        m_4w()->m_60->Reset();
        g_pPostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x8023, 0);
        return 1;
    }
    if (m_1bc) {
        g_pPostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x8023, 0);
        return 1;
    }
    m_4->Post(m_levelIndex + 1);
    return 1;
}

// LoadCursorSprites (0xd0120): select + load the on-screen cursor sprite set for a tool
// `frame`. Early-outs when the requested (frame,flag) already matches the loaded pair.
// Frame 1..0x26 = the numeric chip cursor; 0 = the plain pointer; 0x66 = the flailing-grunt
// cursor (which also fires a booty cue + arms the +0x328 one-shot timer); 0xc8..0xe8 = the
// per-tool cursor table (a dense switch, one GAME_CURSORZ_* per tool). Each path loads via
// the reloc-masked LoadCursor helper (0x39ea, == CPlay::BeginGridWalk) and, on success,
// stamps the cursor state on CPlay's real members (m_dragClampMaxX/Y, m_dragEndNotify,
// m_levelId; the flailing-grunt one-shot reuses the booty-timer block at +0x328).
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
    if (this->m_levelId == frame && flag == this->m_dragEndNotify) {
        return 1;
    }
    if (frame >= 1 && frame <= 0x26) {
        if (this->BeginGridWalk("GAME_INGAMEICONZ_NORMCHIPZ", frame, 0, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != 0) {
            this->m_scrollSink->m_flags |= 1;
        }
        this->m_dragClampMaxX = 0;
        this->m_dragClampMaxY = 0;
        this->m_dragInhibit2 = 1;
        this->m_dragEndNotify = 0;
        this->m_levelId = frame;
        return 1;
    }
    if (frame == 0) {
        if (this->BeginGridWalk("GAME_CURSORZ_POINTER", 1, 1, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != 0) {
            this->m_scrollSink->m_flags &= ~1;
        }
        this->m_dragClampMaxX = 0x10;
        this->m_dragClampMaxY = 0x10;
        this->m_dragEndNotify = 0;
        this->m_levelId = frame;
        return 1;
    }
    if (frame == 0x66) {
        if (this->BeginGridWalk("GAME_CURSORZ_FLAILINGGRUNT", 1, 1, 0x64, 1) == 0) {
            return 0;
        }
        if (this->m_scrollSink != 0) {
            this->m_scrollSink->m_flags |= 1;
        }
        this->m_dragClampMaxX = 0;
        this->m_dragClampMaxY = 0;
        this->m_dragInhibit1 = 1;
        this->m_dragEndNotify = 0;
        ((CGruntSpawnConfig*)g_64556c->m_cueSink)->SpawnVoiceDriver(0, 0x33e, -1, 1, -1, -1);
        this->m_bootyInterval = 0x2710;
        this->m_bootyIntervalHi = 0;
        this->m_bootyTimerLo = g_645588;
        this->m_bootyTimerHi = 0;
        this->m_levelId = frame;
        return 1;
    }
    switch (frame) {
        case 0xc8:
            if (this->BeginGridWalk("GAME_CURSORZ_HANDZ", 1, flag, 0x64, 1) == 0) {
                return 0;
            }
            break;
        case 0xc9:
            if (this->BeginGridWalk("GAME_CURSORZ_BOMBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xca:
            if (this->BeginGridWalk("GAME_CURSORZ_BOOMERANGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcb:
            if (this->BeginGridWalk("GAME_CURSORZ_BRICKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcc:
            if (this->BeginGridWalk("GAME_CURSORZ_CLUBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcd:
            if (this->BeginGridWalk("GAME_CURSORZ_GAUNTLETZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xce:
            if (this->BeginGridWalk("GAME_CURSORZ_GLOVEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xcf:
            if (this->BeginGridWalk("GAME_CURSORZ_GOOBERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd0:
            if (this->BeginGridWalk("GAME_CURSORZ_GRAVITYBOOTZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd1:
            if (this->BeginGridWalk("GAME_CURSORZ_GUNHATZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd2:
            if (this->BeginGridWalk("GAME_CURSORZ_NERFGUNZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd3:
            if (this->BeginGridWalk("GAME_CURSORZ_ROCKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd4:
            if (this->BeginGridWalk("GAME_CURSORZ_SHIELDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd5:
            if (this->BeginGridWalk("GAME_CURSORZ_SHOVELZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd6:
            if (this->BeginGridWalk("GAME_CURSORZ_SPRINGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd7:
            if (this->BeginGridWalk("GAME_CURSORZ_SPYZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd8:
            if (this->BeginGridWalk("GAME_CURSORZ_SWORDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xd9:
            if (this->BeginGridWalk("GAME_CURSORZ_TIMEBOMBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xda:
            if (this->BeginGridWalk("GAME_CURSORZ_TOOBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdb:
            if (this->BeginGridWalk("GAME_CURSORZ_WANDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdc:
            if (this->BeginGridWalk("GAME_CURSORZ_WARPSTONEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdd:
            if (this->BeginGridWalk("GAME_CURSORZ_WELDERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xde:
            if (this->BeginGridWalk("GAME_CURSORZ_WINGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xdf:
            if (this->BeginGridWalk("GAME_CURSORZ_BABYWALKERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe0:
            if (this->BeginGridWalk("GAME_CURSORZ_BEACHBALLZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe1:
            if (this->BeginGridWalk("GAME_CURSORZ_BIGWHEELZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe2:
            if (this->BeginGridWalk("GAME_CURSORZ_GOKARTZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe3:
            if (this->BeginGridWalk("GAME_CURSORZ_JACKINTHEBOXZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe4:
            if (this->BeginGridWalk("GAME_CURSORZ_JUMPROPEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe5:
            if (this->BeginGridWalk("GAME_CURSORZ_POGOSTICKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe6:
            if (this->BeginGridWalk("GAME_CURSORZ_SCROLLZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe7:
            if (this->BeginGridWalk("GAME_CURSORZ_SQUEAKTOYZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case 0xe8:
            if (this->BeginGridWalk("GAME_CURSORZ_YOYOZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (this->m_scrollSink != 0) {
        this->m_scrollSink->m_flags |= 1;
    }
    this->m_dragClampMaxX = 0;
    this->m_dragClampMaxY = 0;
    this->m_dragEndNotify = flag;
    this->m_levelId = frame;
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

    CPlay* self = this;
    ScrollWorld* w = (ScrollWorld*)self->m_4;
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
        self->ResetGoals(sx, sy);
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

// CResSource (the named-set source: this->m_levelBank / m_gameBank, the banks cached
// in m_gruntzBank/m_gameBank; LookupSet 0x13bae0) and CBankMgr (the bank manager at
// this->m_8; Lookup 0x13c030) are the shared CState bank/source facet - one definition
// in <Gruntz/BankMgr.h> (also used by CSplashState/BacklogStateLoaders). Included here
// in-place (declaration order preserved) rather than at the top so the fold stays
// codegen-neutral for this TU.
#include <Gruntz/BankMgr.h>
class CSoundFxEmitter {
public:
    i32 Method_fa8f0(i32 a, i32 b, i32 c, i32 d);
};
// (0xfa150 cleanup is CGameModeBase::BaseCleanup - reached via the CState<->CGameModeBase
// reinterpret at offset 0, the same pattern CState.h uses; no local view needed.)
// The loader family reaches its resource state directly through `this` (a CPlay):
// the bank manager (CState::m_8), the level/GRUNTZ/GAME banks (CState::m_levelBank/
// m_gruntzBank/m_gameBank) and the shared CSpriteFactoryHolder resource registries (CState::m_c->m_10/m_28/m_2c).

// LoadImageBanks (0x0cffe0) - cache the GRUNTZ + GAME asset banks off m_8 into
// m_30/m_34; the int (BOOL) return reuses the just-loaded value at each guard.
RVA(0x000cffe0, 0x3c)
i32 CPlay::LoadImageBanks() {
    CPlay* self = this;
    if (!self->m_8) {
        return 0;
    }
    self->m_gruntzBank = (CSymTab*)((CSymParser*)self->m_8)->ResolvePath("GRUNTZ");
    if (!self->m_gruntzBank) {
        return 0;
    }
    self->m_gameBank = (CSymTab*)((CSymParser*)self->m_8)->ResolvePath("GAME");
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
    if (!force && ((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("ACTION")) {
        return 1;
    }

    ((CDDrawWorkerRegistry*)self->m_c->m_10)->RemoveKeysEqual_155360("ACTION", g_emptyString);
    ((CDDrawWorkerRegistry*)self->m_c->m_10)->RemoveKeysEqual_155360("BACK", g_emptyString);
    g_resourceInstallActive = 0;

    void* tiles = (self->m_levelBank)->ResolvePath("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_c->m_10->Install(tiles, g_emptyString, "_");
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
    if (!force && ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("LEVEL")) {
        return 1;
    }

    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->RemoveKeysEqual_157c70("LEVEL", "_");

    void* sounds = (self->m_levelBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->ScanTree_157ee0((DirNode*)sounds, "LEVEL", "_");
    return 1;
}

// ---------------------------------------------------------------------------
// 0x0db750 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). "LEVEL"
// config sync through the +0x0c owner's +0x2c config leaf (the 0x152xxx registry
// API - distinct from this TU's 0x157xxx CDDrawSubMgrLeafScan). @orphan (owner
// unrecovered; the 0x152xxx leaf + symtab kept as unique local views).
struct Cdb750Cfg {
    i32 HasKeyPrefix_152c50(const char* key);                   // 0x152c50 (ret != 0 tested)
    i32 RemoveKeysEqual_1527d0(const char* key, const char* v); // 0x1527d0
    void ScanTree_152ad0(void* val, const char* key, void* v);  // 0x152ad0
};
struct CHolder_db750 {
    char pad0[0x2c];
    Cdb750Cfg* m_2c; // +0x2c
};
struct Cdb750SymTab {
    void* ResolvePath(const char* arg); // 0x13bae0
};
struct Cdb750 {
    char pad0[0xc];
    CHolder_db750* m_c; // +0x0c
    char pad10[0x28 - 0x10];
    Cdb750SymTab* m_28; // +0x28
    i32 SyncLevelKey(void* arg);
};
RVA(0x000db750, 0x70)
i32 Cdb750::SyncLevelKey(void* arg) {
    if (m_c == 0) {
        return 0;
    }
    if (arg == 0) {
        if (m_c->m_2c->HasKeyPrefix_152c50("LEVEL") != 0) {
            return 1;
        }
    }
    m_c->m_2c->RemoveKeysEqual_1527d0("LEVEL", (const char*)&g_dat60b588);
    void* e = m_28->ResolvePath((const char*)&g_dat613054);
    if (e == 0) {
        return 0;
    }
    m_c->m_2c->ScanTree_152ad0(e, "LEVEL", &g_dat60b588);
    return 1;
}
SIZE_UNKNOWN(Cdb750Cfg);
SIZE_UNKNOWN(CHolder_db750);
SIZE_UNKNOWN(Cdb750SymTab);
SIZE_UNKNOWN(Cdb750);

// LoadLevelImages (0x0db7e0) - sibling of LoadActionTileSprites; a single Register
// (LEVEL), then install the level's IMAGEZ set through the +0x48 virtual slot.
// Brackets the install with two install-active resets.
RVA(0x000db7e0, 0x84)
i32 CPlay::LoadLevelImages(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && ((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("LEVEL")) {
        return 1;
    }

    ((CDDrawWorkerRegistry*)self->m_c->m_10)->RemoveKeysEqual_155360("LEVEL", "_");
    g_resourceInstallActive = 0;

    void* images = (self->m_levelBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_10->Install(images, "LEVEL", "_");
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
    if (((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GAME")) {
        return 1;
    }

    g_resourceInstallActive = 1;
    void* images = (self->m_gameBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_10->Install(images, "GAME", "_");
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
    if (((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GAME")) {
        return 1;
    }

    void* sounds = (self->m_gameBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->ScanTree_157ee0((DirNode*)sounds, "GAME", "_");
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
    if (((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)->HasKeyEqual_155550("GAME")) {
        return 1;
    }

    void* anims = (self->m_gameBank)->ResolvePath("ANIZ");
    if (!anims) {
        return 0;
    }
    ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)->ScanTree_152ad0((CSymTab*)anims, "GAME", "_");
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
// A resolved music-category entry (CSymTab::Insert result): +0xc install key + a
// BeginParse() loader. CParseSource::BeginParse @0x139960 loads the entry (the entry
// IS the parse source); header-less local decl. The level/game MIDIZ banks are the
// real CState::m_levelBank/m_gameBank CSymTab members (read directly, no view).
struct CParseSource {
    char p0[0xc];
    void* m_c;        // +0xc  install key
    i32 BeginParse(); // 0x139960
};
// The destination category table (the sound manager at m_4->m_48).
struct CMusicCatTable {
    void Clear();                                   // 0x538530 (thiscall, no arg)
    void Install(void* res, void* key, char* name); // 0x538670 (thiscall)
};
// Typed view of `this` for the m_4->m_48 sound category table.
struct CMusicOwner {
    char p0[0x4];
    struct SndHolder {
        char p0[0x48];
        CMusicCatTable* m_48; // +0x48
    }* m_4;                   // +0x04
};

#define MUSIC_TAG_XMI 0x584d49 // 'XMI'

RVA(0x000dba30, 0x1ca)
i32 CPlay::BuildMusicCategoryTable(i32) {
    CMusicOwner* self = (CMusicOwner*)this;
    self->m_4->m_48->Clear();

    CSymTab* levelSet = (CSymTab*)m_levelBank->ResolvePath("MIDIZ");
    if (levelSet) {
        CParseSource* e = (CParseSource*)levelSet->Insert("AMBIENT0", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "AMBIENT0");
            }
        }
        e = (CParseSource*)levelSet->Insert("AMBIENT1", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "AMBIENT1");
            }
        }
        e = (CParseSource*)levelSet->Insert("INTRO0", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "INTRO0");
            }
        }
        e = (CParseSource*)levelSet->Insert("INTRO1", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "INTRO1");
            }
        }
    }

    CSymTab* gameSet = (CSymTab*)m_gameBank->ResolvePath("MIDIZ");
    if (gameSet) {
        CParseSource* e = (CParseSource*)gameSet->Insert("POWERUP", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "POWERUP");
            }
        }
        e = (CParseSource*)gameSet->Insert("CURSE", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                self->m_4->m_48->Install(res, e->m_c, "CURSE");
            }
        }
        e = (CParseSource*)gameSet->Insert("MONOLITH", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
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
// The load-notify sink IS a CMulti; OnLoaded @0xbc420 = CMulti::AckJoinFailure. Minimal local decl.
SIZE_UNKNOWN(CMulti);
class CMulti {
public:
    void AckJoinFailure();
};

RVA(0x000dd830, 0x1e3)
i32 CPlay::LoadGruntSoundNamespaces(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }

    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_NORMALGRUNT");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_NORMALGRUNT", "_");
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_DEATHZ");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_DEATHZ", "_");
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_ENTRANCEZ");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_ENTRANCEZ", "_");
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_EXITZ");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_EXITZ", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_GRUNTPUDDLE");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_GRUNTPUDDLE", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_PICKUPS");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_PICKUPS", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_BOMBGRUNT");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((DirNode*)s, "GRUNTZ_BOMBGRUNT", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
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
i32 CPlay::BuildSpriteImageKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    g_resourceInstallActive = 1;
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->Install(s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
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
i32 CPlay::BuildAnizKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)
             ->HasKeyEqual_155550("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)->HasKeyEqual_155550("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_DEATHZ");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)
             ->HasKeyEqual_155550("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)->HasKeyEqual_155550("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)
             ->HasKeyEqual_155550("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)->HasKeyEqual_155550("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_PICKUPS");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_animRegistry)
             ->HasKeyEqual_155550("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
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
            ((CWorldSoundSet*)m_4w()->m_54)->Resume();
        }
        m_4w()->m_68->Reset();
        ((CGruntSpawnConfig*)m_4w()->m_60)->DtorBody();
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
// manager (m_4->m_48) is the real CGruntzSoundZ (<Dsndmgr/GruntzSoundZ.h>,
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
    CBattlezData* m_7c; // +0x7c
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
struct CRpReg44 {
    char p0[0x124];
    i32 m_124; // +0x124
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
    CSaveGame* m_58; // +0x58
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
            self->m_4->m_7c->FillRecord(self->m_1c, 1);
            reg = (CRpReg*)g_64556c;
            if (reg->m_44->m_124 == 0) {
                i32 id = self->m_1c;
                if (id > 0x24 || id == 1) {
                    reg->m_58->SetMaxLevel(id);
                    reg = (CRpReg*)g_64556c;
                }
            }
            reg->m_58->Save(0, 0x81a6);
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
        ((CBattlezMapConfig*)((char*)g_64556c + 0x188 + off))->Method_025c20();
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

// The CRtArr timeline arrays are MFC CObArrays (SetSize @0x1b4f75 = CObArray::SetSize,
// reached via a CObArray cast at each call; CObArray is already visible transitively here).
struct CRtArr { // an MFC CObArray (m_data @+4, m_count @+8); 0x14 stride
    char p0[0x4];
    void** m_data; // +0x4
    i32 m_count;   // +0x8
    char pc[0x14 - 0xc];
};
// the m_68+0x260 array variant: an MFC CPtrArray (SetSize @0x1b52e8 = CPtrArray::SetSize,
// reached via a CPtrArray cast at the call).
struct CRtArr2 {};
struct CRtTimeline { // m_4->m_68 (also g_64556c->m_68)
    char p0[0x260];
    CRtArr2 m_260; // +0x260
    char p264[0x284 - 0x264];
    i32 m_284; // +0x284
    char p288[0x2a0 - 0x288];
    i32 m_2a0; // +0x2a0
};
struct CRtWorld { // this->m_4
    char p0[0x48];
    CGruntzSoundZ* m_48; // +0x48
    char p4c[0x54 - 0x4c];
    CWorldSoundSet* m_54; // +0x54
    char p58[0x60 - 0x58];
    CGruntSpawnConfig* m_60; // +0x60
    char p64[0x68 - 0x64];
    CRtTimeline* m_68; // +0x68
};
// FOREIGN m_c->m_24 image registry: only vtable slot 17 (Teardown, +0x44) is
// dispatched here; the rest are unreconstructed engine code. Honest model = a manual
// vptr into a typed vtable struct naming ONLY the used slot as a 4-byte thiscall PMF
// + char pad[], NO fake virtuals.
// Real polymorphic view: Teardown is slot 17 (+0x44), a real virtual (17 fillers).
struct CRtImageReg { // m_c->m_24
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Teardown(); // slot 17 (+0x44)
    void CallTeardown() {
        Teardown();
    }
};
struct CRtSoundReg { // m_c->m_28
    char p0[0x2c];
    SoundStream* m_2c; // +0x2c
};
struct CRtResMgr { // this->m_c
    char p0[0x8];
    CWwdObjMgr* m_8;       // +0x08
    CDDrawWorkerList* m_c; // +0x0c  (ClearWorkers @0x163c60)
    char p10[0x24 - 0x10];
    CRtImageReg* m_24; // +0x24
    CRtSoundReg* m_28; // +0x28
};
// CRtThis's begin-marker is a CTileTriggerContainer (RemoveAll @0x116fa0); its frame-marker is a
// CTimer (Reset @0x9bc70, <Gruntz/Timer.h>) - same shapes CDtorThis already uses. Declared here so
// CRtThis (below) can name them.
struct CTileTriggerContainer {
    void RemoveAll(); // 0x116fa0
    ~CTileTriggerContainer();
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
    CSBI_RectOnly* m_guts; // +0x2dc  guts subsystem
    char p2e0[0x2e4 - 0x2e0];
    CTileTriggerContainer* m_beginMarker; // +0x2e4  begin marker
    char p2e8[0x370 - 0x2e8];
    CRtArr m_startMarkers; // +0x370  (m_markerData data / m_markerCount count)
    char p384[0x3a4 - 0x384];
    CRtArr m_3a4[4];       // +0x3a4  stride 0x14
    CTimer* m_frameMarker; // +0x3f4  frame marker
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
        ((CTriggerMgr*)self->m_4->m_68)->ClearGridRange(5);
    }
    Teardown1780();
    if (self->m_c->m_28->m_2c != 0) {
        self->m_c->m_28->m_2c->Stop();
    }
    self->m_4->m_48->StopAndFlush();
    self->m_4->m_54->Teardown();
    self->m_4->m_60->ClearSprites();
    ((CTriggerMgr*)((CRtReg*)g_64556c)->m_68)->DestroyAllAnims();
    self->m_c->m_24->CallTeardown();
    self->m_c->m_8->PruneList_15aa90();
    if (self->m_guts != 0) {
        self->m_guts->ResetWidgets(0);
    }
    if (self->m_beginMarker != 0) {
        self->m_beginMarker->RemoveAll();
    }
    if (self->m_frameMarker != 0) {
        self->m_frameMarker->Reset();
    }
    self->m_4e4 = 0;
    ((CTriggerMgr*)self->m_4->m_68)->OverlayTick();
    CRtTimeline* tl68 = self->m_4->m_68;
    ((CPtrArray*)&tl68->m_260)->SetSize(0, -1);
    tl68->m_284 = 0;
    ((CTriggerMgr*)self->m_4->m_68)->Reset1b48a6();
    self->m_4->m_68->m_2a0 = 0;
    self->m_c->m_c->ClearWorkers();
    for (i = 0; i < self->m_startMarkers.m_count; i++) {
        void* node = self->m_startMarkers.m_data[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_freeListNodeBias);
            *p = g_freeList;
            g_freeList = p;
        }
    }
    ((CObArray*)&self->m_startMarkers)->SetSize(0, -1);
    for (k = 0; k < 4; k++) {
        for (i = 0; i < self->m_3a4[k].m_count; i++) {
            void* node = self->m_3a4[k].m_data[i];
            if (node != 0) {
                void** p = (void**)((char*)node - g_freeListNodeBias);
                *p = g_freeList;
                g_freeList = p;
            }
        }
        ((CObArray*)&self->m_3a4[k])->SetSize(0, -1);
    }
    for (i = 0; i < self->m_488.m_count; i++) {
        void* node = self->m_488.m_data[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_freeListNodeBias);
            *p = g_freeList;
            g_freeList = p;
        }
    }
    ((CObArray*)&self->m_488)->SetSize(0, -1);
    for (i32 off = 0; off < 0x8e0; off += 0x238) {
        ((CBattlezMapConfig*)((char*)self->m_4 + 0x188 + off))->FreeArrays();
        ((CBattlezMapConfig*)((char*)self->m_4 + 0x188 + off))->Clear_02ade0();
    }
    self->m_49c = -1;
}

// ---------------------------------------------------------------------------
// CPlayDtorBody (0x0c8700): the ~CPlay teardown body. Frees the per-frame
// workers, clears the four g_mgrSettings config rows, flushes the free-list
// arrays, then chains the base (CState) dtor. Same free-list idiom as
// FreeListTeardown (the m_markerData/m_3a4[4]/m_48c flush).
// ---------------------------------------------------------------------------
// The +0x2e4 begin-marker: Dtor@0xc8640 IS CTileTriggerContainer::~ (IS-a dtor view);
// TU-local minimal decl for the explicit dtor call (real class in tiletriggercontainer unit).
struct DtorObList {
    // Dtor @0x1b9c69 IS CObList::~CObList; cast at the call.
};
struct DtorWorld { // this->m_4
    char p0[0x5c];
    CFontConfig* m_5c; // +0x5c
    char p60[0x128 - 0x60];
    i32 m_128; // +0x128
};

// FOREIGN view of CPlay's `this` for the dtor path: only vtable slot 32 (Vfunc80,
// +0x80) is dispatched; the rest are unreconstructed engine code. Honest model = a
// manual vptr into a typed vtable struct naming ONLY the used slot as a 4-byte
// thiscall PMF + char pad[], NO fake virtuals. m_vtbl sits at +0x00 exactly where the
// fake virtuals' vptr did, so the object layout (p0/m_4/... below) is byte-identical.
// FOREIGN view of CPlay's `this` for the dtor path: Vfunc80 is slot 32 (+0x80),
// a real virtual (32 fillers); the compiler emits the vptr at +0x00 - byte-identical
// layout. CallVfunc80() -> Vfunc80() lowers to the same call [eax+0x80].
struct CDtorThis {
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Slot18();
    virtual void Slot19();
    virtual void Slot20();
    virtual void Slot21();
    virtual void Slot22();
    virtual void Slot23();
    virtual void Slot24();
    virtual void Slot25();
    virtual void Slot26();
    virtual void Slot27();
    virtual void Slot28();
    virtual void Slot29();
    virtual void Slot30();
    virtual void Slot31();
    virtual void Vfunc80(); // slot 32 (+0x80)
    void BaseDtor();        // 0x3f53 thunk  (base CState dtor)

    char p0[0x4];
    DtorWorld* m_4; // +0x04
    char p8[0x1d0 - 0x8];
    i32 m_1d0; // +0x1d0
    char p1d4[0x2dc - 0x1d4];
    CSBI_RectOnly* m_guts;                // +0x2dc
    CChatBoxOwner* m_hitTest;             // +0x2e0
    CTileTriggerContainer* m_beginMarker; // +0x2e4
    char p2e8[0x320 - 0x2e8];
    CLightFxRender* m_320; // +0x320
    char p324[0x370 - 0x324];
    CRtArr m_startMarkers; // +0x370
    char p384[0x3a4 - 0x384];
    CRtArr m_3a4[4];       // +0x3a4
    CTimer* m_frameMarker; // +0x3f4
    char p3f8[0x488 - 0x3f8];
    CRtArr m_488; // +0x488
    i32 m_49c;    // +0x49c
    void CallVfunc80() {
        Vfunc80();
    }
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
        self->m_320->Ctor();
        ::operator delete(self->m_320);
        self->m_320 = 0;
    }
    self->CallVfunc80();
    if (self->m_4) {
        self->m_4->m_128 = 0;
        ((CObList*)((char*)self->m_4 + 0xc8))->~CObList();
    }
    self->m_1d0 = 0;
    i32 off = 0;
    do {
        off += 0x238;
        *(i32*)((char*)g_64556c + off - 0xc8) = 0;
    } while (off < 0x8e0);
    if (self->m_4 && self->m_4->m_5c) {
        ((CFontConfig*)self->m_4->m_5c)->FreeNodes();
    }
    if (self->m_guts) {
        self->m_guts->DtorMembers();
        ::operator delete(self->m_guts);
        self->m_guts = 0;
    }
    if (self->m_hitTest) {
        self->m_hitTest->Deactivate();
        ::operator delete(self->m_hitTest);
        self->m_hitTest = 0;
    }
    if (self->m_beginMarker) {
        self->m_beginMarker->~CTileTriggerContainer();
        ::operator delete(self->m_beginMarker);
        self->m_beginMarker = 0;
    }
    if (self->m_frameMarker) {
        self->m_frameMarker->Reset();
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
    ((CObArray*)&self->m_startMarkers)->SetSize(0, -1);
    for (i32 k = 0; k < 4; k++) {
        for (i = 0; i < self->m_3a4[k].m_count; i++) {
            void* node = self->m_3a4[k].m_data[i];
            if (node != 0) {
                void** p = (void**)((char*)node - g_freeListNodeBias);
                *p = g_freeList;
                g_freeList = p;
            }
        }
        ((CObArray*)&self->m_3a4[k])->SetSize(0, -1);
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
    ((CObArray*)&self->m_488)->SetSize(0, -1);
    ((CGameModeBase*)self)->BaseCleanup();
}

// ---------------------------------------------------------------------------
// EnterMode (0x0d6fa0): the mode-enter gate. Pause the guts + world, optionally
// run the deferred draw, (re)install the renderer view, then re-arm the guts and
// (mode 9) latch the saved game clock. A dense chain of CPlay/registry thunks.
// ---------------------------------------------------------------------------
struct EmGuts { // this->m_guts
};
struct EmHdr14 {
    char p0[0x2c];
    CDDSurface* m_2c; // +0x2c
};
struct EmCWorld { // m_c->m_4 (real: CDDrawWorkerMgr)
    char p0[0x14];
    EmHdr14* m_14; // +0x14
    i32 m_18;      // +0x18
};
struct EmRendC { // m_c->m_c; real polymorphic, Present is slot 13 (+0x34)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void __stdcall Present(EmHdr14*, i32); // slot 13 (+0x34)
};
struct EmResMgr { // this->m_c
    char p0[0x4];
    EmCWorld* m_4; // +0x04
    void* m_8;     // +0x08
    EmRendC* m_c;  // +0x0c
    char p10[0x24 - 0x10];
    CGameLevel* m_24; // +0x24
};
struct EmWorld { // this->m_4
    char p0[0x10];
    i32 m_10; // +0x10
    char p14[0x54 - 0x14];
    void* m_54; // +0x54
};

struct EmThis {
    // DeferredDraw IS CPlay::NotifyVisibleEntities; cast at the call.
    // ArmTimer @0xfa8f0 IS CSoundFxEmitter::Method_fa8f0; cast at the call.
    // FinishMode IS CPlay::RegisterInputBindings; cast at the call.

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
void EmRegWorldStep(CGruntzMgr* reg, EmGuts* guts, i32 a); // 0x2356  cdecl(reg, guts, a)

// @early-stop
// large state-machine wall: the mode dispatch + renderer reinstall + guts re-arm
// are faithful, but the whole ILT-thunk referent set (~15 unnamed CPlay/registry
// leaves) keeps it reloc-fuzzy; codegen plateau, not source-steerable.
RVA(0x000d6fa0, 0x1fa)
i32 CPlay::EnterMode(i32 mode) {
    EmThis* self = (EmThis*)this;
    ((CGruntzMgr*)g_64556c)->CheckSavedMode();
    ((CSBI_RectOnly*)self->m_guts)->Deactivate();
    ((CSBI_RectOnly*)self->m_guts)->LoadDestructButtonSprite(0);
    ((CGruntzMgr*)self->m_4)->PerFrameTick();

    if (self->m_1c4 != 0) {
        self->m_1c4 = 0;
        self->m_c->m_4->m_14->m_2c->Fill(0);
        EmRegWorldStep((CGruntzMgr*)g_64556c, self->m_guts, self->m_470);
        if (self->m_474 != 0) {
            ((CPlay*)self)->NotifyVisibleEntities();
        } else {
            self->m_c->m_24->VisitVisible(self->m_c->m_4->m_14, (CGameObjChain*)self->m_c->m_8);
            self->m_c->m_c->Present(self->m_c->m_4->m_14, self->m_c->m_4->m_18);
        }
        ((CSBI_RectOnly*)self->m_guts)->Deactivate();
        ((CSBI_RectOnly*)self->m_guts)->LoadMainStatusBarSprite();
    } else {
        if (self->m_474 != 0) {
            ((CPlay*)self)->NotifyVisibleEntities();
        } else {
            self->m_c->m_24->VisitVisible(self->m_c->m_4->m_14, (CGameObjChain*)self->m_c->m_8);
            self->m_c->m_c->Present(self->m_c->m_4->m_14, self->m_c->m_4->m_18);
        }
        ((CSBI_RectOnly*)self->m_guts)->Deactivate();
        ((CSBI_RectOnly*)self->m_guts)->LoadMainStatusBarSprite();
        if (mode == 9) {
            if (((CDDrawSubMgrPages*)self->m_c->m_4)->Method_158d20() != 0) {
                goto finish;
            }
            if (((CDDrawSubMgrPages*)self->m_c->m_4)->Method_158cb0(0, 0x30000) != 0) {
                goto finish;
            }
            return 0;
        }
        self->m_c->m_4->m_14->m_2c->Fill(0);
    }

finish:
    ((CDDrawSubMgrPages*)self->m_c->m_4)->Method_158e90();
    ((CSoundFxEmitter*)self)->Method_fa8f0(0x50, 0x3e8, 0, 1);
    if (self->m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)self->m_c->m_24->m_mainPlane)->CenterScrollB();
    }
    ((CGruntzMgr*)self->m_4)->PerFrameTick();
    self->m_inputWarmup1 = 0;
    self->m_inputWarmup2 = 0;
    self->m_inputHalfSel = 0;
    if (self->m_4->m_10 != 0 && mode != 9) {
        ((CWorldSoundSet*)self->m_4->m_54)->Resume();
    }
    if (mode == 9) {
        g_645588_clk = self->m_savedClock;
    }
    ((CSBI_RectOnly*)self->m_guts)->Deactivate();
    ((CPlay*)self)->RegisterInputBindings();
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
struct AgWorld { // this->m_4
    char p0[0x68];
    CTriggerMgr* m_68; // +0x68
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
            msg.Format("Could not add Grunt: Player=%d", g->m_124, y, x);
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

// The four placed-object dynamic-type markers LoadWarlordSprites tests obj->m_7c[4]
// against: each is the address of that object class's vtable slot-4 (+0x10) method
// thunk. Named so the `cmp eax,<thunk>` emits its DIR32 reloc (retail relocates the
// immediate) instead of a bare number - the reloc is what the match needs.
DATA(0x000024a5)
extern char g_objIdThunk_24a5[]; // "multi-sprite warlord" object (m_11c/m_120 + m_118 switch)
DATA(0x0000288d)
extern char g_objIdThunk_288d[]; // counted object keyed on m_124
DATA(0x00003d0f)
extern char g_objIdThunk_3d0f[]; // counted object keyed on m_11c
DATA(0x0000137a)
extern char g_objIdThunk_137a[]; // counted object keyed on m_11c (sibling of 3d0f)

// A placed object walked in the in-level branch: its +0x7c dynamic-type vtable
// (whose +0x10 slot carries the type marker) + its sprite/type ids.
SIZE_UNKNOWN(CWarlordObj);
struct CWarlordObj {
    char p0[0x7c];
    void** m_7c; // +0x7c  (raw vtable of the identity sub-object)
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
    CWarlordListHead* head = &((CRenderer*)this->m_c->m_8)->m_10;
    if (!head) {
        return 0;
    }
    CWarlordListNode* node = head->m_4;
    while (node) {
        CWarlordObj* obj = node->m_8;
        CWarlordListNode* nxt = node->m_0;
        if (obj) {
            void* marker = obj->m_7c[4];
            if (marker == (void*)g_objIdThunk_24a5) {
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
            } else if (marker == (void*)g_objIdThunk_288d) {
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
            } else if (marker == (void*)g_objIdThunk_3d0f || marker == (void*)g_objIdThunk_137a) {
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
// m_c->m_28 + 0x10 is an MFC CMapStringToOb (Lookup @0x1b8438); cast at each call.
struct CEffMap {};
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
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GAME_PYRAMIDMOVE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GAME_TELEPORTEROPEN", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GAME_TELEPORTERCLOSE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GAME_TELEPORTERALL", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 4000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GAME_BRICKBREAK", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_DEATHBRIDGEMOVE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_WATERBRIDGEMOVE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_ROCKBREAK", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_LAVAGEYSER", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_TRAPDOORCLOSE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_TRAPDOOROPEN", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_CANDLEIGNITE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_CANDLEUP", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_CANDLEDOWN", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_GOLFBALLAIR2", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_GOLFBALLHOLE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_GOLFBALLSINK", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GAME_EXPLOSION1", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_OUTLETHAZARD", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE1A", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE2A", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)
        ->Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)
        ->Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GRUNTZ_DEATHZ_RESSURECT", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("GRUNTZ_DEATHZ_DEATHZSQUASH1A", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_CLOUDHAZARDMOVE", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 10000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_CLOUDHAZARDKILL", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 3000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)
        ->Lookup("GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)
        ->Lookup("GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)
        ->Lookup("GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)
        ->Lookup("GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1", (CObject*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    ((CMapStringToOb*)&self->m_c->m_28->m_10)->Lookup("LEVEL_PLANEHAZARDFLY", (CObject*&)d);
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
SIZE_UNKNOWN(CAnimRegistry);
SIZE_UNKNOWN(CCueState);
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
SIZE_UNKNOWN(StateMgrBZ);
SIZE_UNKNOWN(CRenderer);
SIZE_UNKNOWN(CRpFrame);
SIZE_UNKNOWN(CRpGeom);
SIZE_UNKNOWN(CRpM30);
SIZE_UNKNOWN(CRpReg);
SIZE_UNKNOWN(CRpReg44);
SIZE_UNKNOWN(CRpReg58);
SIZE_UNKNOWN(CRpScroll);
SIZE_UNKNOWN(CRpSlot);
SIZE_UNKNOWN(CRpThis);
SIZE_UNKNOWN(CRpTimeline);
SIZE_UNKNOWN(CRpWorld);
SIZE_UNKNOWN(CRtArr);
SIZE_UNKNOWN(CRtArr2);
SIZE_UNKNOWN(CRtGuts);
SIZE_UNKNOWN(CRtImageReg);
SIZE_UNKNOWN(CRtReg);
SIZE_UNKNOWN(CRtRendererA);
SIZE_UNKNOWN(CDDrawWorkerList);
SIZE_UNKNOWN(CRtResMgr);
SIZE_UNKNOWN(CRtRow);
SIZE_UNKNOWN(CRtSound);
SIZE_UNKNOWN(CRtSoundReg);
SIZE_UNKNOWN(CRtSub60);
SIZE_UNKNOWN(CRtThis);
SIZE_UNKNOWN(CRtTimeline);
SIZE_UNKNOWN(CRtWorld);
SIZE_UNKNOWN(CRtWorldDraw);
SIZE_UNKNOWN(CSoundRegistry);
SIZE_UNKNOWN(CState);
SIZE_UNKNOWN(CVisEntity);
SIZE_UNKNOWN(CVisEntityType);
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
SIZE_UNKNOWN(DtorWorkerE);
SIZE_UNKNOWN(DtorWorld);
SIZE_UNKNOWN(Edge);
SIZE_UNKNOWN(EmCWorld);
SIZE_UNKNOWN(EmGuts);
SIZE_UNKNOWN(EmHdr14);
SIZE_UNKNOWN(EmHdr2c);
SIZE_UNKNOWN(EmReg24Sub);
SIZE_UNKNOWN(EmRendC);
SIZE_UNKNOWN(EmResMgr);
SIZE_UNKNOWN(EmSink5c);
SIZE_UNKNOWN(EmSub54);
SIZE_UNKNOWN(EmThis);
SIZE_UNKNOWN(EmWorld);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// @early-stop
// 0x0cf0a0 (1.4 KB) - homed from src/Stub/GapFunctions.cpp (matcher-5); a large Play
// worker in this TU's .text block, no vtable-ref. Homed pending leaf-first reconstruction.
RVA(0x000cf0a0, 0x567)
i32 Gap_0cf0a0(void) {
    return 0;
}

// @early-stop
// 0x0cfc90 (465 B) - homed from src/Stub/GapFunctions.cpp (matcher-5); a Play leaf,
// no vtable-ref. Homed pending leaf-first reconstruction.
RVA(0x000cfc90, 0x1d1)
i32 Gap_0cfc90(void) {
    return 0;
}
