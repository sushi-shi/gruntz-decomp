#include <rva.h>

#include <Gruntz/SplashState.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundStream.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/State.h>
#include <Gruntz/View.h>
#include <Wap32/GameApp.h>

#include <ddraw.h>

RVA(0x000f9780, 0x8c)
i32 CSplashState::LoadGameAssetNamespaces(CGruntzMgr* a, i32 b, i32 c) {
    if (CAssetRootStorage::s_value.GetLength() == 0) {
        return 0;
    }

    if (!CState::LoadGameAssetNamespaces(a, b, c)) {
        return 0;
    }
    SetCursor(0);
    m_mgr->RestoreVideoMode(0);

    m_stateBank = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_SPLASH"));
    if (!m_stateBank) {
        return 0;
    }

    void* soundz = SymTab2c()->FindSub("SOUNDZ");
    if (soundz) {
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(soundz), g_emptyString, "_");
    }
    return 1;
}

RVA(0x000f9840, 0x29)
void CSplashState::ReleaseResources() {
    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != 0) {
        reg->m_soundStream->Stop();
    }
    m_world->m_soundRegistry->ClearMap();
    CState::ReleaseResources();
}

RVA(0x000f9920, 0x108)
i32 CSplashState::Render() {
    IDirectDrawSurface* in = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            m_mgr->ReportError(0x8006, 0x447);
            return 0;
        }
    }

    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->PurgeVoiceList(-1);
    }

    if (static_cast<u32>(g_wap32FrameDelta) >= m_splashCountdownMs) {
        m_splashCountdownMs = 0;
    } else {
        m_splashCountdownMs = m_splashCountdownMs - g_wap32FrameDelta;
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
        if (m_splashCountdownMs) {
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
    return RunTitleSeq(static_cast<const char*>(CAssetRootStorage::s_value), 0, 0, 1, 0);
}

RVA(0x000f9af0, 0x3e)
i32 CSplashState::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return RunTitleSeq(static_cast<const char*>(CAssetRootStorage::s_value), 0, 0, 1, 0);
}

RVA(0x000f9b40, 0x37)
i32 CSplashState::OnKeyDown(i32 code, i32) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

RVA(0x000f9b90, 0x24)
i32 CSplashState::OnLButtonDown(i32, i32, i32) {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    return 1;
}
