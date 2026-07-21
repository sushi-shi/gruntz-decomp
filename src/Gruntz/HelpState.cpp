#include <Mfc.h> // ShowCursor (afx-first)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>  // CDDrawSubMgrPages::PagesReady (the page gate)
#include <DDrawMgr/DDrawSurfacePair.h> // the front pair's held surface (Render's busy probe)
#include <DDrawMgr/DDSurface.h>

#include <Gruntz/BankMgr.h>      // CBankMgr::Lookup (inherited m_8) -> CResSource
#include <Gruntz/GruntzMgr.h>    // CGruntzMgr m_4 + m_gameWnd->PumpMessages (pulls State.h/Wap32.h)
#include <Gruntz/HelpState.h>    // canonical CHelpState (was defined locally here)
#include <Gruntz/SplashState.h>  // CSplashState (the 0x8d000 /GX out-of-line dtor)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (the typed CState::m_c holder)
#include <Gruntz/Attract.h>      // CMenuRoot chain (m_c): Render's busy surface + attract registrar
#include <DDrawMgr/DDSurface.h> // CDDSurface::m_8 (the held IDirectDrawSurface, Render's busy gate)
#include <ddraw.h>              // IDirectDrawSurface::IsLost (slot 24) - Render's busy poll
#include <Globals.h>            // g_titleBuf (RunTitleSeq title buffer)
#include <Gruntz/AttractActor.h> // the shared AttractActor/AttractActorList + g_actorList
#include <rva.h>

RVA(0x0008cf30, 0x55)
CHelpState::~CHelpState() {
    CState::ReleaseResources(); // 0xfa150 (the base slot-2 teardown; qualified -> direct)
}

RVA(0x0008d000, 0x55)
CSplashState::~CSplashState() {
    CSplashState::ReleaseResources(); // 0xf9840 (own slot-2 override; in-dtor static bind)
}

RVA(0x00095090, 0x6e)
i32 CHelpState::LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) {
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (!CState::LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;
    m_2c = static_cast<CResSource*>(m_symParser->ResolvePath("STATEZ_HELP"));
    if (!m_2c) {
        return 0;
    }
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

RVA(0x00095140, 0x6e)
i32 CHelpState::Vslot09(i32 arg) {
    m_mgr->RestoreVideoMode(0);
    // The pages ptr is re-read at each call (retail does NOT cache it in a reg across
    // the Method_158d20 call - a caching local would pin it in edi and mismatch).
    if (m_world->m_drawTarget->Method_158d20() == 0
        && m_world->m_drawTarget->Method_158cb0(0, 0x30000) == 0) {
        return 0;
    }
    if (FadeInTitle(reinterpret_cast<const char*>(&g_titleBuf), 0, 0, 0, 0, 1) == 0) {
        return 0;
    }
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)
    return 1;
}

// CHelpState::Render (0x951f0, slot 5) - the help/attract per-frame poll/draw: when the
// menu page's busy surface reports idle AND the InputVirtual slot reports idle, report the
// exit error (0x8006/0x445) and bail; else stop the registrar's pooled resource, run every
// attract actor's Update(), and if any actor raised its flags post the exit WM_COMMAND
// (0x8036) and clear the app run gate. Byte-identical to retail except reloc-masked
// operands (ReportError bare label, PostMessageA IAT-absolute, PurgeVoiceList cross-unit);
// same scoring-artifact plateau as the sibling CAttract::Render (docs/matching-patterns.md).
// @early-stop
// 99.89%: complete + correct. Residual is a regalloc coin-flip on the res-chain
// intermediate register (retail `mov eax,[eax+0x28]; mov ecx,[eax+0x2c]`, our cl
// `mov ecx,[eax+0x28]; mov ecx,[ecx+0x2c]` - same value, m_28 pinned in eax vs ecx;
// permuter no-change) plus the reloc-masked IAT/cross-unit operands the sibling
// CAttract::Render documents (ReportError/PostMessageA/PurgeVoiceList). topic:regalloc.
RVA(0x000951f0, 0xeb)
i32 CHelpState::Render() {
    IDirectDrawSurface* busy = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (busy == 0 || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(0x8006, 0x445);
            return 0;
        }
    }

    SoundStream* res = m_world->m_soundRegistry->m_2c;
    if (res) {
        res->PurgeVoiceList(-1);
    }

    AttractActorList* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_data[i]->Update();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_data[i]->m_2ac & 0xffffff) {
            PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
            m_mgr->m_owner->m_running = 0;
            return 1;
        }
    }
    return 1;
}

RVA(0x00095320, 0x56)
i32 CHelpState::InputVirtual() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    i32 r = RunTitleSeq(reinterpret_cast<char*>(&g_titleBuf), 0, 0, 1, 0); // 0xfa350 (CState base method)
    while (ShowCursor(FALSE) >= 0) {
    }
    return r;
}

RVA(0x000953a0, 0x3c)
i32 CHelpState::Vslot06() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq(reinterpret_cast<char*>(&g_titleBuf), 0, 0, 1, 0); // 0xfa350 (CState base method)
}

RVA(0x000953f0, 0x37)
i32 CHelpState::Vslot0c(i32 code, i32 unused) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
    }
    return 1;
}

RVA(0x00095440, 0x24)
i32 CHelpState::Vslot0e(i32, i32, i32) {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
    return 1;
}
