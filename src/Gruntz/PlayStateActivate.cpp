// PlayStateActivate.cpp - CPlay::OnActivate (0x0cb800), the PLAY level-state
// activation (vtable slot 8; reached directly by CTriggerMgr::NotifyCell). Re-homed
// from the artifact "GameLevelState" placeholder of BacklogStateLoaders.cpp: RVA
// 0x0cb800 sits inside CPlay's method band (between CPlay::ModeCleanup @0x0cb740 and
// CPlay::PresentAndFlush @0x0cba10), so the real owner is CPlay (<Gruntz/Play.h>).
//
// It chains the base activate, hides the cursor, registers the TILEZ/IMAGEZ(LEVEL)/
// IMAGEZ(GRUNTZ) namespaces through the m_c->m_10 registrar (vtable slot +0x4c),
// then runs the level-specific init chain (m_2dc map mgr, m_c sub-objects) and kicks
// the state timer. __thiscall; every callee is a reloc-masked external.
//
// Following CPlay's documented loader idiom (<Gruntz/Play.h>): the routine casts
// `this` once to a typed activation facet (the +0xc resource root, +0x28/+0x30
// bank sources, +0x2dc guts, the region gates), keeping the sub-object views local.
#include <Mfc.h>         // ShowCursor (afx-first)
#include <Bute/SymTab.h> // canonical CSymTab (ResolvePath @0x13bae0)

#include <Gruntz/Play.h>       // the real CPlay : CState (method owner)
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <Gruntz/ResMgr.h>     // canonical CImageRegistry (+0x10 image registrar)
#include <Gruntz/View.h>       // canonical CSpriteFactoryHolder sub-objects (CRenderer @+0xc)
#include <rva.h>
#include <Globals.h>                   // g_gameReg / shared globals
#include <Gruntz/GameLevel.h>          // canonical CGameLevel (VisitVisible)
#include <DinMgr2/DirectInputMgr2.h>   // canonical DirectInputMgr2 (ReadAll)
#include <DDrawMgr/DDrawSubMgrPages.h> // canonical CDDrawSubMgrPages (Method_158e90/159ef0)
class CSBI_RectOnly {
public:
    i32 LoadMainStatusBarSprite();
    i32 Deactivate();
};
class CDDSurface {
public:
    i32 Fill(unsigned int c);
}; // 0x13e760
// 0xface0: the shared image-load activate gate (CState base method; the recovered
// symbol is CMgrPersistObj::Init, Attract.cpp). 0xfa8f0 is CState::RetireScene (the
// shared state-timer arm, inherited by CPlay - called cast-free below).
class CMgrPersistObj {
public:
    i32 Init();
};

// The global empty C string (0x6293f4).
extern "C" char g_emptyString[];

// The +0x10 image registrar is the canonical CImageRegistry (ResMgr.h): the
// namespace-register op here is its LoadNamespace slot-19 (+0x4c) virtual. Uses the
// real class - no local registrar view.
struct GLSNamespace { // m_28 / m_30
    // Lookup @0x13bae0 IS CSymTab::ResolvePath; cast at each call.
};
struct GLSSub2c {
    // Step @0x13e760 IS CDDSurface::Fill; cast at the call.
};
struct GLSSub14 {
    char m_pad00[0x2c];
    GLSSub2c* m_2c; // +0x2c
};
struct GLSSubA { // m_c->m_4
    // Begin @0x158e90 IS CDDrawSubMgrPages::Method_158e90; cast at the call.
    char m_pad00[0x14];
    GLSSub14* m_14; // +0x14
    void* m_18;     // +0x18
};
struct GLSObj24 { // m_c->m_24
    // Wire @0x15dc90 IS CGameLevel::VisitVisible; cast at the call.
};
// The asset root (CPlay+0xc) is the canonical CSpriteFactoryHolder (Play.h): m_c is CSpriteFactoryHolder's
// renderer-B (CSpriteFactoryHolder+0xc, the real class CRenderer, View.h), and the dispatched
// slot 13 (+0x34) is CRenderer::Present. Uses the real class - no local view.
struct GLSAssetRoot { // this->m_c (== CSpriteFactoryHolder, View.h)
    char m_pad00[0x4];
    GLSSubA* m_4;         // +0x04  CSpriteFactoryHolder render-state (CDDrawSubMgrPages family)
    void* m_8;            // +0x08  renderer A
    CRenderer* m_c;       // +0x0c  renderer B (CRenderer, Present slot 13)
    CImageRegistry* m_10; // +0x10  the image/tile registrar
    char m_pad14[0x24 - 0x14];
    GLSObj24* m_24; // +0x24
};
struct GLSMapMgr { // this->m_2dc
    // Finalize @0x125d IS CSBI_RectOnly::Deactivate; cast at the call.
    // Activate2 @0x21b7 IS CSBI_RectOnly::LoadMainStatusBarSprite; cast at the call.
};
// The +0xc4 reset manager is the DirectInputMgr2 input singleton g_645570
// (DAT_00245570, bound extern "C" in GruntzMgr.cpp): ReadAll (@0x133110) polls devices.
extern "C" DirectInputMgr2* g_645570;
// The game-manager singleton (0x64556c); mangled ?g_gameReg@@3PAUWwdGameReg@@A.
DATA(0x0024556c)
// The camera auto-scroll/clamp update (MgrAutoScroll.cpp @0xebd70, cdecl 3-arg),
// called with (g_gameReg, this->m_2dc, this->m_470).
class CGruntzMgr;
void UpdateMgrScroll(CGruntzMgr* pm, i32* pMode, i32 snapFlag); // reloc-masked

// The CPlay activation facet: `this` cast once, so the field accesses take CPlay's
// activation offsets without disturbing Play.cpp's Render-side member typing.
struct PlayActivate {
    char m_pad00[0xc];
    GLSAssetRoot* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    GLSNamespace* m_28; // +0x28
    char m_pad2c[0x30 - 0x2c];
    GLSNamespace* m_30; // +0x30
    char m_pad34[0x2dc - 0x34];
    GLSMapMgr* m_2dc; // +0x2dc
    char m_pad2e0[0x470 - 0x2e0];
    i32 m_470; // +0x470
    i32 m_474; // +0x474
    char m_pad478[0x510 - 0x478];
    i32 m_510; // +0x510
};

RVA(0x000cb800, 0x191)
i32 CPlay::OnActivate() {
    PlayActivate* p = (PlayActivate*)this;
    if (!((CMgrPersistObj*)this)->Init()) { // shared base activate gate @0xface0
        return 0;
    }
    while (ShowCursor(FALSE) >= 0)
        ;

    void* h = ((CSymTab*)p->m_28)->ResolvePath("TILEZ");
    if (!h) {
        return 0;
    }
    if (p->m_c->m_10->LoadNamespace(h, g_emptyString, "_") == -1) {
        return 0;
    }

    h = ((CSymTab*)p->m_28)->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (p->m_c->m_10->LoadNamespace(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = ((CSymTab*)p->m_30)->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (p->m_c->m_10->LoadNamespace(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_645570->ReadAll();
    while (ShowCursor(FALSE) >= 0)
        ;

    ((CDDSurface*)p->m_c->m_4->m_14->m_2c)->Fill(0);
    UpdateMgrScroll((CGruntzMgr*)g_gameReg, (i32*)p->m_2dc, p->m_470);

    if (p->m_474 != 0) {
        NotifyVisibleEntities(); // CPlay @0xd9050
    } else {
        ((CGameLevel*)p->m_c->m_24)
            ->VisitVisible((void*)p->m_c->m_4->m_14, (CGameObjChain*)p->m_c->m_8);
        p->m_c->m_c->Present(p->m_c->m_4->m_14, p->m_c->m_4->m_18);
    }

    ((CSBI_RectOnly*)p->m_2dc)->Deactivate();
    ((CSBI_RectOnly*)p->m_2dc)->LoadMainStatusBarSprite();
    p->m_510 = 2;
    ((CDDrawSubMgrPages*)p->m_c->m_4)->Method_158e90();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited by CPlay, cast-free)
    return 1;
}

SIZE_UNKNOWN(GLSAssetRoot);
SIZE_UNKNOWN(GLSMapMgr);
SIZE_UNKNOWN(GLSNamespace);
SIZE_UNKNOWN(GLSObj24);
SIZE_UNKNOWN(GLSSub14);
SIZE_UNKNOWN(GLSSub2c);
SIZE_UNKNOWN(GLSSubA);
SIZE_UNKNOWN(PlayActivate);
