// SplashState.cpp - the splash-screen game state (its own TU; LoadSounds is the
// only reconstructed method so far). Re-homed from src/Stub/SplashState.cpp
// (C:\Proj\Gruntz).
//
// CSplashState : public CState (RTTI .?AVCSplashState@@, vtbl@0x1e9d74) - a real
// CState leaf. LoadSounds reads the shared CState owner/view/bank facets cast-free
// through their real INHERITED types:
//   * m_4  (CGruntzMgr*)  - RestoreVideoMode (re-assert the 640x480 display mode).
//   * m_8  (CBankMgr*)    - Lookup the "STATEZ_SPLASH" bank -> CResSource.
//   * m_2c (CResSource*)  - the cached splash bank; LoadGroup its "SOUNDZ" set.
//   * m_c  (CDDrawSurfaceMgr*)       - its +0x28 sound registry Install()s the loaded set.
// The 0x48ddd0/0x53c030/0x53a230/0x557ee0 call targets are the VA form (RVA +
// 0x400000 image base) of CGruntzMgr::RestoreVideoMode (0x08ddd0), CBankMgr::Lookup
// (0x13c030), CResSource::LoadGroup (0x13a230) and CDDrawSubMgrLeafScan::Install
// (0x157ee0) - the SAME engine methods the in-game CPlay resource facet
// already models; here they are reloc-masked externals.
//
// The gating global g_assetRoot is the REAL MFC CString: CString::GetLength() inlines
// to GetData()->nDataLength == m_pchData[-2], byte-identical to the load's `mov
// eax,ds:g; mov ecx,[eax-8]; test ecx,ecx` guard.
#include <Mfc.h>   // CString + <windows.h> (SetCursor)
#include <ddraw.h> // IDirectDrawSurface (the frame surface's IsLost poll, m_c->...->m_2c->m_8)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages::Method_158bc0 (m_c->m_04 page gate)
#include <DDrawMgr/DDSurface.h>        // the frame surface CDDSurface (m_10->m_2c->m_8 IsLost poll)

#include <Gruntz/BankMgr.h>      // CBankMgr::Lookup / CResSource::LoadGroup (m_8/m_2c)
#include <Gruntz/GameMode.h>     // CGMEntity/CGMEntityList/g_actorList/GM_SimpleAnim (Render spine)
#include <Gruntz/State.h>        // CState base (m_4/m_8/m_c/m_2c owner/view/bank facets)
#include <Gruntz/View.h>         // CState::m_c render sub-object facets
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (the m_c holder)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSubMgrLeafScan (m_c->m_soundRegistry Install facet)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr::RestoreVideoMode (m_4 facet) + m_gameWnd->m_hwnd
#include <Gruntz/Attract.h> // CSymParser (m_8 facet; ResolvePath). RunTitleSeq @0xfa350 is now a CState base method.
#include <Gruntz/SplashState.h> // CSplashState (shared def; dtor emitted in HelpState.cpp)
#include <rva.h>
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)

// The global empty C string the sound loader's prefix is seeded from (0x6293f4).

// The global asset-root CString whose emptiness gates the load (0x64e25c;
// declared in <Gruntz/AssetRoot.h>, DATA home NetMgrMisc.cpp).
#include <Gruntz/AssetRoot.h>

// The engine's just-refreshed per-frame clock delta (?g_wap32FrameDelta@@3HA); the
// splash title timer counts down by it (0x653c74).
extern i32 g_wap32FrameDelta; // 0x653c74

// CSplashState (: CState, vtbl@0x1e9d74) - the shared class def now lives in
// <Gruntz/SplashState.h> so its out-of-line dtor (0x08d000, emitted in HelpState.cpp)
// stamps the real ??_7CSplashState. This loader TU never defines a virtual body, so
// cl emits no ??_7CSplashState here (member-offset codegen unchanged).

// @confidence: high
// @source: decomp-xref
// @early-stop
// Code bytes 100% byte-identical to retail; the reloc operands can never pair by
// name (g_assetRoot @0x64e25c delinks as ?g_netE25c@@..., the "_" arg as
// ?g_dat60b588@@..., and LoadGameAssetNamespaces is an unnamed placeholder), so the
// function stays at the documented reloc-masked plateau, NOT exact. See
// docs/patterns/external-nobody-callee.md + reloc-typing-vptr-global.md.
RVA(0x000f9780, 0x8c)
i32 CSplashState::LoadSounds(i32 a, i32 b, i32 c) {
    if (g_assetRoot.GetLength() == 0) {
        return 0;
    }
    if (!LoadGameAssetNamespaces(a, b, c)) {
        return 0;
    }
    SetCursor(0);
    m_4->RestoreVideoMode(0);

    m_2c = (CResSource*)m_8->ResolvePath("STATEZ_SPLASH");
    if (!m_2c) {
        return 0;
    }

    void* soundz = SymTab2c()->FindSub("SOUNDZ");
    if (soundz) {
        m_c->m_soundRegistry->ScanTree_157ee0((CSymTab*)soundz, g_emptyString, "_");
    }
    return 1;
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
    IDirectDrawSurface* in = m_c->m_drawTarget->m_frontPair->m_surface->m_8;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            m_4->ReportError(0x8006, 0x447);
            return 0;
        }
    }

    if (m_c->m_soundRegistry->m_2c) {
        GM_SimpleAnim(-1);
    }

    if ((u32)g_wap32FrameDelta >= m_1b8) {
        m_1b8 = 0;
    } else {
        m_1b8 = m_1b8 - g_wap32FrameDelta;
    }

    {
        CGMEntityList* L = g_actorList;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_data[i]->Update();
        }
    }

    {
        CGMEntityList* L = g_actorList;
        i32 n = L->m_count;
        i32 j;
        for (j = 0; j < n; j++) {
            if (L->m_data[j]->m_2ac & 1) {
                goto post;
            }
        }
        if (m_1b8) {
            return 1;
        }
    }
post:
    PostMessageA(m_4->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    m_4->m_owner->m_running = 0;
    return 1;
}

// CSplashState::InputVirtual (0xf9a80, slot 8) - per-frame poll: gate on the page
// manager's readiness, hide the cursor, roll the splash title sequence with the
// current asset-root name, return its result.
RVA(0x000f9a80, 0x44)
i32 CSplashState::InputVirtual() {
    if (m_c->m_drawTarget->Method_158bc0() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq((const char*)g_assetRoot, 0, 0, 1, 0); // 0xfa350 (CState base method)
}

// CSplashState::Vslot06 (0xf9af0, slot 6) - activation-ready poll: gate on the state's
// own readiness virtual (Vfunc3), hide the cursor, roll the splash title sequence.
RVA(0x000f9af0, 0x3e)
i32 CSplashState::Vslot06() {
    if (Vfunc3() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq((const char*)g_assetRoot, 0, 0, 1, 0); // 0xfa350 (CState base method)
}

// CSplashState::Vslot0c (0xf9b40, slot 12) - key handler: on ESC/SPACE/ENTER post a
// WM_COMMAND 0x8023 to the top-level window.
RVA(0x000f9b40, 0x37)
i32 CSplashState::Vslot0c(i32 code, i32) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        PostMessageA(m_4->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

// CSplashState::Vslot0e (0xf9b90, slot 14) - unconditional command post: notify the
// top-level window with WM_COMMAND 0x8023.
RVA(0x000f9b90, 0x24)
i32 CSplashState::Vslot0e(i32, i32, i32) {
    PostMessageA(m_4->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    return 1;
}

VTBL(CSplashState, 0x001e9d74); // real class binds its own vtable (was placeholder CEngObj_1e9d74)
