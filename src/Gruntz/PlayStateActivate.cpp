#include <rva.h>

#include <Gruntz/PlayStateActivate.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <EmptyString.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/WwdGameReg.h>

class CGruntzMgr;

RVA(0x000cb800, 0x191)
i32 CPlay::InputVirtual() {
    if (!CState::InputVirtual()) {
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
        NotifyVisibleEntities();
    } else {
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->PruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
    }

    m_guts->Deactivate();
    m_guts->LoadMainStatusBarSprite();
    m_stepCountdown = 2;
    m_world->m_drawTarget->TransTitle();
    RetireScene(0x50, 0x3e8, 0, 1);
    return 1;
}
