#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Mfc.h> // GameMode.h needs the afx umbrella (WINAPI/windows.h come with it)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>

#include <rva.h>
#include <Gruntz/BankMgr.h>  // CResSource::LookupSet (the state's +0x2c/+0x30 asset source)
#include <Gruntz/GameMode.h> // canonical CBootyState : CState + the shared CDDrawSurfaceMgr facet
#include <Gruntz/ImageState.h> // canonical CImageState (CState leaf, MENU image loader)

RVA(0x000a09a0, 0x6a)
i32 CImageState::LoadStateImages() {
    if (CState::InputVirtual() == 0) {
        return 0;
    }
    void* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (tree == 0) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(tree, "MENU", "_") == -1) {
        return 0;
    }
    if (Vslot06() == 0) { // the per-state image hook (slot 6, +0x18)
        return 0;
    }
    int(WINAPI * sc)(BOOL) = ::ShowCursor;
    i32 r = sc(1);
    while (r < 0) {
        r = sc(1);
    }
    return 1;
}

RVA(0x0001c8a0, 0xec)
i32 CBootyState::InputVirtual() {
    if (CState::InputVirtual() == 0) {
        return 0;
    }
    int(WINAPI * sc)(BOOL) = ::ShowCursor;
    i32 r = sc(0);
    while (r >= 0) {
        r = sc(0);
    }
    void* booty = SymTab2c()->ResolvePath("IMAGEZ");
    if (booty == 0) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(booty, "BOOTY", "_") == -1) {
        return 0;
    }
    void* gruntz = m_gruntzBank->ResolvePath("IMAGEZ");
    if (gruntz == 0) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(gruntz, "GRUNTZ", "_") == -1) {
        return 0;
    }
    if (m_activation != 200) {
        if (FadeInTitle("bg", 0, 0, 0, 0, 1) == 0) {
            return 0;
        }
        ShowLevelCompleteMessage();
    } else {
        ShowSecretBonusMessage();
    }
    m_world->m_drawTarget->TransExit();
    RetireScene(
        0x50,
        0x3e8,
        0,
        1
    ); // 0xfa8f0 CState::RetireScene (inherited; was fake CBootyState::BuildPage)
    return 1;
}
