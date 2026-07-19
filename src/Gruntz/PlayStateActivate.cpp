// PlayStateActivate.cpp - CPlay::OnActivate (0x0cb800), the PLAY level-state
// activation (vtable slot 8; reached directly by CTriggerMgr::NotifyCell). Re-homed
// from the artifact "GameLevelState" placeholder of BacklogStateLoaders.cpp: RVA
// 0x0cb800 sits inside CPlay's method band (between CPlay::ModeCleanup @0x0cb740 and
// CPlay::PresentAndFlush @0x0cba10), so the real owner is CPlay (<Gruntz/Play.h>).
//
// It chains the base activate, hides the cursor, registers the TILEZ/IMAGEZ(LEVEL)/
// IMAGEZ(GRUNTZ) namespaces through the m_c->m_imageRegistry registrar (vtable slot +0x4c),
// then runs the level-specific init chain (m_guts status bar, m_c sub-objects) and
// kicks the state timer. __thiscall; every callee is a reloc-masked external.
//
// (2026-07-14: the former PlayActivate/GLS* view nest - PlayActivate, GLSAssetRoot,
// GLSSubA, GLSSub14, GLSSub2c, GLSObj24, GLSNamespace, GLSMapMgr, plus the local
// CPlay/CState/CDDrawSurfaceMgr member that already existed under its real name
// (m_levelBank/m_gruntzBank/m_guts/m_region0Gate/m_region1Gate/m_stepCountdown; the
// asset root's m_4/m_c/m_24 are m_drawTarget/m_workerList/m_24 with their real
// classes). "Present" IS CDDrawWorkerList::PruneWorkers, slot 13.)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/GameRegPtr.h>
#include <Mfc.h>                          // ShowCursor (afx-first)
#include <EmptyString.h>                  // g_emptyString
#include <Bute/SymTab.h>                  // canonical CSymTab (ResolvePath @0x13bae0)

#include <Gruntz/Play.h>
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (the VisitVisible chain arg)       // the real CPlay : CState (method owner)
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CImageRegistry (+0x10 image registrar) + CDDrawSubMgrPages
#include <rva.h>
#include <Globals.h>                   // g_gameReg / shared globals
#include <Gruntz/GameLevel.h>          // canonical CGameLevel (VisitVisible)
#include <DinMgr2/DirectInputMgr2.h>   // canonical DirectInputMgr2 (ReadAll)
#include <DinMgr2/InputMgrPtr.h>       // g_inputMgr (DirectInputMgr2* view; the one decl)
#include <DDrawMgr/DDrawSubMgrPages.h> // canonical CDDrawSubMgrPages (Method_158e90)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (m_surface Fill target)
#include <DDrawMgr/DDrawWorkerList.h>  // renderer B (PruneWorkers - the "present")
#include <DDrawMgr/DDSurface.h>        // CDDSurface::Fill (0x13e760)
#include <Gruntz/StatusBarMgr.h>       // canonical CStatusBarMgr (m_guts Deactivate/Load...)

// 0xface0: the shared image-load activate gate = CState::InputVirtual (slot-8 base
// virtual, SYMBOL-bound in Attract.cpp), called qualified (direct) on `this` - the same
// spelling StateImages.cpp's CBootyState/CImageState use. 0xfa8f0 is CState::RetireScene
// (the shared state-timer arm, inherited by CPlay - called cast-free below).

// The global empty C string (0x6293f4).

// The +0xc4 reset manager is the DirectInputMgr2 input singleton g_inputMgr
// (DAT_00245570, decl in <DinMgr2/InputMgrPtr.h>): ReadAll (@0x133110) polls devices.
// The game-manager singleton (0x64556c). Declared here (it used to arrive from
// <Gruntz/Play.h>, whose header-level decl was removed so each TU can pick the view /
// real class it needs -- see the note in Play.h). Type unchanged for this TU.
// The camera auto-scroll/clamp update (MgrAutoScroll.cpp @0xebd70, cdecl 3-arg),
// called with (g_gameReg, this->m_guts, this->m_region0Gate).
class CGruntzMgr;
void UpdateMgrScroll(CGruntzMgr* pm, i32* pMode, i32 snapFlag); // reloc-masked

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
    if (m_c->m_imageRegistry->LoadNamespace(h, g_emptyString, "_") == -1) {
        return 0;
    }

    h = m_levelBank->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_c->m_imageRegistry->LoadNamespace(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = m_gruntzBank->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_c->m_imageRegistry->LoadNamespace(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_inputMgr->ReadAll();
    while (ShowCursor(FALSE) >= 0)
        ;

    m_c->m_drawTarget->m_backPair->m_surface->Fill(0);
    UpdateMgrScroll((CGruntzMgr*)g_gameReg, reinterpret_cast<i32*>(m_guts), m_region0Gate);

    if (m_region1Gate != 0) {
        NotifyVisibleEntities(); // CPlay @0xd9050
    } else {
        m_c->m_level->VisitVisible(static_cast<void*>(m_c->m_drawTarget->m_backPair), m_c->m_childGroup);
        m_c->m_workerList->PruneWorkers(
            m_c->m_drawTarget->m_backPair,
            m_c->m_drawTarget->m_overlayPair
        );
    }

    m_guts->Deactivate();
    m_guts->LoadMainStatusBarSprite();
    m_stepCountdown = 2;
    m_c->m_drawTarget->Method_158e90();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited by CPlay, cast-free)
    return 1;
}
