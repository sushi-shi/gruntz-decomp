// StateImages.cpp - two state-family vfunc-8 image-namespace loaders, folded onto the
// canonical CState/CDDrawSurfaceMgr facets (<Gruntz/State.h> + <Gruntz/View.h>):
//   - CImageState::LoadStateImages (0xa09a0): a CState-derived "MENU" image loader.
//   - CBootyState::InputVirtual   (0x1c8a0): the REAL CBootyState (vtbl 0x5e9cec) slot 8
//     (GameMode.h declares this slot @0x01c8a0 declared-only; homed here) - the booty
//     "bg"/secret-bonus activation loader; NOT its sibling CMultiBootyState (0x5e9bdc).
//
// Both resolve an "IMAGEZ" tree off the state's asset source (CState::m_2c/m_gruntzBank,
// CResSource::LookupSet == 0x13bae0) and register it through the CDDrawSurfaceMgr image registry
// (m_c->m_imageRegistry->LoadNamespace, vtable slot +0x4c). The old per-TU StateMgr /
// WorkerReg / CSymTab shadows of the +0x0c/+0x2c facets are folded away. Field names are
// placeholders; only offsets + code bytes are load-bearing.
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Mfc.h> // GameMode.h needs the afx umbrella (WINAPI/windows.h come with it)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>

#include <rva.h>
#include <Gruntz/BankMgr.h>  // CResSource::LookupSet (the state's +0x2c/+0x30 asset source)
#include <Gruntz/GameMode.h> // canonical CBootyState : CState + the shared CDDrawSurfaceMgr facet
#include <Gruntz/ImageState.h> // canonical CImageState (CState leaf, MENU image loader)

// 0xface0 is CState's slot-8 base virtual (the shared image-load/activate gate every
// state override calls first via CState::InputVirtual(); def SYMBOL-bound in Attract.cpp).

// The cursor-show counter, cached in a game-owned function pointer (ff 15).
// reloc-fidelity: RVA 0x2c44c4 (was the VA 0x6c44c4 = 0x400000+RVA typo, which
// keep-last-wins poisoned the shared ?::ShowCursor binding for `play`/apphelpers).

// ---------------------------------------------------------------------------
// CImageState - a CState-derived front-end state whose slot-8 loader installs the "MENU"
// image namespace. Its concrete RTTI name is unrecovered (the 0xa09a0 body is reached
// non-virtually via an ILT thunk), so it is modeled as a minimal CState subclass: the
// +0x0c view (m_c) and +0x2c source (m_2c) are the inherited CDDrawSurfaceMgr/CResSource facets, and
// the per-state image hook is CState's slot 6 (Vslot06). (Canonical def: <Gruntz/ImageState.h>.)

RVA(0x000a09a0, 0x6a)
i32 CImageState::LoadStateImages() {
    if (CState::InputVirtual() == 0) {
        return 0;
    }
    void* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (tree == 0) {
        return 0;
    }
    if (m_c->m_imageRegistry->LoadNamespace(tree, "MENU", "_") == -1) {
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

// ---------------------------------------------------------------------------
// CBootyState::InputVirtual (slot 8, 0x1c8a0): hide the cursor, load the BOOTY + GRUNTZ
// image namespaces, then either pop the secret-bonus toast (when m_activation == 200) or
// fade the "bg" title and show the level-complete toast, kick the render worker apply
// (m_c->m_drawTarget->Flush) and build the booty page.
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
    if (m_c->m_imageRegistry->LoadNamespace(booty, "BOOTY", "_") == -1) {
        return 0;
    }
    void* gruntz = m_gruntzBank->ResolvePath("IMAGEZ");
    if (gruntz == 0) {
        return 0;
    }
    if (m_c->m_imageRegistry->LoadNamespace(gruntz, "GRUNTZ", "_") == -1) {
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
    m_c->m_drawTarget->Method_158ee0();
    RetireScene(
        0x50,
        0x3e8,
        0,
        1
    ); // 0xfa8f0 CState::RetireScene (inherited; was fake CBootyState::BuildPage)
    return 1;
}
