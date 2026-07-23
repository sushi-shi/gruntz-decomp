#include <Mfc.h>   // CString + <windows.h> (SetCursor)
#include <ddraw.h> // IDirectDrawSurface (the frame surface's IsLost poll, m_c->...->m_2c->m_8)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Dsndmgr/SoundStream.h>       // SoundStream::Stop (ReleaseResources' owned stream)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages::PagesReady (m_c->m_04 page gate)
#include <DDrawMgr/DDSurface.h>        // the frame surface CDDSurface (m_10->m_2c->m_8 IsLost poll)

#include <Gruntz/BankMgr.h>           // CBankMgr::Lookup / CSymTab::LoadGroup (m_8/m_2c)
#include <DinMgr2/DirectInputMgr2.h>  // CInputDevBase (Poll/m_currentKeys press-edge flags)
#include <Gruntz/GameMode.h>          // g_actorList (poll list) + GM_SimpleAnim (Render spine)
#include <Gruntz/State.h>             // CState base (m_4/m_8/m_c/m_2c owner/view/bank facets)
#include <Gruntz/View.h>              // CState::m_c render sub-object facets
#include <Gruntz/GameRegistry.h>      // CDDrawSurfaceMgr (the m_c holder)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSubMgrLeafScan (m_c->m_soundRegistry Install facet)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr::RestoreVideoMode (m_4 facet) + m_gameWnd->m_hwnd
#include <Gruntz/Attract.h> // CSymParser (m_8 facet; ResolvePath). RunTitleSeq @0xfa350 is now a CState base method.
#include <Gruntz/SplashState.h> // CSplashState (shared def; dtor emitted in HelpState.cpp)
#include <rva.h>
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)

#include <Gruntz/AssetRoot.h>

// @confidence: high
// @source: decomp-xref
// @early-stop
// Code bytes 100% byte-identical to retail; some reloc operands can never pair by
// name (g_assetRoot @0x64e25c delinks as ?g_netE25c@@..., the "_" arg as
// ?g_dat60b588@@...), so the function stays at the documented reloc-masked
// plateau, NOT exact. (The CState::LoadGameAssetNamespaces base-chain reloc DOES
// bind now - the slot-1 family unification named it.) See
// docs/patterns/external-nobody-callee.md + reloc-typing-vptr-global.md.
#include <Wap32/GameApp.h> // ex Globals.h
RVA(0x000f9780, 0x8c)
i32 CSplashState::LoadGameAssetNamespaces(i32 a, i32 b, i32 c) {
    if (CAssetRootStorage::s_value.GetLength() == 0) {
        return 0;
    }
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (!CState::LoadGameAssetNamespaces(a, b, c)) {
        return 0;
    }
    SetCursor(0);
    m_mgr->RestoreVideoMode(0);

    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_SPLASH"));
    if (!m_2c) {
        return 0;
    }

    void* soundz = SymTab2c()->FindSub("SOUNDZ");
    if (soundz) {
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(soundz), g_emptyString, "_");
    }
    return 1;
}

// CSplashState::ReleaseResources (0xf9840, slot 2 override) - stop the owned sound
// stream, ClearMap the whole sound sub-manager map, then chain the CState base
// teardown (qualified -> direct call, retail's trailing rel32 to ILT 0x3f53).
// IDENTITY (ex "CGameModeBase::Reset"): retail ??_7CSplashState @0x1e9d74 slot 2
// holds ILT 0x2919 -> 0xf9840 (byte-verified), and its only direct caller is
// ~CSplashState @0x8d02b (the in-dtor statically-bound own-override call). Homed
// here from GameMode.cpp (this TU owns the 0xf97xx-0xf9bxx CSplashState band).
// m_c->m_soundRegistry is re-read each statement (retail does not cache it).
// @early-stop
// ~98.7% - m_28-intermediate regalloc wall (retail reuses eax->eax->ecx; cl picks
// fresh ecx/edx) - a 2-3 byte modrm micro-diff, not source-steerable.
RVA(0x000f9840, 0x29)
void CSplashState::ReleaseResources() {
    if (m_world->m_soundRegistry->m_2c != 0) {
        m_world->m_soundRegistry->m_2c->Stop();
    }
    m_world->m_soundRegistry->ClearMap();
    CState::ReleaseResources();
}

// CSplashState::Render (0xf9920, slot 5) - the per-frame splash draw spine (the
// canonical CState Render shape, cf. CCreditsState::Render): input poll -> input-
// virtual bail -> cursor anim -> title-timer countdown -> per-entity Update loop
// -> flagged-entity/timer-expiry command post. The +0x1b8 timer counts down by the
// frame delta (clamped at 0); when an entity latches flag&1 OR the timer has expired
// (m_1b8==0), post WM_COMMAND 0x8023 to the game window and clear the app run gate.
//
// @early-stop
// Byte-exact except the cursor-anim gate `if(m_c->m_soundRegistry->m_2c)`:
// retail materializes the chain reusing eax (`mov eax,[eax+0x28]; mov ecx,[eax+0x2c];
// cmp ecx,esi`), cl keeps m_c in eax and folds the m_2c load into the cmp
// (`mov ecx,[eax+0x28]; cmp [ecx+0x2c],esi`) - the reread-member-view-pointer
// register wall (docs/patterns/reread-member-view-pointer.md). The direct analog
// CCreditsState::Render (identical source line) hits the same wall at 97% and
// LevelPreview::Tick @early-stops on it too. Logic + control flow + all externs
// byte-exact; final sweep.
RVA(0x000f9920, 0x108)
i32 CSplashState::Render() {
    IDirectDrawSurface* in = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            m_mgr->ReportError(0x8006, 0x447);
            return 0;
        }
    }

    if (m_world->m_soundRegistry->m_2c) {
        GM_SimpleAnim(-1);
    }

    if (static_cast<u32>(g_wap32FrameDelta) >= m_1b8) {
        m_1b8 = 0;
    } else {
        m_1b8 = m_1b8 - g_wap32FrameDelta;
    }

    {
        CFixedPtrArray32* L = g_actorList;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_items[i]->Poll();
        }
    }

    {
        CFixedPtrArray32* L = g_actorList;
        i32 n = L->m_count;
        i32 j;
        for (j = 0; j < n; j++) {
            if (L->m_items[j]->m_currentKeys & 1) {
                goto post;
            }
        }
        if (m_1b8) {
            return 1;
        }
    }
post:
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    m_mgr->m_owner->m_running = 0;
    return 1;
}

RVA(0x000f9a80, 0x44)
i32 CSplashState::InputVirtual() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq(
        static_cast<const char*>(CAssetRootStorage::s_value),
        0,
        0,
        1,
        0
    ); // 0xfa350 (CState base method)
}

RVA(0x000f9af0, 0x3e)
i32 CSplashState::Vslot06() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq(
        static_cast<const char*>(CAssetRootStorage::s_value),
        0,
        0,
        1,
        0
    ); // 0xfa350 (CState base method)
}

RVA(0x000f9b40, 0x37)
i32 CSplashState::Vslot0c(i32 code, i32) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

RVA(0x000f9b90, 0x24)
i32 CSplashState::Vslot0e(i32, i32, i32) {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    return 1;
}
