// HelpState.cpp - CHelpState, the help-screen game state (CHelpState : CState,
// RTTI .?AVCHelpState@@, vtbl@0x1e9dfc). Real-class TU merged from the former
// StateLeaf8cf30.cpp (the /GX dtor + vtables) and the CHelpState slot-1 loader
// fragment of BacklogStateLoaders.cpp (the accepted dual-view is now one TU).
//
//   0x08cf30  ~CHelpState - the /GX leaf dtor: stamp the derived vtable (0x5e9dfc =
//             ??_7CHelpState), run the member teardown (0x1357) under the EH frame,
//             then chain the CState vtable (0x5ea21c) + base dtor.
//   0x095090  LoadGameAssetNamespaces (slot 1, ex "LoadAssets") - chain the base asset loader, hide the
//             cursor, resolve the "STATEZ_HELP" bank through the inherited m_8
//             (CBankMgr, cached at m_2c), then pump a fixed message burst through
//             the owner's game window (m_4->m_gameWnd).
//
// Only offsets + code bytes are load-bearing; every engine callee is a reloc-masked
// external. The overridden CState slots are declared so the emitted ??_7CHelpState
// carries CHelpState's overrides (the other slots inherit CState's).
#include <Mfc.h> // ShowCursor (afx-first)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages::Method_158bc0 (m_c->m_04 page gate)

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

// The 11 overridden CState slots (vtbl@0x1e9dfc; the other slots inherited). The
// override bodies live in the class's other (unmatched) TUs; declared-only here.
// CHelpState is the canonical <Gruntz/HelpState.h> class (included above). It had to
// leave this .cpp so CGruntzMgr::TransitionState can `new` the REAL class instead of a
// reduced local twin whose all-base-slot ??_7CHelpState was an ODR landmine.

RVA(0x0008cf30, 0x55)
CHelpState::~CHelpState() {
    CState::ReleaseResources(); // 0xfa150 (the base slot-2 teardown; qualified -> direct)
}

// ---------------------------------------------------------------------------
// 0x08d000 - CSplashState::~CSplashState (/GX): the out-of-line splash-state dtor,
// homed in this RVA band (0x08dxxx, RVA-contiguous with ~CHelpState @0x8cf30). It
// stamps the derived ??_7CSplashState (0x1e9d74, disasm-proven the real CSplashState
// vtable - the identity the CMenuState8d000 placeholder concealed), runs the teardown
// (0x2919 == CSplashState::ReleaseResources, its own slot-2 override @0xf9840 -
// the in-dtor statically-bound call), then folds the CState base (restamp ??_7CState
// @0x5ea21c + base dtor). The class def is shared via <Gruntz/SplashState.h>; defining
// slot-0 here is what emits ??_7CSplashState (the loader TU never does).
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
    m_2c = static_cast<CResSource*>(m_8->ResolvePath("STATEZ_HELP"));
    if (!m_2c) {
        return 0;
    }
    m_4->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

// CHelpState::Vslot09 (0x95140, slot 9) - the mode-restore/title step: re-assert the
// video mode, gate on the page manager being busy (Method_158d20) or acquirable
// (Method_158cb0), roll the help title (FadeInTitle) then retire the prior scene.
// Returns 1 on the full path, 0 on either early-out. Homed from OrphanMethods.cpp
// (was the CState95 placeholder view); identity proven by the ??_7CHelpState@@6B@+0x24
// slot-9 data-ref (via thunk 0x3b43). m_c's +0x04 DDraw pages are the typed
// m_drawTarget (GameRegistry.h coexists with GruntzMgr.h - the old ODR-clash note was
// stale; SplashState.cpp already included both).
RVA(0x00095140, 0x6e)
i32 CHelpState::Vslot09(i32 arg) {
    m_4->RestoreVideoMode(0);
    // The pages ptr is re-read at each call (retail does NOT cache it in a reg across
    // the Method_158d20 call - a caching local would pin it in edi and mismatch).
    if (m_c->m_drawTarget->Method_158d20() == 0
        && m_c->m_drawTarget->Method_158cb0(0, 0x30000) == 0) {
        return 0;
    }
    if (FadeInTitle(reinterpret_cast<const char*>(&g_titleBuf), 0, 0, 0, 0, 1) == 0) {
        return 0;
    }
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)
    return 1;
}

// (The per-frame attract-actor list + element class used to be re-declared HERE, a third
// identical copy of the shape <Gruntz/AttractActor.h> already carries - the file's own TODO
// said "fold both views onto one shared header". Done: the header is included above, and
// g_actorList is its single extern-"C" declaration, DEFINED in MenuState.cpp.)
// The attract registrar's pooled resource: its Stop IS SoundDevice::PurgeVoiceList
// (SoundDevice now from <Gruntz/SoundCue.h>, pulled via GruntzMgr.h->GameRegistry.h).

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
    IDirectDrawSurface* busy = (reinterpret_cast<CMenuRoot*>(m_c))->m_04->m_10->m_2c->m_8;
    if (busy == 0 || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_4->ReportError(0x8006, 0x445);
            return 0;
        }
    }

    CAttractPooledRes* res = (reinterpret_cast<CMenuRoot*>(m_c))->m_28->m_2c;
    if (res) {
        ((SoundDevice*)res)->PurgeVoiceList(-1);
    }

    AttractActorList* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_data[i]->Update();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_data[i]->m_2ac & 0xffffff) {
            PostMessageA(m_4->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
            m_4->m_owner->m_running = 0;
            return 1;
        }
    }
    return 1;
}

// CHelpState::InputVirtual (0x95320, slot 8) - the per-frame poll: gate on the page
// manager's readiness, hide the cursor, roll the help title sequence, hide the cursor
// again, return the sequence result.
RVA(0x00095320, 0x56)
i32 CHelpState::InputVirtual() {
    if (m_c->m_drawTarget->Method_158bc0() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    i32 r = RunTitleSeq(reinterpret_cast<char*>(&g_titleBuf), 0, 0, 1, 0); // 0xfa350 (CState base method)
    while (ShowCursor(FALSE) >= 0) {
    }
    return r;
}

// CHelpState::Vslot06 (0x953a0, slot 6) - activation-ready poll: gate on the state's
// own readiness virtual (Vfunc3), hide the cursor, roll the help title sequence.
RVA(0x000953a0, 0x3c)
i32 CHelpState::Vslot06() {
    if (Vfunc3() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq(reinterpret_cast<char*>(&g_titleBuf), 0, 0, 1, 0); // 0xfa350 (CState base method)
}

// CHelpState::Vslot0c (0x953f0, slot 12) - keydown handler: on ESC/SPACE/ENTER post a
// WM_COMMAND 0x8036 to the top-level window. (Re-homed from ApiCallers CmdHost_0953f0.)
RVA(0x000953f0, 0x37)
i32 CHelpState::Vslot0c(i32 code, i32 unused) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        PostMessageA(m_4->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
    }
    return 1;
}

// CHelpState::Vslot0e (0x95440, slot 14) - unconditional command post: notify the
// top-level window with WM_COMMAND 0x8036.
RVA(0x00095440, 0x24)
i32 CHelpState::Vslot0e(i32, i32, i32) {
    PostMessageA(m_4->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
    return 1;
}
