#include <Gruntz/String.h> // MFC CString (Vslot09's CMapStringToOb/CObject); MFC-first
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Attract.h>
#include <Bute/SymParser.h> // CSymParser (m_8: ResolvePath 0x13c030) + CSymTab (m_2c: FindSub 0x13a230)
#include <Gruntz/GameRegistry.h> // CGameRegistry / g_gameReg (+ SoundCue chain: DirectSoundMgr/SoundDevice/SoundStream)
#include <Gruntz/AttractActor.h>       // the shared per-frame g_actorList view
#include <DDrawMgr/DDrawSurfaceMgr.h>  // CDDrawSubMgrPages (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (Vslot09 BlitPage)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair/m_frontPair->m_surface)
#include <DDrawMgr/DDSurface.h>        // CDDSurface (Vslot07 Flip; m_10->m_2c)
#include <ddraw.h>                     // IDirectDrawSurface (Render busy IsLost)
#include <rva.h>
#include <Globals.h> // Vslot09: g_randSeeded / g_randSeed

#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ScanTree/RemoveKeysEqual)

extern "C" {
    extern u32 g_frameDelta;
}

DATA(0x0020b5bc)
char s_dat60b5bc[] = "2";
extern char g_emptyString[];

RVA(0x00013fb0, 0xd5)
i32 CAttract::LoadGameAssetNamespaces(i32 a, i32 b, i32 mode) {
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (CState::LoadGameAssetNamespaces(a, b, mode) == 0) {
        return 0;
    }

    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    owner()->RestoreVideoMode(0);

    CSymTab* state = static_cast<CSymTab*>(stateMgr()->ResolvePath("STATEZ_ATTRACT"));
    m_2c = (state);
    if (state == 0) {
        return 0;
    }

    void* sound = state->FindSub("SOUNDZ");
    if (sound == 0) {
        return 0;
    }

    menuRoot()->m_soundRegistry->ScanTree_157ee0(static_cast<CSymTab*>(sound), "ATTRACT", "_");

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    if (mode == 3) {
        m_activeFlag = 0;
        m_host = 0;
    } else {
        m_activeFlag = 1;
        m_host = 0;
    }
    return 1;
}

RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    CDDrawSubMgrLeafScan* reg = menuRoot()->m_soundRegistry;
    if (reg->m_2c) {
        reg->m_2c->Stop();
    }
    menuRoot()->m_soundRegistry->RemoveKeysEqual_157c70("ATTRACT", "_");
    // Chain the base slot-2 teardown (0xfa150 IS CState::ReleaseResources - the
    // CState vtable slot 2 default body; qualified -> direct call).
    CState::ReleaseResources();
}

// CAttract::Vslot09(arg) (slot 9 / +0x24, 0x014120): the full attract title-screen
// entry (/GX EH frame from the CString format local). Hide the cursor, roll a random
// TITLE%d and run it (as the siblings do), advance the active menu page (BlitPage),
// then - via the inline MS-CRT LCG (== Rng::Next, seeded through the cached timeGetTime
// fn-ptr) - build a random "ATTRACT_TITLE%s" key, look it up in the registrar's
// CMapStringToOb (m_28+0x10) to (re)acquire the host/sound sub-object (m_host), (re)play
// its voice + latch the idle timeout (or a 0x1f40 default), then poke each g_actorList
// actor's slot-5 virtual. Returns 1. Re-homed from src/Stub/GapFunctions.cpp.
// @early-stop
// 98.3%: the whole 425B body - the /GX frame, the ShowCursor roll, the TITLE%d format +
// RunTitleSeq, the page fade, the inline MS-CRT LCG + %2 pick, the wsprintfA, the
// CMapStringToOb::Lookup host (re)acquire, the sound/idle branch and the actor slot-5
// loop - is byte-faithful. The only residue is a pair of register-selection coin-flips:
// retail seats m_c in eax across the Lookup argument setup where cl uses ecx, and loads
// the actor's vptr through eax where cl uses edx. A pure regalloc choice (operand-order /
// spelling variations do not flip it; docs/patterns/zero-register-pinning.md family).
// Deferred to the final sweep.
RVA(0x00014120, 0x1a9)
i32 CAttract::Vslot09(i32 arg) {
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    RunTitleSeq(s, 0, 0, 1, 0);
    CDDrawSubMgrPages* page = menuRoot()->m_drawTarget;
    page->BlitPage(page->m_backPair);

    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = ::timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    i32 r = (g_randSeed >> 0x10) & 0x7fff;
    const char* pick = (r % 2) ? s_dat60b5bc : g_emptyString;

    char buf[0x40];
    ::wsprintfA(buf, "ATTRACT_TITLE%s", pick);

    CMapStringToOb* map = reinterpret_cast<CMapStringToOb*>(&menuRoot()->m_soundRegistry->m_10); // the Ob-band read of the Ptr map (documented dual-band keep)
    CObject* found = 0;
    map->Lookup(buf, found);
    m_host = reinterpret_cast<CAttractHost*>(found);
    if (found != 0 && m_activeFlag != 0) {
        if (g_sndEnabled) {
            m_host->m_10->ApplyAndPlay(0x64, 0, 0, 0);
        }
        m_idleTimer = m_host->m_10->m_durationMs + 0x2710;
    } else {
        m_idleTimer = 0x1f40;
    }

    AttractActorList* list = g_actorList;
    for (i32 i = 0; i < list->m_count; i++) {
        list->m_data[i]->Vslot05();
    }
    return 1;
}

// CAttract::FrameSlot28(arg) (slot 10 / +0x28, 0x014340): per-frame voice poll.
// If the host's voice (m_host->m_10) is playing, (re)start it (Restart(0,0x1f4,1)),
// then if it is still playing stop the registrar's pooled resource (Stop(-1)) and
// loop while the voice keeps reporting playing. Returns 1.
// @early-stop
// regalloc back-edge coin-flip (docs/patterns/zero-register-pinning.md): body
// byte-identical except the final loop-back IsPlaying load - retail re-reads
// m_host through eax (8b 86 .. 8b 48 10), the recompile through ecx (8b 8e .. 8b 49
// 10). A pure allocator choice on the do-while back-edge; no source lever flips it.
RVA(0x00014340, 0x71)
i32 CAttract::FrameSlot28(i32 arg) {
    if (m_host == 0) {
        return 1;
    }
    if (!m_host->m_10->IsPlaying()) {
        return 1;
    }
    m_host->m_10->CloneAndPlay(0, 0x1f4, 1);
    if (!m_host->m_10->IsPlaying()) {
        return 1;
    }
    do {
        SoundStream* r = menuRoot()->m_soundRegistry->m_2c;
        if (r) {
            r->PurgeVoiceList(-1);
        }
    } while (m_host->m_10->IsPlaying());
    return 1;
}

// CAttract::Render (slot 5 / +0x14, 0x143e0): the attract-mode per-frame poll/draw.
// If the page's render-busy object reports idle AND the InputVirtual slot reports
// idle, report the exit error (0x8006/0x3e8) and bail. Otherwise stop the registrar's
// pooled resource, tick the m_idleTimer timeout down by the frame delta, run every
// actor's Update(), and if any actor raised its 0x100 flag post the exit WM_COMMAND.
// Code byte-identical to retail (~97% fuzzy = reloc-masked plateau): the residual
// is purely cross-unit/IAT symbol-naming on three reloc operands - ReportError (a
// bare delinker label), 0x136e20 (already owned by DirectSoundMgr::winapi_136e20_
// timeGetTime; the sibling FrameSlot28 names it CAttractPooledRes::Stop too), and
// the PostMessageA IAT call (target bakes a bare absolute 0x6c44c8, no symbol).
// Not source-steerable; topic:scoring-artifact (docs/matching-patterns.md).
// @early-stop
// reloc-masked IAT/cross-unit operands only (see above); code bytes byte-exact.
RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy = menuRoot()->m_drawTarget->m_frontPair->m_surface->m_8;
    if (busy == 0 || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(0x8006, 0x3e8);
            return 0;
        }
    }

    SoundStream* res = menuRoot()->m_soundRegistry->m_2c;
    if (res) {
        res->PurgeVoiceList(-1);
    }

    if (g_frameDelta < m_idleTimer) {
        m_idleTimer -= g_frameDelta;
    } else {
        m_idleTimer = 0;
    }

    AttractActorList* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_data[i]->Update();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_data[i]->m_2ac & 0x100) {
            ::PostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            return 1;
        }
    }
    return 1;
}

RVA(0x00014520, 0xc3)
i32 CAttract::InputVirtual() {
    // The page "loaded?" gate is CDDrawSubMgrPages::PagesReady (0x158bc0), reached
    // through the page's real class (the CMenuPage view's IsLoaded @0x158bc0 == this).
    if (menuRoot()->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    return RunTitleSeq(s, 0, 0, 1, 0);
}

RVA(0x00014630, 0xbd)
i32 CAttract::Vslot06() {
    if (IsActive() == 0) {
        return 0;
    }
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    return RunTitleSeq(s, 0, 0, 1, 0);
}

RVA(0x00014720, 0x37)
i32 CAttract::Vslot0c(i32 code, i32 unused) {
    if (code == 0x20 || code == 0xd || code == 0x1b) {
        ::PostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

RVA(0x00014770, 0x24)
i32 CAttract::Vslot0e(i32, i32, i32) {
    ::PostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    return 1;
}

RVA(0x000147b0, 0x6a)
i32 CAttract::Vslot07() {
    if (!IsActive()) {
        return 0;
    }
    if (!m_c) {
        return 0;
    }
    if (!CState::Vslot07()) {
        return 0;
    }
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg.
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    menuRoot()->m_drawTarget->m_frontPair->m_surface->Flip(0);
    menuRoot()->m_drawTarget->BlitPage(menuRoot()->m_drawTarget->m_backPair);
    return 1;
}

RVA(0x0008cd40, 0x6)
GameStateId CAttract::Update() {
    return GAMESTATE_ATTRACT;
}

RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}

SIZE(CAttract, 0x1c0); // retail operator-new size (TransitionState 0x8bacf)
SIZE_UNKNOWN(CAttractHost);
SIZE_UNKNOWN(CAttractVideo);
SIZE_UNKNOWN(CDDrawSurfacePair);
