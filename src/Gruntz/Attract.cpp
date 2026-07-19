// Attract.cpp - the attract state-services interval TU at retail .text
// [0x0fa1f0 .. 0x0fb328]: one WOVEN original obj (weave
// 0.36) that interleaves function-by-function the CAttract title/fade services
// (FadeInTitle/RunTitle/RunTitleSeq), the CSoundFxEmitter screen-transition
// emitters, the CState draw/shade/paint helpers (Vslot17/Vslot07/ShadeScreen) and
// the CMgrPersistObj serialize family. SaveRecord (0xfaff0)'s Load is the read-side
// twin of Save @0xfb1c0.
//
// REHOME/holding-TU drain (D5): the CAttract state-machine CORE obj (the 0x13fb0
// band of CState vtable-slot overrides + the ??1 dtor @0x08cd90) was a SEPARATE
// original obj and moved to AttractState.cpp (proven by the CAttract vtable). The
// 0x13fb0 band never truly "lived here" - it was conflated. Owner attribution:
// `sema class CAttract` (slot RVAs) for the core; `sema xref --tree` .rdata refs
// for the foreign fragments below.
//
// TEMPORARILY retained here (Phase-2 fold to their PROVEN owners is pending; each
// is a foreign obj's method mis-attributed to CAttract - matches only because the
// sibling CState leaves share the layout):
//   RefreshTitle    0x039160  -> CCreditsState slot 10 (??_7CCreditsState@@6B@+0x28)
//   LoadTitleConfig 0x0a03f0  -> CMenuState    slot 9  (??_7CMenuState@@6B@+0x24)
//   Activate        0x0a0a30  -> CMenuState    slot 6  (??_7CMenuState@@6B@+0x18)
//
// NB: CPreviewState::LoadScreen (0xfab90) also belongs to this interval but
// stays in LevelPreview.cpp for now - moving it needs CPreviewState homed into
// a shared header first (it is that TU's local class); deferred.
// Field names are placeholders; only OFFSETS + code bytes matter.
#include <Gruntz/String.h> // MFC CString (the title-roll formats into one); MFC-first
#include <Gruntz/GameRegPtr.h>
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/GruntzMgr.h>
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Gruntz/Attract.h>
#include <Gruntz/GameRegistry.h> // the ONE game-registry shape (CGameRegistry / g_gameReg)
#include <Gruntz/AttractActor.h> // the shared per-frame g_actorList view (also used by CDemo/CHelpState)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSubMgrPages (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <DDrawMgr/DDrawSubMgrPages.h> // the ONE CDDrawSubMgrPages shape (Method_158b40)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair/m_frontPair->m_surface)
#include <DDrawMgr/DDSurface.h>        // the frame surface CDDSurface (m_10->m_2c: Flip + m_8)
#include <ddraw.h>                     // IDirectDrawSurface (the flip surface's raw +0x8 COM iface)
#include <Gruntz/SoundFxEmitter.h>     // CSoundFxEmitter (the screen-transition emitters)
#include <Gruntz/Fader.h>              // CFaderMgr / CFader / CFxModeT2/T3
#include <Bute/SymParser.h>            // CSymParser (CMgrPersistObj::m_rezLocator ResolvePath)
#include <Gruntz/SerialArchive.h>      // the ONE shared archive stream (Read@+0x2c / Write@+0x30)
#include <Gruntz/MgrPersist.h>         // CMgrPersistObj / SplashParams (the persisted settings obj)
#include <Wap32/EngStr.h>              // THE canonical EngStr_DrawText (0x115440) lean decl
#include <rva.h>
#include <Globals.h>

#include <stdio.h>  // sprintf (the "\SCREENZ\%s" path formatter)
#include <string.h> // inline /Oi strlen (repnz scasb) for the Vslot17 TextOutA
// DirectSoundMgr (IsPlaying/CloneAndPlay) + SoundDevice (PurgeVoiceList) now come from the
// real <Dsndmgr/SoundDevice.h>, pulled through <Gruntz/SoundCue.h> (via GameRegistry.h).

// The attract-cue registrar IS a CDDrawSubMgrLeafScan (header-less); local decl (exact arg types).
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree/RemoveKeysEqual)

// ---------------------------------------------------------------------------
// External engine globals (reloc-masked DATA symbols).
// ---------------------------------------------------------------------------

// The CButeMgr text-config singleton g_buteMgr (0x6453d8) comes from <Bute/ButeMgr.h>.

// The game registry singleton (canonical CGameRegistry, <Gruntz/GameRegistry.h>): its
// +0x80 launch counter (m_numRuns, "Num_Runs") selects the TITLE state. The retail
// reads it off the canonical g_gameReg pointer at ds:0x64556c (verified in InputVirtual/
// Activate/LoadTitleConfig: mov ecx,ds:0x64556c; mov eax,[ecx+0x80]). Canonical DATA at
// 0x24556c (the CMgrPersistObj::Save m_world gate is the same object).

// The attract-state count divisor (DAT_00645534) is declared in <Gruntz/Attract.h>
// (included above); DATA home + producer is src/Rez/RezSync.cpp.

// The "Disable Fades" [Config] gate the CSoundFxEmitter methods poll (0x2455c4).
// DEFINED in src/Rez/RezSync.cpp (owner); declared in <Globals.h>.

// The present-suppress latch (DAT_0064e360), private to the attract loop; DEFINED
// here (owner TU), a plain `extern` stays in Globals.h.
DATA(0x0024e360)
i32 g_suppress_64e360 = 0; // 0x24e360

// ShowCursor is the real USER32 import (<Mfc.h>); its IAT slot @0x6c44c4.

// Source string literals (objdiff matches these .data relocations by value).

// FadeInTitle's resolved-state object (m_2c re-typed) is the CAttractScreenObj
// facet (full model in <Gruntz/Attract.h>); its ResolveScreen (FUN_00520120) maps
// the "\SCREENZ\%s" path + a screen-type tag to a fade page.
// The screen-type tag (DAT_00504358) ResolveScreen keys off.

// The menu page worker (m_c->m_04 re-typed): its fader (0x158b40, ret 8) runs the
// title fade, returning non-zero when the fade is still in progress. Named to retail
// (?Method_158b40@CDDrawSubMgrPages) so the call pairs exactly. See CDDrawSubMgrPages.h.

// ---------------------------------------------------------------------------
// FramePoll (0x143e0) sub-object models.
// ---------------------------------------------------------------------------

// The render-busy check dispatches IDirectDrawSurface::IsLost (slot 24, +0x60,
// __stdcall self-on-stack) on the flip surface's held DirectDraw COM interface
// (CDDSurface::m_8) to see whether the page still needs a restore.

// AttractActor / AttractActorList (the per-frame g_actorList view) live in the
// shared <Gruntz/AttractActor.h> (also used by CDemo/CHelpState).

// (The band-A-only externs - g_frameDelta, g_pPostMessageA, s_dat60b5bc,
// g_emptyString, g_sndEnabled, g_pTimeGetTime, g_pwsprintfA, the AttractWndHolder
// view + s_ATTRACT_TITLE_s - moved with the CAttract core to AttractState.cpp.)

// ===========================================================================
// The CAttract title/menu foreign fragments (temporarily retained here pending
// the Phase-2 fold to their proven owners): RefreshTitle @0x39160 is CCreditsState
// vtable slot 10 (??_7CCreditsState@@6B@+0x28), LoadTitleConfig @0xa03f0 is
// CMenuState slot 9 (+0x24) and Activate @0xa0a30 is CMenuState slot 6 (+0x18).
// The CAttract state-machine CORE band [0x13fb0..0x14819] + the ??1 dtor moved to
// AttractState.cpp (its true separate obj). See the file header + report.
// ===========================================================================

// CAttract::RefreshTitle - re-prime the attract title scene. Resets the scene
// slot off m_4->m_48 (PrimeScene then RestoreScene), re-resolves the
// "STATEZ_ATTRACT" state into m_2c, runs the title sequence with the bare "TITLE"
// tag, and returns 1.
RVA(0x00039160, 0x46)
i32 CAttract::RefreshTitle(i32 unused) {
    ((CGruntzSoundZ*)video()->m_48)->IsPlaying();
    ((CGruntzSoundZ*)video()->m_48)->StopAndFlush();
    m_2c = (CResSource*)stateMgr()->ResolvePath("STATEZ_ATTRACT");
    RunTitleSeq("TITLE", 0, 0, 1, 0);
    return 1;
}

// CAttract::LoadTitleConfig - configure the attract/title sequence.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; retail emits a separate inline `xor eax,eax` for the FadeInTitle
// fail return-0, the recompile reuses the already-zero eax. Not steerable by source.
RVA(0x000a03f0, 0x14b)
i32 CAttract::LoadTitleConfig(i32 mode) {
    char stateName[0x20];
    char titleName[0x20];

    if (mode != 2) {
        i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
        sprintf(stateName, "STATEZ_ATTRACT");
        sprintf(titleName, "TITLE%d", idx);

        CSymTab* saved = attractState();
        CSymTab* state = (CSymTab*)stateMgr()->ResolvePath(stateName);
        m_2c = (CResSource*)state;
        if (state == 0) {
            return 0;
        }

        i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
        m_2c = (CResSource*)saved;
        if (faded == 0) {
            return 0;
        }

        CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_14->m_2c;
        ((CDDSurface*)tgt)
            ->ShadeRect(g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32), (tagRECT*)0);
        menuRoot()->m_04->TransTitle();
    } else {
        menuRoot()->m_04->TransEnter();
        CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_18->m_2c;
        ((CDDSurface*)tgt)
            ->ShadeRect(g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32), (tagRECT*)0);
        menuRoot()->m_04->TransExit();
    }

    RetireScene(0x50, 0x3e8, 0,
                1); // 0xfa8f0 CState::RetireScene
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(1) < 0) {
        do {
        } while (ShowCursor(1) < 0);
    }
    CommitStage();
    return 1;
}

// CAttract::Activate - virtual attract-screen (re)entry. Gates on slot-3
// (Vfunc3); if it fails, returns that result. Otherwise resets the title
// brightness target, picks a random TITLE state off the registry, resolves it,
// runs the title fade, sets menu brightness, transitions the page, rebuilds the
// menu page, forces the cursor visible, and returns 1.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; on the FadeInTitle fail path retail emits a fresh inline
// `xor eax,eax` (return 0) while the recompile reuses the already-zero FadeInTitle
// result in eax. Same non-steerable wall as the sibling LoadTitleConfig.
RVA(0x000a0a30, 0x110)
i32 CAttract::Activate() {
    char stateName[0x20];
    char titleName[0x20];

    i32 gate = Vfunc3();
    if (gate == 0) {
        return gate;
    }

    ((CMenuBrightnessReset*)menuRoot()->m_04->m_14->m_2c)->Reset(0);

    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);

    CSymTab* saved = attractState();
    CSymTab* state = (CSymTab*)stateMgr()->ResolvePath(stateName);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }

    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (CResSource*)saved;
    if (faded == 0) {
        return 0;
    }

    CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_14->m_2c;
    ((CDDSurface*)tgt)
        ->ShadeRect(g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32), (tagRECT*)0);
    menuRoot()->m_04->TransTitle();

    RetireScene(0x50, 0x3e8, 0,
                1); // 0xfa8f0 CState::RetireScene
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(1) < 0) {
        do {
        } while (ShowCursor(1) < 0);
    }
    return 1;
}

// ===========================================================================
// The state-services interval [0x0fa1f0 .. 0x0fb328].
// ===========================================================================

// CState::FadeInTitle(name, a, b, c, d, e) (0x0fa1f0, 6 args, ret 0x18): resolve the
// "\SCREENZ\<name>" fade page off m_2c (with the screen-type tag), then run the page
// worker's fade (mode 2 when `e`, else 1); on `e` retry once with mode 1. ret 1 on a
// started fade, else 0.
// @early-stop
// frame-reservation + reloc wall (~74%): logic + offsets byte-exact; retail reserves an
// 0x40 frame (0xc outgoing-arg scratch below the 0x34 buf) where our cl reserves only the
// 0x34 buf, and the ResolveScreen callee (FUN_00520120) is an unnamed body that can't pair
// (reloc-masked DIR32). Not source-steerable. topic:wall.
RVA(0x000fa1f0, 0xc6)
i32 CState::FadeInTitle(const char* name, i32 a, i32 b, i32 c, i32 d, i32 e) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    if (!m_c) {
        return 0;
    }
    if (!m_8) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    char buf[0x34];
    sprintf(buf, "\\SCREENZ\\%s", name);
    void* page = screenObj()->ResolveScreen(buf, &g_screenTag);
    if (page == 0) {
        return 0;
    }
    CDDrawSubMgrPages* w = (CDDrawSubMgrPages*)menuRoot()->m_04;
    if (w->Method_158b40((i32)page, e != 0 ? 2 : 1) != 0) {
        return 1;
    }
    if (e == 0) {
        return 1;
    }
    if (w->Method_158b40((i32)page, 1) != 0) {
        return 1;
    }
    return 0;
}

// CState::RunTitle(...) (0x0fa300, 5 args, ret 0x14): the title-render entry.
// Bail (0) if the menu root (m_c), state machine (m_8), or active state (m_2c) is
// null; otherwise flip the menu page's render target and return 1.
// @early-stop
// regalloc chain-staging coin-flip (docs/patterns/zero-register-pinning.md): body
// byte-identical except ONE modrm in the m_04->m_10->m_2c->Flip chain - retail
// stages the penultimate deref through eax (8b 40 10) then ecx, the recompile
// switches to ecx one deref early (8b 48 10). The SAME inline chain matches in
// Vslot07 (different surrounding pressure) - a pure allocator choice, no source lever.
RVA(0x000fa300, 0x3a)
i32 CState::RunTitle(i32 a, i32 b, i32 c, i32 d, i32 e) {
    if (!m_c) {
        return 0;
    }
    if (!m_8) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    menuRoot()->m_04->m_10->m_2c->Flip(0);
    return 1;
}

// CState::RunTitleSeq(name, a, b, c, d) (0x0fa350, 5 args, ret 0x14): the title-roll
// entry. Bail (0) if the menu root/state-machine/active-state is null; FadeInTitle the
// screen (mode 0); on success return RunTitle() != 0.
RVA(0x000fa350, 0x84)
i32 CState::RunTitleSeq(const char* name, i32 a, i32 b, i32 c, i32 d) {
    if (!m_c) {
        return 0;
    }
    if (!m_8) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    if (FadeInTitle(name, a, b, c, d, 0) == 0) {
        return 0;
    }
    return RunTitle((i32)name, a, b, c, d) != 0;
}

// ---------------------------------------------------------------------------
// CSoundFxEmitter - five sibling sound-effect/screen-transition emitters
// (0xfa410, 0xfa550, 0xfa790, 0xfa8f0, 0xfaa60). See CSoundFxEmitter.h for the
// recovered class/chain layout. Each: gate on the resource chain, fill a
// CFxModeT2/T3 transition descriptor on the stack, register it with the CFaderMgr,
// then - per g_disableFades - apply the channel op now or defer it through the new
// fader, and finally Remove the fader. All callees are reloc-masked externs.
// ---------------------------------------------------------------------------

// 0xfa410: single-channel type-2 emitter (4 args).
RVA(0x000fa410, 0xf5)
i32 CSoundFxEmitter::Method_fa410(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_worker->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_10 = 1;
    t.m_18 = a1;
    t.m_1c = a2;
    t.m_04 = (i32)chan;
    t.m_08 = 0;
    CFader* f = mgr->Add(1, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(a3);
        m_resChain->m_worker->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(a3, a4, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// 0xfa550: two-channel type-2 emitter (4 args). Blt channel A onto channel B.
// @early-stop
// 98.7% - logic byte-faithful. Residual is store/arg scheduling: cl hoists the
// m_18=a1 temp store before the Add-arg push where retail hoists m_1c=a2, and the
// deferred winapi_17e620 branch picks ecx/edx vs retail's eax/ecx for the two arg
// temporaries. Both are /O2 instruction-scheduling choices, not source-steerable.
RVA(0x000fa550, 0x10c)
i32 CSoundFxEmitter::Method_fa550(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_worker->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_1c = a2;
    t.m_10 = 0;
    t.m_18 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(1, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(a3);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a3, a4, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// ---------------------------------------------------------------------------
// CState::Vslot17 (0x0fa6b0, vtable slot 23 / +0x5c, inherited by every state):
// the frame-surface GDI text overlay - draw `str` at (x,y) onto the frame
// surface via GDI (GetDC on the IDirectDrawSurface, SetBkMode/SetTextColor,
// TextOutA(x,y,str), ReleaseDC). Bails (0) on a null string, a missing frame
// surface, or a failed GetDC; returns 1 on success. Inline /Oi strlen (repnz
// scasb) feeds TextOutA's length. All states inherit this one body.
// ---------------------------------------------------------------------------
RVA(0x000fa6b0, 0xa7)
i32 CState::Vslot17(i32 x, i32 y, char* str, i32 color, i32 bkMode) {
    if (str == 0) {
        return 0;
    }
    CDDSurface* s = m_c->m_drawTarget->m_frontPair->m_surface;
    if (s == 0) {
        return 0;
    }
    HDC hdc = 0;
    s->m_8->GetDC(&hdc);
    if (hdc == 0) {
        return 0;
    }
    SetBkMode(hdc, bkMode);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, str, strlen(str));
    s->m_8->ReleaseDC(hdc);
    return 1;
}

// 0xfa790: two-channel type-3 emitter (3 args).
// @early-stop
// 99.1% - logic byte-faithful. Residual is the chanA/chanB esi<->edi regalloc
// swap (docs/patterns/zero-register-pinning.md family): retail gives the
// longer-lived cached channel (chanB) the preferred callee-saved esi and the
// re-derived channel (chanA) edi, while cl's greedy allocator assigns them the
// other way round; identical structure, a few push/mov reg bytes differ. Not
// source-steerable (computation order is pinned by the chain walk).
RVA(0x000fa790, 0x104)
i32 CSoundFxEmitter::Method_fa790(i32 a1, i32 a2, i32 a3) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_worker->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 0;
    t.m_10 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a2, a3, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// CState::RetireScene(a1,a2,a3,a4) (0xfa8f0): two-channel type-3 screen-transition
// emitter; channel B chosen via a4 + CDDrawWorkerMgr::Method_158d20. No bank-stop
// bracketing on this variant. On CState:
// the retail caller graph shows every screen state (CPreviewState/CAttract/CBootyState/
// CCreditsState/CMulti/CPlay/...) invokes it on its OWN `this`, and the body reads only
// the CState resource-chain facet - so it IS a CState-level helper. m_faderMgr is the
// CState +0x10 member; fxRes() views the +0x0c holder as the emitter resource chain.
// @early-stop
// 98.4% - logic byte-faithful. Same chanA/chanB esi<->edi regalloc swap as
// 0xfa790 plus the deferred-branch arg-temp register choice (see those notes);
// /O2 scheduling/regalloc, not source-steerable.
RVA(0x000fa8f0, 0x118)
i32 CState::RetireScene(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (fxRes()->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = fxRes()->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDrawSurfacePair* holderB;
    if (a4 != 0 && fxRes()->m_worker->Method_158d20() != 0) {
        holderB = fxRes()->m_worker->m_overlayPair;
    } else {
        holderB = fxRes()->m_worker->m_backPair;
    }
    CDDSurface* chanB = holderB->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 0;
    t.m_10 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    if (g_disableFades != 0) {
        ActiveWait(a2);
        fxRes()->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a2, a3, 0);
    }
    mgr->Remove(f);
    return 1;
}

// 0xfaa60: single-channel type-3 emitter (3 args).
RVA(0x000faa60, 0xed)
i32 CSoundFxEmitter::Method_faa60(i32 a1, i32 a2, i32 a3) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_worker->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 1;
    t.m_10 = a1;
    t.m_04 = (i32)chan;
    t.m_08 = 0;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(a2, a3, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// (CPreviewState::LoadScreen @0x0fab90 belongs here; parked in LevelPreview.cpp
// until CPreviewState is homed into a shared header - see the file comment.)

// ---------------------------------------------------------------------------
// CState::Vslot07 (slot 7 / +0x1c, 0x0fac70): the base-state top-window paint poll.
// Its .rdata data-ref is ??_7CState@@6B@+0x1c, and CCreditsState/CSplashState/CHelpState/
// CDemo/CMulti/CPlay all inherit this slot; the callers CAttract::Vslot07 (0x147b0),
// CMultiBootyState::ReadyAndPaint (0x1ce30) and CGuardedDispatch::Run (0x1f870) each
// call CState::Vslot07() on their own `this`. If the owner's game window is present, it
// runs a null Begin/EndPaint cycle and returns 1. Declared in <Gruntz/State.h>; its
// out-of-line COMDAT lands here (retail sited it inside the state-services TU).
// ---------------------------------------------------------------------------
RVA(0x000fac70, 0x4c)
i32 CState::Vslot07() {
    if (!m_4) {
        return 0;
    }
    if (!m_4->m_gameWnd) {
        return 0;
    }
    PAINTSTRUCT ps;
    BeginPaint(m_4->m_gameWnd->m_hwnd, &ps);
    EndPaint(m_4->m_gameWnd->m_hwnd, &ps);
    return 1;
}

// ===========================================================================
// CMgrPersistObj - the persisted game-mgr object. Its Save/Load stream the
// fields through the shared WAP32 CSerialArchive slots (Read @+0x2c / Write
// @+0x30); Init drives the "loading imagez" splash + GAME_IMAGEZ load. Offsets
// + code bytes are load-bearing; field/class names are best-guess placeholders.
// ===========================================================================

DATA(0x0024e35c)
i32 g_playActive;

// (The former CMgrImageSet 20-slot fake-vtable view is GONE: the +0x10 object the
// GAME_IMAGEZ load dispatches into is the REAL CImageRegistry (<Gruntz/ResMgr.h>,
// included above) - the same world+0x10 registry CWorldZ models - and its "Load at
// slot 19 (+0x4c)" IS CImageRegistry::LoadNamespace, the very slot the game-state
// activators (CBootyState/CMenuState/CPlay slot-8 loaders) already reach.)

// CLevelData / CDisplayMgr are the canonical classes: m_levelData IS the canonical
// CDDrawSurfaceMgr (<Gruntz/GameRegistry.h>; its +0x04 CDDrawSubMgrPages worker ==
// the old m_ready, its +0x10 CImageRegistry == the old m_imageSet) and m_displayMgr IS
// the canonical CGruntzMgr (<Gruntz/GruntzMgr.h>; its +0x8c/+0x90 m_modeW/m_modeH ==
// the old m_8c/m_90 video mode). Both are the CState m_c / m_4 prefix. The persisted
// object (CMgrPersistObj) + SplashParams + EngStr_DrawText now live in the shared
// <Gruntz/MgrPersist.h> (included above), not as .cpp-local views.

// @early-stop
// /GX frame-packing artifact (~96%): the instruction stream is byte-faithful, but
// retail reserves `sub esp,0x14` and builds the splash block's tail two dwords in
// the transient arg-push area, where this cl reserves the whole block (`sub esp,
// 0x1c`), shifting every esp-relative displacement by 8; plus the EH scope-table
// symbol is named/represented differently by the delinker.  Logic is complete.
// CMgrPersistObj::Init: hide the cursor, gate on the level being ready,
// draw the "loading imagez" splash (once), resolve the GAME_IMAGEZ rez and load it
// into the image-set, then mark the object started.
// reloc-fidelity: 0xface0 IS CState's slot-8 base virtual (data-ref @0x1ea23c ==
// ??_7CState@@6B@+0x20; called on state `this` as the base image-load gate by
// CBootyState/CMultiBootyState/CImageState/CPlay slot-8 overrides). SYMBOL exports it
// under the canonical CState::InputVirtual name so those overrides' base calls bind;
// the CMgrPersistObj member VIEW is a fake facet of CState (m_levelData@+0x0c ==
// CState::m_c) - its body-shape fold onto CState is deferred.
SYMBOL(?InputVirtual@CState@@UAEHXZ)
RVA(0x000face0, 0x17c)
i32 CMgrPersistObj::Init() {
    if (m_levelData == 0) {
        return 0;
    }
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    while (ShowCursor(0) >= 0)
        ;
    if (m_levelData->m_drawTarget->Method_158bc0() == 0) {
        return 0;
    }
    if (g_playActive == 0) {
        SplashParams sp;
        sp.text.LoadString(0x81a9);
        sp.m_08 = m_displayMgr->m_modeW;
        sp.m_0c = m_displayMgr->m_modeH;
        sp.m_10 = 0;
        sp.m_14 = 0;
        EngStr_DrawText(
            (EngStrRenderObj*)m_levelData,
            (i32)&sp,
            (i32)&sp.m_04,
            0x78,
            1,
            0xff,
            0,
            0xff,
            1
        );
    }
    while (ShowCursor(0) >= 0)
        ;
    g_playActive = 0;
    char* path = static_cast<char*>(m_rezLocator->ResolvePath("GAME_IMAGEZ"));
    if (path == 0) {
        return 0;
    }
    if (m_levelData->m_imageRegistry->LoadNamespace(path, "GAME", "_") == -1) {
        return 0;
    }
    m_1a8 = 0;
    m_1ac = 1;
    m_1b0 = 0;
    return 1;
}

// -------------------------------------------------------------------------
// 0x0faec0. Per-frame
// present/refresh of the bound view: if the suppress latch is set, clear it and
// return; else Refresh the back pair, ShadeRect the back surface with arg0, Flip
// the front surface, then Refresh again. @orphan: reached from CGruntzMgr::
// RunModalDialog/ExitModalUI on an unidentified modal-UI presenter object.
// (g_suppress_64e360 - the present-suppress latch - comes from <Globals.h>.)
// IT IS CState::Present, and the xrefs say so with no ambiguity. 0xfaec0 has exactly two
// callers: CGruntzMgr::RunModalDialog (0x90260) invokes it as `mov ecx,[esi+0x2c]; call
// 0x1ec9` - and CGruntzMgr+0x2c is m_curState, a CState* (<Gruntz/GruntzMgr.h>) - while
// CPlay::Vslot23 (0xcfef0) invokes it on its OWN `this`, and CPlay IS a CState. One receiver
// type, reached two ways. The call is a direct rel32 through an ILT thunk, so it is a plain
// non-virtual - the same CState-level helper shape as RetireScene (0xfa8f0) and
// LoadGameAssetNamespaces (0xf9ea0) next door, which this unit already owns.
// The "Mid_faec0" it hung off is the canonical FxResource (<Gruntz/SoundFxEmitter.h>):
// CState::m_c reinterpreted as the emitter resource chain, +0x04 = the shared
// CDDrawSubMgrPages worker - the exact shape, already reachable through CState::fxRes().
RVA(0x000faec0, 0x67)
void CState::Present(i32 arg0) {
    if (g_suppress_64e360 != 0) {
        g_suppress_64e360 = 0;
        return;
    }
    fxRes()->m_worker->Method_158c70(fxRes()->m_worker->m_backPair);
    fxRes()->m_worker->m_backPair->m_surface->ShadeRect(arg0, (RECT*)0);
    fxRes()->m_worker->m_frontPair->m_surface->Flip((CDDSurface*)0);
    fxRes()->m_worker->Method_158c70(fxRes()->m_worker->m_backPair);
}

// CState::ShadeScreen (0x0faf50): consume the g_suppress latch (return its old value)
// on the frame it is armed; otherwise shade the whole draw surface `pct` percent.
RVA(0x000faf50, 0x31)
i32 CState::ShadeScreen(i32 pct) {
    i32 v = g_suppress_64e360;
    if (v != 0) {
        g_suppress_64e360 = 0;
        return v;
    }
    return m_c->m_drawTarget->m_backPair->m_surface->ShadeRect(pct, 0);
}

// -------------------------------------------------------------------------
// 0x0fafa0. __stdcall(4)
// validity gate: returns 0 if arg0 is null; for kind 4 / 7 validates arg0 through
// a per-kind checker (return 0 on fail); otherwise (and on success) returns 1.
i32 __stdcall Check4_2ce8(i32 h); // 0x0faff0 (kind 4)
i32 __stdcall Check7_36bb(i32 h); // 0x0fb1c0 (kind 7)
// @early-stop
// regalloc wall (topic:wall topic:regalloc): the switch body (cmp 4 je / cmp 7
// jne / kind7 inline / kind4 trailing) is byte-identical to retail; residual is
// a0 landing in ecx (cl) vs eax (retail) - so the a0==0 return needs an extra
// xor eax, and the push/cmp register encodings shift. Not source-steerable. ~93.3%.
// reloc-fidelity: 0xfafa0 IS CPlay::HeaderSerialize - CPlay::SyncState (0xd7520,
// play) calls it __thiscall (mov ecx,edi=this; push ar/mode/a2/a3) as the header
// serialize/mode pre-step. SYMBOL exports it under the canonical CPlay name so that
// call binds; the free-fn Validate_fafa0 view is the recovered-symbol placeholder
// (body-fold onto CPlay deferred - it touches no members here). The param mangles
// PAVCFileMemBase (CSerialArchive is a typedef of the class CFileMemBase), matching
// CPlay::SyncState's emitted HeaderSerialize call - NOT an elaborated struct U.
SYMBOL(?HeaderSerialize@CPlay@@QAEHPAVCFileMemBase@@HHH@Z)
RVA(0x000fafa0, 0x3b)
i32 __stdcall Validate_fafa0(i32 a0, i32 kind, i32 a2, i32 a3) {
    if (a0 == 0) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (Check4_2ce8(a0) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Check7_36bb(a0) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// CMgrPersistObj::Load (0x0faff0, ex SaveRecord::Load - the second view of this
// object, now folded): gate on the archive + the +0x0c level-data slot, then
// stream the same persisted fields through the archive's +0x30 slot (the m_1b0
// tail word is NOT part of this pass).
RVA(0x000faff0, 0x163)
i32 CMgrPersistObj::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!m_levelData) {
        return 0;
    }
    s->Write(&m_1c, 4);
    s->Write(&m_20, 4);
    s->Write(&m_24, 4);
    s->Write(&m_38, 4);
    s->Write(&m_3c, 4);
    s->Write(&m_40, 4);
    s->Write(&m_44, 4);
    s->Write(&m_48, 4);
    s->Write(m_4c, 0x100);
    s->Write(&m_14c, 4);
    s->Write(&m_150, 4);
    s->Write(&m_154, 4);
    s->Write(&m_158, 4);
    s->Write(&m_15c, 4);
    s->Write(m_168, 0x10);
    s->Write(m_178, 0x10);
    s->Write(m_188, 0x10);
    s->Write(m_198, 0x10);
    s->Write(&m_1a8, 4);
    s->Write(&m_1ac, 4);
    return 1;
}

// CMgrPersistObj::Save: gate on the writer + the settings singleton, then
// stream every persisted field through the writer's Read(buf,len) virtual.
RVA(0x000fb1c0, 0x168)
i32 CMgrPersistObj::Save(CSerialArchive* w) {
    if (w == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    w->Read(&m_1c, 4);
    w->Read(&m_20, 4);
    w->Read(&m_24, 4);
    w->Read(&m_38, 4);
    w->Read(&m_3c, 4);
    w->Read(&m_40, 4);
    w->Read(&m_44, 4);
    w->Read(&m_48, 4);
    w->Read(m_4c, 0x100);
    w->Read(&m_14c, 4);
    w->Read(&m_150, 4);
    w->Read(&m_154, 4);
    w->Read(&m_158, 4);
    w->Read(&m_15c, 4);
    w->Read(m_168, 0x10);
    w->Read(m_178, 0x10);
    w->Read(m_188, 0x10);
    w->Read(m_198, 0x10);
    w->Read(&m_1a8, 4);
    w->Read(&m_1ac, 4);
    w->Read(&m_1b0, 4);
    return 1;
}

SIZE(CAttract, 0x1c0); // retail operator-new size (TransitionState 0x8bacf)
SIZE_UNKNOWN(CAttractHost);
SIZE_UNKNOWN(CAttractPooledRes);
SIZE_UNKNOWN(CAttractRegistrar);
SIZE_UNKNOWN(CAttractSceneSlot);
SIZE_UNKNOWN(CAttractScreenObj);
SIZE_UNKNOWN(CAttractVideo);
SIZE_UNKNOWN(CMenuBrightnessHolder);
SIZE_UNKNOWN(CMenuBrightnessReset);
SIZE_UNKNOWN(CMenuBrightnessTarget);
SIZE_UNKNOWN(CMenuRenderM10);
SIZE_UNKNOWN(CMenuRoot);
SIZE_UNKNOWN(CSoundFxEmitter);
SIZE_UNKNOWN(CDDrawSurfacePair);
SIZE_UNKNOWN(FxResource);
SIZE_UNKNOWN(CMgrPersistObj);
SIZE_UNKNOWN(CRezLocator);
SIZE_UNKNOWN(SplashParams);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
