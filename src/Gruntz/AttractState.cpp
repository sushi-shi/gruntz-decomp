#include <EmptyString.h>          // g_emptyString (ex .cpp extern)
#include <Gruntz/String.h>        // MFC CString (Vslot09's key buffer); MFC-first
#include <Rez/FrameClock.h>       // frame-clock band (g_frameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Attract.h>
#include <Bute/SymParser.h> // CSymParser (m_8: ResolvePath 0x13c030) + CSymTab (m_2c: FindSub 0x13a230)
#include <Gruntz/GameRegistry.h> // CGameRegistry / g_gameReg (+ SoundCue chain: DirectSoundMgr/SoundDevice/SoundStream)
#include <Gruntz/FixedPtrArray32.h>    // the game-controller poll list (g_actorList)
#include <DinMgr2/DirectInputMgr2.h>   // CInputDevBase (Poll/ResetState/m_currentKeys)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // CDDrawSubMgrPages (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (Vslot09 BlitPage)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair/m_frontPair->m_surface)
#include <DDrawMgr/DDSurface.h>        // CDDSurface (Vslot07 Flip; m_10->m_2c)
#include <ddraw.h>                     // IDirectDrawSurface (Render busy IsLost)
#include <rva.h>

#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ScanTree/RemoveKeysEqual)
#include <Gruntz/SoundState.h>            // ex Globals.h transitive
#include <Gruntz/Random.h>                // ex Globals.h transitive

VTBL(CAttract, 0x001ea194);
VTBL(CState, 0x001ea21c);
DATA(0x0020b5bc)
char s_dat60b5bc[] = "2";

RVA(0x00013fb0, 0xd5)
i32 CAttract::LoadGameAssetNamespaces(CGruntzMgr* a, i32 b, i32 mode) {
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

    menuRoot()->m_soundRegistry->ScanTree(static_cast<CSymTab*>(sound), "ATTRACT", "_");

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
    if (reg->m_soundStream) {
        reg->m_soundStream->Stop();
    }
    menuRoot()->m_soundRegistry->RemoveKeysEqual("ATTRACT", "_");
    // Chain the base slot-2 teardown (0xfa150 IS CState::ReleaseResources - the
    // CState vtable slot 2 default body; qualified -> direct call).
    CState::ReleaseResources();
}

// CAttract::Vslot09(arg) (slot 9 / +0x24, 0x014120): the full attract title-screen
// entry (/GX EH frame from the CString format local). Hide the cursor, roll a random
// TITLE%d and run it (as the siblings do), advance the active menu page (BlitPage),
// then - via the inline MS-CRT LCG (== Rng::Next, seeded through the cached timeGetTime
// fn-ptr) - build a random "ATTRACT_TITLE%s" key, look it up in the registrar's
// CMapStringToPtr (m_28+0x10) to (re)acquire the host/sound sub-object (m_host), (re)play
// its voice + latch the idle timeout (or a 0x1f40 default), then poke each g_actorList
// actor's slot-5 virtual. Returns 1. Re-homed from src/Stub/GapFunctions.cpp.
// @early-stop
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

    // m_10 IS a CMapStringToPtr and retail calls the Ptr band (`call 0x1b8438`); the
    // ex "dual-band keep" reinterpret_cast to CMapStringToOb bound the Ob-band Lookup
    // COMDAT at 0x1b8008 instead - mfc_class --audit flagged it WRONG-CLASS.
    void* found = 0;
    menuRoot()->m_soundRegistry->m_10.Lookup(buf, found);
    m_host = static_cast<LeafCue*>(found);
    if (found != 0 && m_activeFlag != 0) {
        if (g_sndEnabled) {
            m_host->m_10->ApplyAndPlay(0x64, 0, 0, 0);
        }
        m_idleTimer = m_host->m_10->m_durationMs + 0x2710;
    } else {
        m_idleTimer = 0x1f40;
    }

    CFixedPtrArray32* list = g_actorList;
    for (i32 i = 0; i < list->m_count; i++) {
        list->m_items[i]->ResetState();
    }
    return 1;
}

// CAttract::FrameSlot28(arg) (slot 10 / +0x28, 0x014340): per-frame voice poll.
// If the host's voice (m_host->m_10) is playing, (re)start it (Restart(0,0x1f4,1)),
// then if it is still playing stop the registrar's pooled resource (Stop(-1)) and
// loop while the voice keeps reporting playing. Returns 1.
// EXACT. The "back-edge coin-flip" (retail `mov eax,[esi+0x1b8] / mov ecx,[eax+0x10]`
// vs our collapsed `mov ecx,[esi+0x1b8] / mov ecx,[ecx+0x10]`) was the loop BODY's
// spelling, not the back edge: reading the registry through a named local used TWICE
// (the null test + the call) instead of latching ->m_2c into a local splits the chain.
// docs/patterns/named-local-keeps-deref-base-in-own-register.md
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
        CDDrawSubMgrLeafScan* reg = menuRoot()->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->PurgeVoiceList(-1);
        }
    } while (m_host->m_10->IsPlaying());
    return 1;
}

// CAttract::Render (slot 5 / +0x14, 0x143e0): the attract-mode per-frame poll/draw.
// If the page's render-busy object reports idle AND the InputVirtual slot reports
// idle, report the exit error (0x8006/0x3e8) and bail. Otherwise stop the registrar's
// pooled resource, tick the m_idleTimer timeout down by the frame delta, run every
// actor's Update(), and if any actor raised its 0x100 flag post the exit WM_COMMAND.
// EXACT. Two real bugs, both fixed: (1) the m_idleTimer countdown was a branch-polarity
// bug - written `if (delta < timer) sub; else zero`, cl emitted `jae`->zero where retail
// emits `jb`->sub; the `if (delta >= timer) zero; else sub` form (matching CDemo::Render)
// flipped it. (2) the m_soundRegistry->m_2c chain collapsed into one register because
// ->m_2c was latched into a local; naming the REGISTRY and reading ->m_2c twice off it
// (null test + call) keeps the base in eax exactly as retail does.
// docs/patterns/named-local-keeps-deref-base-in-own-register.md
RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy = menuRoot()->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (busy == 0 || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(0x8006, 0x3e8);
            return 0;
        }
    }

    CDDrawSubMgrLeafScan* reg = menuRoot()->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->PurgeVoiceList(-1);
    }

    if (g_frameDelta >= m_idleTimer) {
        m_idleTimer = 0;
    } else {
        m_idleTimer -= g_frameDelta;
    }

    CFixedPtrArray32* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_items[i]->Poll();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_items[i]->m_currentKeys & 0x100) {
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
    if (!m_world) {
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

RVA_COMPGEN(0x0008cd60, 0x1e, ??_GCAttract@@UAEPAXI@Z)
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}
