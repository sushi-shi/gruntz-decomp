#include <Gruntz/String.h>        // MFC CString (the title-roll formats into one); MFC-first
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/GruntzMgr.h>
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Gruntz/Attract.h>
#include <Gruntz/GameMode.h> // CMenuState + CCreditsState: their slot overrides are DEFINED here (retail TU placement)
#include <Gruntz/GameRegistry.h> // the ONE game-registry shape (CGameRegistry / g_gameReg)
#include <Gruntz/AttractActor.h> // the shared per-frame g_actorList view (also used by CDemo/CHelpState)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSubMgrPages (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <DDrawMgr/DDrawSubMgrPages.h> // the ONE CDDrawSubMgrPages shape (LoadPageImage)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair/m_frontPair->m_surface)
#include <DDrawMgr/DDSurface.h>        // the frame surface CDDSurface (m_10->m_2c: Flip + m_8)
#include <ddraw.h>                     // IDirectDrawSurface (the flip surface's raw +0x8 COM iface)
#include <Gruntz/SoundFxEmitter.h>     // CSoundFxEmitter (the screen-transition emitters)
#include <Gruntz/Fader.h>              // CFaderMgr / CFader / CFxModeT2/T3
#include <Bute/SymParser.h>            // CSymParser (m_symParser->ResolvePath)
#include <Gruntz/SerialArchive.h>      // the ONE shared archive stream (Read@+0x2c / Write@+0x30)
#include <Gruntz/SplashParams.h>       // the "loading imagez" splash-caption params
#include <Gruntz/Play.h>               // CPlay (HeaderSerialize 0xfafa0 is DEFINED here - retail TU placement)
#include <Wap32/EngStr.h>              // THE canonical EngStr_DrawText (0x115440) lean decl
#include <rva.h>

#include <stdio.h>  // sprintf (the "\SCREENZ\%s" path formatter)
#include <string.h> // inline /Oi strlen (repnz scasb) for the Vslot17 TextOutA

#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree/RemoveKeysEqual)
#include <Gruntz/LevelPreview.h> // ex Globals.h

DATA(0x0024e360)
i32 g_suppress_64e360 = 0; // 0x24e360

RVA(0x00039160, 0x46)
i32 CCreditsState::FrameSlot28(i32 unused) {
    owner()->m_sound->IsPlaying();
    owner()->m_sound->StopAndFlush();
    m_2c = static_cast<CSymTab*>(stateMgr()->ResolvePath("STATEZ_ATTRACT"));
    RunTitleSeq("TITLE", 0, 0, 1, 0);
    return 1;
}

// CAttract::LoadTitleConfig - configure the attract/title sequence.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; retail emits a separate inline `xor eax,eax` for the FadeInTitle
// fail return-0, the recompile reuses the already-zero eax. Not steerable by source.
RVA(0x000a03f0, 0x14b)
i32 CMenuState::Vslot09(i32 mode) {
    char stateName[0x20];
    char titleName[0x20];

    if (mode != 2) {
        i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
        sprintf(stateName, "STATEZ_ATTRACT");
        sprintf(titleName, "TITLE%d", idx);

        CSymTab* saved = attractState();
        CSymTab* state = static_cast<CSymTab*>(stateMgr()->ResolvePath(stateName));
        m_2c = (state);
        if (state == 0) {
            return 0;
        }

        i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
        m_2c = (saved);
        if (faded == 0) {
            return 0;
        }

        CDDSurface* tgt = menuRoot()->m_drawTarget->m_backPair->m_surface;
        (static_cast<CDDSurface*>(tgt))
            ->ShadeRect(
                g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32),
                static_cast<tagRECT*>(0)
            );
        menuRoot()->m_drawTarget->TransTitle();
    } else {
        menuRoot()->m_drawTarget->TransEnter();
        CDDSurface* tgt = menuRoot()->m_drawTarget->m_overlayPair->m_surface;
        (static_cast<CDDSurface*>(tgt))
            ->ShadeRect(
                g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32),
                static_cast<tagRECT*>(0)
            );
        menuRoot()->m_drawTarget->TransExit();
    }

    RetireScene(0x50, 0x3e8, 0,
                1); // 0xfa8f0 CState::RetireScene
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(1) < 0) {
        do {
        } while (ShowCursor(1) < 0);
    }
    StartMusic(); // 0xa05a0 (ex the phantom CAttract::CommitStage alias)
    return 1;
}

// CAttract::Activate - virtual attract-screen (re)entry. Gates on slot-3
// (IsActive); if it fails, returns that result. Otherwise resets the title
// brightness target, picks a random TITLE state off the registry, resolves it,
// runs the title fade, sets menu brightness, transitions the page, rebuilds the
// menu page, forces the cursor visible, and returns 1.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; on the FadeInTitle fail path retail emits a fresh inline
// `xor eax,eax` (return 0) while the recompile reuses the already-zero FadeInTitle
// result in eax. Same non-steerable wall as the sibling LoadTitleConfig.
RVA(0x000a0a30, 0x110)
i32 CMenuState::Vslot06() {
    char stateName[0x20];
    char titleName[0x20];

    i32 gate = IsActive();
    if (gate == 0) {
        return gate;
    }

    menuRoot()->m_drawTarget->m_backPair->m_surface->Fill(0);

    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);

    CSymTab* saved = attractState();
    CSymTab* state = static_cast<CSymTab*>(stateMgr()->ResolvePath(stateName));
    m_2c = (state);
    if (state == 0) {
        return 0;
    }

    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (saved);
    if (faded == 0) {
        return 0;
    }

    CDDSurface* tgt = menuRoot()->m_drawTarget->m_backPair->m_surface;
    tgt->ShadeRect(
        g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32),
        static_cast<tagRECT*>(0)
    );
    menuRoot()->m_drawTarget->TransTitle();

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

// CState::FadeInTitle(name, a, b, c, d, e) (0x0fa1f0, 6 args, ret 0x18): resolve the
// "\SCREENZ\<name>" fade page off m_2c (with the screen-type tag), then run the page
// worker's fade (mode 2 when `e`, else 1); on `e` retry once with mode 1. ret 1 on a
// started fade, else 0.
// @early-stop
// frame-reservation wall (~74%): logic + offsets byte-exact; retail reserves an 0x40
// frame (0xc outgoing-arg scratch below the 0x34 buf) where our cl reserves only the
// 0x34 buf. (The resolver callee is the REAL CSymTab::ResolveQualified @0x13be40 now -
// the ex-CAttractScreenObj::ResolveScreen "0x120120" was a misread; that rva is _strchr.)
RVA(0x000fa1f0, 0xc6)
i32 CState::FadeInTitle(const char* name, i32 a, i32 b, i32 c, i32 d, i32 e) {
    static_cast<void>(a);
    static_cast<void>(b);
    static_cast<void>(c);
    static_cast<void>(d);
    if (!m_world) {
        return 0;
    }
    if (!m_symParser) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    char buf[0x34];
    sprintf(buf, "\\SCREENZ\\%s", name);
    CParseSource* page = SymTab2c()->ResolveQualified(buf, &g_screenTag); // 0x13be40
    if (page == 0) {
        return 0;
    }
    CDDrawSubMgrPages* w = menuRoot()->m_drawTarget;
    if (w->LoadPageImage(page, e != 0 ? 2 : 1) != 0) {
        return 1;
    }
    if (e == 0) {
        return 1;
    }
    if (w->LoadPageImage(page, 1) != 0) {
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
    if (!m_world) {
        return 0;
    }
    if (!m_symParser) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    menuRoot()->m_drawTarget->m_frontPair->m_surface->Flip(0);
    return 1;
}

RVA(0x000fa350, 0x84)
i32 CState::RunTitleSeq(const char* name, i32 a, i32 b, i32 c, i32 d) {
    if (!m_world) {
        return 0;
    }
    if (!m_symParser) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    if (FadeInTitle(name, a, b, c, d, 0) == 0) {
        return 0;
    }
    return RunTitle(reinterpret_cast<i32>(name), a, b, c, d) != 0;
}

RVA(0x000fa410, 0xf5)
i32 CSoundFxEmitter::FadeSceneClear1(i32 a1, i32 a2, i32 a3, i32 a4) {
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
    t.m_04 = reinterpret_cast<i32>(chan);
    t.m_08 = 0;
    CFader* f = mgr->Add(1, &t);
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
i32 CSoundFxEmitter::FadeScene1(i32 a1, i32 a2, i32 a3, i32 a4) {
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
    t.m_04 = reinterpret_cast<i32>(chanA);
    t.m_08 = reinterpret_cast<i32>(chanB);
    CFader* f = mgr->Add(1, &t);
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

RVA(0x000fa6b0, 0xa7)
i32 CState::Vslot17(i32 x, i32 y, char* str, i32 color, i32 bkMode) {
    if (str == 0) {
        return 0;
    }
    CDDSurface* s = m_world->m_drawTarget->m_frontPair->m_surface;
    if (s == 0) {
        return 0;
    }
    HDC hdc = 0;
    s->m_ddSurface->GetDC(&hdc);
    if (hdc == 0) {
        return 0;
    }
    SetBkMode(hdc, bkMode);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, str, strlen(str));
    s->m_ddSurface->ReleaseDC(hdc);
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
i32 CSoundFxEmitter::FadeScene2(i32 a1, i32 a2, i32 a3) {
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
    t.m_04 = reinterpret_cast<i32>(chanA);
    t.m_08 = reinterpret_cast<i32>(chanB);
    CFader* f = mgr->Add(2, &t);
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
// emitter; channel B chosen via a4 + CDDrawWorkerMgr::HasOverlay. No bank-stop
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
    if (a4 != 0 && fxRes()->m_worker->HasOverlay() != 0) {
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
    t.m_04 = reinterpret_cast<i32>(chanA);
    t.m_08 = reinterpret_cast<i32>(chanB);
    CFader* f = mgr->Add(2, &t);
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

RVA(0x000faa60, 0xed)
i32 CSoundFxEmitter::FadeSceneClear2(i32 a1, i32 a2, i32 a3) {
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
    t.m_04 = reinterpret_cast<i32>(chan);
    t.m_08 = 0;
    CFader* f = mgr->Add(2, &t);
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

RVA(0x000fac70, 0x4c)
i32 CState::Vslot07() {
    if (!m_mgr) {
        return 0;
    }
    if (!m_mgr->m_gameWnd) {
        return 0;
    }
    PAINTSTRUCT ps;
    BeginPaint(m_mgr->m_gameWnd->m_hwnd, &ps);
    EndPaint(m_mgr->m_gameWnd->m_hwnd, &ps);
    return 1;
}

DATA(0x0024e35c)
i32 g_playActive;

// @early-stop
// /GX frame-packing artifact (~96%): the instruction stream is byte-faithful, but
// retail reserves `sub esp,0x14` and builds the splash block's tail two dwords in
// the transient arg-push area, where this cl reserves the whole block (`sub esp,
// 0x1c`), shifting every esp-relative displacement by 8; plus the EH scope-table
// symbol is named/represented differently by the delinker.  Logic is complete.
// CState's slot-8 base virtual (data-ref @0x1ea23c == ??_7CState@@6B@+0x20;
// base-called by the CBootyState/CMultiBootyState/CImageState/CPlay slot-8
// overrides): hide the cursor, gate on the level being ready, draw the "loading
// imagez" splash (once), resolve the GAME_IMAGEZ rez and load it into the
// image-set, then seed the input latches. (Ex the CMgrPersistObj fake facet's
// "Init" + a SYMBOL name override - both DISSOLVED by the +0x1a8..+0x1b0
// family re-base.)
RVA(0x000face0, 0x17c)
i32 CState::InputVirtual() {
    if (m_world == 0) {
        return 0;
    }
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    while (ShowCursor(0) >= 0)
        ;
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    if (g_playActive == 0) {
        SplashParams sp;
        sp.text.LoadString(0x81a9);
        sp.m_08 = m_mgr->m_modeW;
        sp.m_0c = m_mgr->m_modeH;
        sp.m_10 = 0;
        sp.m_14 = 0;
        EngStr_DrawText(
            m_world,
            reinterpret_cast<i32>(&sp),
            reinterpret_cast<i32>(&sp.m_04),
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
    char* path = static_cast<char*>(m_symParser->ResolvePath("GAME_IMAGEZ"));
    if (path == 0) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(path, "GAME", "_") == -1) {
        return 0;
    }
    m_inputWarmup1 = 0;
    m_inputWarmup2 = 1;
    m_inputHalfSel = 0;
    return 1;
}

RVA(0x000faec0, 0x67)
void CState::Present(i32 arg0) {
    if (g_suppress_64e360 != 0) {
        g_suppress_64e360 = 0;
        return;
    }
    fxRes()->m_worker->BlitPage(fxRes()->m_worker->m_backPair);
    fxRes()->m_worker->m_backPair->m_surface->ShadeRect(arg0, static_cast<RECT*>(0));
    fxRes()->m_worker->m_frontPair->m_surface->Flip(static_cast<CDDSurface*>(0));
    fxRes()->m_worker->BlitPage(fxRes()->m_worker->m_backPair);
}

RVA(0x000faf50, 0x31)
i32 CState::ShadeScreen(i32 pct) {
    i32 v = g_suppress_64e360;
    if (v != 0) {
        g_suppress_64e360 = 0;
        return v;
    }
    return m_world->m_drawTarget->m_backPair->m_surface->ShadeRect(pct, 0);
}

// The state-header serialize/mode pre-step (CPlay::SyncState 0xd7520 calls it
// `mov ecx,edi; push ar/mode/a2/a3`): mode 4 -> the HeaderWrite pass, mode 7 ->
// HeaderRead. The body reads no members, so entry `this` rides ecx untouched
// into the __thiscall CState callees (ex the __stdcall Validate_fafa0 view +
// its Check4_2ce8/Check7_36bb alias decls - all dissolved).
RVA(0x000fafa0, 0x3b)
i32 CPlay::HeaderSerialize(CFileMemBase* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (HeaderWrite(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (HeaderRead(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// The kind-4 state-header WRITE pass (HeaderSerialize @0xfafa0 dispatches here):
// stream the CState header block out via the archive's vtbl+0x30 Write.
RVA(0x000faff0, 0x163)
i32 CState::HeaderWrite(CFileMemBase* ar) {
    if (!ar) {
        return 0;
    }
    if (!m_world) {
        return 0;
    }
    ar->Write(&m_levelIndex, 4);
    ar->Write(&m_levelType, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_38, 4);
    ar->Write(&m_ready, 4);
    ar->Write(&m_notifyLatch, 4);
    ar->Write(&m_44, 4);
    ar->Write(&m_48, 4);
    ar->Write(m_versionString, 0x100);
    ar->Write(&m_14c, 4);
    ar->Write(&m_cursorX, 4);
    ar->Write(&m_cursorY, 4);
    ar->Write(&m_snapOriginX, 4);
    ar->Write(&m_snapOriginY, 4);
    ar->Write(&m_168, 0x10);
    ar->Write(&m_178, 0x10);
    ar->Write(&m_188, 0x10);
    ar->Write(&m_198, 0x10);
    ar->Write(&m_inputWarmup1, 4);
    ar->Write(&m_inputWarmup2, 4);
    return 1;
}

// The kind-7 state-header READ pass (HeaderSerialize's other arm; the symmetric
// vtbl+0x2c Read, plus the m_inputHalfSel tail dword HeaderWrite never streams).
RVA(0x000fb1c0, 0x168)
i32 CState::HeaderRead(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    ar->Read(&m_levelIndex, 4);
    ar->Read(&m_levelType, 4);
    ar->Read(&m_24, 4);
    ar->Read(&m_38, 4);
    ar->Read(&m_ready, 4);
    ar->Read(&m_notifyLatch, 4);
    ar->Read(&m_44, 4);
    ar->Read(&m_48, 4);
    ar->Read(m_versionString, 0x100);
    ar->Read(&m_14c, 4);
    ar->Read(&m_cursorX, 4);
    ar->Read(&m_cursorY, 4);
    ar->Read(&m_snapOriginX, 4);
    ar->Read(&m_snapOriginY, 4);
    ar->Read(&m_168, 0x10);
    ar->Read(&m_178, 0x10);
    ar->Read(&m_188, 0x10);
    ar->Read(&m_198, 0x10);
    ar->Read(&m_inputWarmup1, 4);
    ar->Read(&m_inputWarmup2, 4);
    ar->Read(&m_inputHalfSel, 4);
    return 1;
}

