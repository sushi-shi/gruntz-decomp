#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Mfc.h>                          // ShowCursor (afx-first)
#include <EmptyString.h>                  // g_emptyString
#include <Bute/SymTab.h>                  // canonical CSymTab (ResolvePath @0x13bae0)

#include <Gruntz/Play.h>
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (the VisitVisible chain arg)       // the real CPlay : CState (method owner)
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawWorkerRegistry (+0x10 image registrar) + CDDrawSubMgrPages
#include <rva.h>
#include <Gruntz/GameLevel.h>          // canonical CGameLevel (VisitVisible)
#include <DinMgr2/DirectInputMgr2.h>   // canonical DirectInputMgr2 (ReadAll)
#include <DinMgr2/InputMgrPtr.h>       // g_inputMgr (DirectInputMgr2* view; the one decl)
#include <DDrawMgr/DDrawSubMgrPages.h> // canonical CDDrawSubMgrPages (TransTitle)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (m_surface Fill target)
#include <DDrawMgr/DDrawWorkerList.h>  // renderer B (PruneWorkers - the "present")
#include <DDrawMgr/DDSurface.h>        // CDDSurface::Fill (0x13e760)
#include <Gruntz/StatusBarMgr.h>       // canonical CStatusBarMgr (m_guts Deactivate/Load...)

class CGruntzMgr;
void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, i32 snapFlag); // reloc-masked

RVA(0x000cb800, 0x191)
i32 CPlay::InputVirtual() {
    if (!CState::
            InputVirtual()) { // 0xface0 CState base activate gate (was fake CMgrPersistObj::Init)
        return 0;
    }
    while (ShowCursor(FALSE) >= 0)
        ;

    void* h = m_levelBank->ResolvePath("TILEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, g_emptyString, "_") == -1) {
        return 0;
    }

    h = m_levelBank->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = m_gruntzBank->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_inputMgr->ReadAll();
    while (ShowCursor(FALSE) >= 0)
        ;

    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);

    if (m_region1Gate != 0) {
        NotifyVisibleEntities(); // CPlay @0xd9050
    } else {
        m_world->m_level->VisitVisible(static_cast<void*>(m_world->m_drawTarget->m_backPair), m_world->m_childGroup);
        m_world->m_workerList->PruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
    }

    m_guts->Deactivate();
    m_guts->LoadMainStatusBarSprite();
    m_stepCountdown = 2;
    m_world->m_drawTarget->TransTitle();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited by CPlay, cast-free)
    return 1;
}
