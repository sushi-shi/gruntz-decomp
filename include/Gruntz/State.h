// State.h - the WAP32 base game-state class (C:\Proj\Gruntz). One canonical
// definition, shared by GameMode.h (the leaf states CMenuState/CCreditsState/
// CBootyState + the gamemode CPlay::Update match) and CPlay.h (the in-game PLAY
// state whose Render drives the high vtable slots).
//
// Layout is the ctor ground truth (CState::CState at 0x08c750 zeroes a flat
// scalar list and seeds four fields to 0x40). The two reconstructions modeled
// the same owner/view sub-objects under different names: the +0x04 owner back-
// ptr is the game-manager singleton `CGruntzMgr *` (gamemode/cplay downcast it to
// their local CGMOwner/CWorld facet views); the +0x0c holder is `CDDrawSurfaceMgr *`
// (cplay dereferences it directly, gamemode casts to CGMView). Both are 4-byte
// pointer slots, so the casts are codegen-neutral.
//
// The virtual interface is ~41 slots. Most are out-of-line stubs that only
// anchor the vftable order so the meaningful slots land at the right offset:
// Update (slot 4 / +0x10), Render (slot 5 / +0x14), InputVirtual (slot 8 / +0x20,
// CCreditsState's per-frame poll), BeginFrameClear (slot 31 / +0x7c) and
// RenderSlow/RenderFast (slots 39/40, the CPlay frame-rate split). The vftables
// are not diffed, so the high slots' presence is free; the concrete states
// override the few they implement.
#ifndef GRUNTZ_GRUNTZ_CSTATE_H
#define GRUNTZ_GRUNTZ_CSTATE_H

#include <Ints.h>
#include <rva.h> // SIZE/VTBL vtable-catalog annotations
#include <Gruntz/GameModeBase.h>
#include <Gruntz/GameStateId.h> // Update()'s per-state id return type

class CDDrawSurfaceMgr; // +0x0c render/resource holder == CGameRegistry::m_world;
                        // defined in <Gruntz/GameRegistry.h> (its render sub-object
                        // facets CRenderer/CDrawSurface in <Gruntz/View.h>). Opaque here.
class CSymParser;       // +0x08 the level/rez symbol parser (<Bute/SymParser.h>) - the manager's
// +0x34 m_symParser, cached here by LoadGameAssetNamespaces. The ex-CBankMgr
// shell WAS this class: every consumer called ((CSymParser*)m_8)->ResolvePath
// (@0x13c030), and the loader stores mgr->m_symParser here outright.
class CDDSurface;        // +0x160/+0x164 the two 64x64 scratch blit surfaces (DDrawMgr)
struct CResSource;       // +0x28/+0x30/+0x34 resolved asset banks (LookupSet a named set)
class CSymTab;           // m_2c's symbol-table facet (ResolvePath/FindSub; <Bute/SymTab.h>)
class CMenuRoot;         // m_c's title/menu-root facet (the title-roll cluster's view)
class CAttractScreenObj; // m_2c's fade-screen-resolver facet (FadeInTitle's view)
class CGruntzMgr;        // +0x04 owner back-ptr: the game-manager singleton (*g_gameReg).
                         // Forward-declared (MFC-free) so this widely-included header stays
                         // afx-neutral; GruntzMgr.h/GameRegistry.h complete the two views.
class CFaderMgr;         // +0x10 fader manager (the CSoundFxEmitter facet's fader mgr;
                         // RetireScene's Add/Remove target). Opaque here.
struct FxResource;       // +0x0c viewed as the emitter resource chain (== m_c; the DDraw
                         // worker + gate RetireScene walks). Full shape in SoundFxEmitter.h.
class CString;           // MFC - BuildAssetNamespacePrefixes' key arg (reference-only here)

// The base game-state vtable (RTTI ??_7CState@@6B@ @0x005ea21c, 26 slots); the retail
// CState ctor @0x08c750 (reconstructed in GameMode.cpp) stamps it. Explicit VTBL()
// catalog entry.
SIZE_UNKNOWN(CState);
VTBL(CState, 0x001ea21c);
class CState {
public:
    CState();
    // dtor body INLINE so MSVC folds the vtable-restore + base cleanup into the
    // synthesized scalar-deleting dtor ??_G (matched) rather than emitting a ??1.
    virtual ~CState() {
        ((CGameModeBase*)this)->BaseCleanup();
    } // slot 0
    virtual i32 Vfunc1(i32, i32, i32); // slot 1 (asset/state load; leaf-overridden)
    virtual void ReleaseResources();   // slot 2  (+0x8)  resource teardown (leaf override)
    RVA(0x0008c490, 0x4)
    virtual i32 Vfunc3() {
        return m_ready;
    }
    RVA(0x0008c4b0, 0x6)
    virtual GameStateId Update() {
        return GAMESTATE_BASE;
    }
    RVA(0x0008c4d0, 0x6)
    virtual i32 Render() {
        return 1;
    }
    RVA(0x0008c4f0, 0x3)
    virtual i32 Vslot06() {
        return 0;
    } // slot 6  (+0x18)  activation-ready poll
    virtual i32 Vslot07();        // slot 7  (+0x1c)  lobby-host-ready poll
    virtual i32 InputVirtual();   // slot 8  (+0x20)  per-frame input poll
    virtual i32 Vslot09(i32);     // slot 9  (+0x24)  notify w/ state id
    virtual i32 FrameSlot28(i32); // slot 10 (+0x28)  per-frame poll (leaf override)
    // CGruntzMgr's per-state forwarders (0x8d9d0..0x8dbe0) dispatch a 2-arg or
    // 3-arg notification into these slots; the int return / arg shapes are what
    // those forwarders' push/ret-N codegen needs (vtables not diffed).
    RVA(0x0008c550, 0x5)
    virtual i32 Vslot0b(i32, i32) {
        return 0;
    } // slot 11 (+0x2c)
    RVA(0x0008c570, 0x5)
    virtual i32 Vslot0c(i32, i32) {
        return 0;
    } // slot 12 (+0x30)
    RVA(0x0008c590, 0x5)
    virtual i32 Vslot0d(i32, i32) {
        return 0;
    } // slot 13 (+0x34)
    RVA(0x0008c5b0, 0x5)
    virtual i32 Vslot0e(i32, i32, i32) {
        return 0;
    } // slot 14 (+0x38)
    RVA(0x0008c5d0, 0x5)
    virtual i32 Vslot0f(i32, i32, i32) {
        return 0;
    } // slot 15 (+0x3c)
    RVA(0x0008c5f0, 0x5)
    virtual i32 Vslot10(i32, i32, i32) {
        return 0;
    } // slot 16 (+0x40)
    RVA(0x0008c610, 0x5)
    virtual i32 Vslot11(i32, i32, i32) {
        return 0;
    }
    RVA(0x0008c630, 0x5)
    virtual i32 Vslot12(i32, i32, i32) {
        return 0;
    } // slot 18 (+0x48)
    RVA(0x0008c650, 0x5)
    virtual i32 Vslot13(i32, i32, i32) {
        return 0;
    } // slot 19 (+0x4c)
    RVA(0x0008c670, 0x5)
    virtual i32 Vslot14(i32, i32, i32) {
        return 0;
    } // slot 20 (+0x50)
    // slot 21 (+0x54): HandleCommand's 0x800e path polls it as a veto gate
    // (`if (m_curState->Vslot15()) return 1;`) - the i32 return is proven there.
    RVA(0x0008c690, 0x3)
    virtual i32 Vslot15() {
        return 0;
    }
    // slot 22 (+0x58): `33 c0 c3` (xor eax,eax; ret) proves an i32 return-0, not void.
    RVA(0x0008c6b0, 0x3)
    virtual i32 Vslot16() {
        return 0;
    }
    // slot 23 (+0x5c, 0x0fa6b0): the frame-surface GDI text overlay (all states inherit
    // it). GetDC on the frame surface, SetBkMode/SetTextColor, TextOutA(x,y,str), ReleaseDC.
    virtual i32 Vslot17(i32 x, i32 y, char* str, i32 color, i32 bkMode);
    virtual void Vslot18();
    virtual void Vslot19();

    // Non-virtual leaf (matched): seeds the begin-clear params.
    i32 SetBeginClearParams(i32 unused, i32 arg2, i32 arg3); // 0x8c970
    // Non-virtual exit notification (reloc-masked; called by ExitModalUI).
    void NotifyExit(i32 code);

    // ShadeScreen (0x0fa6b0's sibling @0x0faf50): the once-suppressed screen dim. On the
    // first call after the g_suppress latch is armed it consumes the latch (returns it);
    // otherwise it shades the draw surface (m_c->m_drawTarget->m_backPair->m_2c->ShadeRect(pct,0)).
    i32 ShadeScreen(i32 pct); // 0x0faf50 (reloc-masked; defined in StateDrawText.cpp)

    // The title-roll cluster (0xfa1f0/0xfa300/0xfa350). Each body reads ONLY CState
    // members (m_c/m_8/m_2c), and every sibling state (CHelpState/CSplashState/
    // CBootyState/CMultiBootyState/CAttract/...) calls them directly on its own `this`
    // -> they ARE CState-level helpers. Definitions live in Attract.cpp (the attract
    // unit owns the 0xfa1f0.. RVAs) as CState:: methods; the callers stay cast-free
    // (CAttract is a sibling of CState, not a base). Reloc-masked.
    i32 FadeInTitle(const char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa1f0
    i32 RunTitle(i32 a, i32 b, i32 c, i32 d, i32 e);                      // 0x0fa300
    i32 RunTitleSeq(const char* name, i32 a, i32 b, i32 c, i32 d);        // 0x0fa350
    // RetireScene (0xfa8f0): the two-channel screen-transition emitter every screen state
    // runs on its own `this` (xref: CBootyState/CMultiBootyState/CCreditsState/CAttract/
    // CPreviewState/CMulti/CPlay/... all call it). It reads the CState resource-chain facet
    // (fxRes()/m_faderMgr) only, so it IS a CState-level helper. Definition lives in
    // Attract.cpp (the
    // attract unit owns 0xfa8f0.. RVAs) as a CState:: method; reloc-masked.
    i32 RetireScene(i32 a1, i32 a2, i32 a3, i32 a4); // 0x0fa8f0
    // Present (0xfaec0): per-frame present/refresh of the bound view - shade the back
    // surface, flip the front. Same CState-level non-virtual shape as RetireScene, and the
    // xrefs prove the receiver: CGruntzMgr::RunModalDialog calls it as `mov ecx,[esi+0x2c];
    // call 0x1ec9` (CGruntzMgr+0x2c IS m_curState, a CState*), and CPlay::Vslot23 calls it on
    // its own `this`. Direct rel32 => non-virtual. Definition in Attract.cpp (the unit that
    // owns the 0xfa.. band).
    void Present(i32 arg0); // 0x0faec0
    // The emitter resource-chain view of the +0x0c holder (== m_c reinterpreted): its
    // +0x04 DDraw worker + +0x1c gate are what RetireScene walks. Inline -> the same
    // `mov reg,[this+0x0c]` as the direct member read (forward-declared facet).
    FxResource* fxRes() {
        return (FxResource*)m_c;
    }
    // The per-area GAME asset-namespace loader (0xf9ea0, GameAssetNamespaces.cpp,
    // entered via the 0x43a9 ILT thunk). Every leaf state (CSplashState/CHelpState/
    // CCreditsState/CBootyState/CMenuState/CAttract/CPreviewState/CMulti) calls it on
    // its own `this` and TESTs the int result -> it IS a CState-level non-virtual
    // Returns 1 on success, 0 on bail.
    i32 LoadGameAssetNamespaces(i32 mgr, i32 areaArg, i32 a3); // 0x0f9ea0
    // The title cluster's typed views of the shared CState slots (m_c is the menu
    // root, m_2c the fade screen-resolver when a title rolls). Inline -> the same
    // `mov reg,[this+off]` falls out; forward-declared facets (attract-scoped types).
    CMenuRoot* menuRoot() {
        return (CMenuRoot*)m_c;
    }
    CAttractScreenObj* screenObj() {
        return (CAttractScreenObj*)m_2c;
    }

    // Register/unregister a "GRUNTZ_<name>" asset namespace across the state's three
    // resource registries (m_c->m_imageRegistry/m_28/m_animRegistry) from the m_gruntzBank symbol
    // tree (0xdca70). Non-virtual __thiscall on the base state; every caller reaches it
    // on the active game-state (g_gameReg->m_curState, a CState* -> its concrete CPlay).
    // (Ex the CNamespaceLoader fake-view facet - RTTI proves CState is a root and CPlay's
    // only base is CState, so that "class" was this method wearing a view owner.)
    i32 BuildAssetNamespacePrefixes(const CString& name, i32 mode, i32 lightGate, i32 finishGate);

    // --- scalar members, at the offsets CState::CState pins ---
    // +0x04  owner back-ptr == the game-manager singleton (*g_gameReg). PROVEN one
    // object: m_4->m_c is the SAME field as g_gameReg->m_c (the active-selection gate),
    // m_4->m_48 the same sound bank, m_4->m_8c..m_98 the same live video mode. The
    // gamemode/cplay TUs still downcast it to their local CGMOwner/CWorld facet views.
    CGruntzMgr* m_4;
    // +0x08  the level/rez symbol parser (ResolvePath @0x13c030; == mgr->m_symParser,
    // cached by LoadGameAssetNamespaces). Ex `CBankMgr*` - a shell type every consumer
    // bridged with a ((CSymParser*)m_8) cast; typed for real, the casts are gone.
    CSymParser* m_8;
    // +0x0c  render/resource context. VERIFIED (matcher-2, sema): the SAME object as
    // CGameRegistry::m_world (+0x30) == the canonical CDDrawSurfaceMgr - non-polymorphic;
    // its +0x04 sub-object is the DDraw worker manager (CDDrawSubMgrPages::Method_158ee0
    // @0x158ee0) and its +0x10 registrar is CImageRegistry (Install/LoadTree +0x48,
    // LoadNamespace +0x4c). The state activators (CBootyState/CMultiBootyState/CImageState
    // slot-8 loaders) reach it through this one holder; their old per-TU StateMgr/BootyAssetRoot
    // shadows are folded away. (The former `CView`/`CDDrawSurfaceMgr` render view is folded onto
    // CDDrawSurfaceMgr; its render sub-object facets live in <Gruntz/View.h>.)
    CDDrawSurfaceMgr* m_c; // +0x0c
    CFaderMgr* m_faderMgr; // +0x10  fader mgr (RetireScene's Add/Remove target; the
                           //         loader caches mgr->m_40 here - the +0x40 slot's
                           //         CFaderMgr-vs-CTriggerMgr identity conflict is open)
    i32 m_14;              // +0x14
    i32 m_18;              // +0x18
    i32 m_levelIndex;      // +0x1c  play-state level index 1..0x28 (CGruntzMgr::GoToNext/PrevLevel)
    i32 m_levelType;       // +0x20  level terrain-class id; CProjectile::LoadProjectileEffects
                           //         switches on it (4/5/8 land-death, 6 no-death) to pick the
                           //         level death effect
    i32 m_24;              // +0x24
    // +0x28  level asset bank; a Bute CSymTab (LookupSet == CSymTab::ResolvePath
    // 0x13bae0), so every user reaches it as CSymTab* -> typed here (kills the casts).
    // LoadGameAssetNamespaces stores the resolved "AREA%i" node here.
    CSymTab* m_levelBank; // +0x28  level asset bank (TILEZ/IMAGEZ/SOUNDZ/MIDIZ source)
    // +0x2c  the resolved asset source a state loader caches (CBankMgr::Lookup
    // result): CSplashState/CHelpState store the "STATEZ_*" namespace here and
    // (splash) LoadGroup its "SOUNDZ" set; the attract path stashes its resolved
    // TITLE state here. A 4-byte pointer slot.
    CResSource* m_2c; // +0x2c
    // The cached asset source (m_2c) is a Bute CSymTab; one typed accessor for that
    // facet so the state loaders drop the (CSymTab*)m_2c casts. <Bute/SymTab.h>.
    CSymTab* SymTab2c() {
        return (CSymTab*)m_2c;
    }
    CSymTab* m_gruntzBank; // +0x30  GRUNTZ asset bank (CSymTab; LoadImageBanks caches here)
    CSymTab* m_gameBank;   // +0x34  GAME asset bank (CSymTab; GAME-namespace loaders' source)
    i32 m_38;              // +0x38
    i32 m_ready;           // +0x3c  active/ready gate (Vfunc3 returns it)
    i32 m_40;              // +0x40  notify latch (HandleCommand 0x8006 sets 1 before the
                           //         menu transition)
    i32 m_44;              // +0x44  (LoadGameAssetNamespaces seeds -1; role unrecovered)
    i32 m_48;              // +0x48  (LoadGameAssetNamespaces seeds -1; role unrecovered)
    // +0x4c..+0x14b  the version-string buffer LoadGameAssetNamespaces sprintf's
    // ("Alpha Version, Build %i, ..."); the state save serializers stream it raw as a
    // 0x100 block (the MgrPersist view's m_4c[0x100]).
    char m_versionString[0x100];
    i32 m_14c;         // +0x14c  (LoadGameAssetNamespaces clears; role unrecovered)
    i32 m_cursorX;     // +0x150 live cursor X (ResetForMode GetCursorPos); BeginFrameClear arg
    i32 m_cursorY;     // +0x154 live cursor Y
    i32 m_snapOriginX; // +0x158 drag/select snap origin X
    i32 m_snapOriginY; // +0x15c drag/select snap origin Y
    // +0x160/+0x164: the two 64x64 scratch blit surfaces (LoadGameAssetNamespaces
    // creates them via m_c->m_ptrColl->MakeAndAddB(0x40,0x40,...); BaseCleanup
    // RemoveItemA's them back to the pool; StepInputA BltFast's the selected half).
    // The old "axis value" reading was wrong. +0x168..+0x1a4 is the per-half
    // src-RECT/edge block StepInputA feeds (four extents seeded 0x40 by the ctor
    // - the 64x64 rect dims).
    CDDSurface* m_160; // +0x160 first-half scratch surface
    CDDSurface* m_164; // +0x164 second-half scratch surface
    i32 m_168;         // +0x168 first-half block (addr taken)
    i32 m_16c;
    i32 m_170; // +0x170 (= 0x40)
    i32 m_174; // +0x174 (= 0x40)
    i32 m_178; // +0x178 second-half block (addr taken)
    i32 m_17c;
    i32 m_180; // +0x180 (= 0x40)
    i32 m_184; // +0x184 (= 0x40)
    i32 m_188; // +0x188 first-half {x,y} edge feed
    i32 m_18c; // +0x18c
    i32 m_190;
    i32 m_194;
    i32 m_198; // +0x198 second-half {x,y} edge feed
    i32 m_19c;
    i32 m_1a0;
    i32 m_1a4;

    // BuildWarpStoneGlitterAnimation (0x19540) is a CMultiBootyState method (GameMode.h).
    //
    // LoadGruntEffectSprites (0x1a040), LevelMsgHudDriver (0x1a700) and FormatHudText
    // (0x1af70) are CBootyState methods (proof on CBootyState in <Gruntz/GameMode.h>).
    // None of them could ever have lived
    // here: they touch [this+0x1d0], [this+0x264], [this+0x2c4] and write m_icons out to
    // [this+0x31c], while CState is the base of the allocation-proven 0x1c0 CMenuState
    // and is therefore <= 0x1c0. Their `this` comes from CBootyState's own vtable slot 1
    // (0x18830, data-referenced at
    // ??_7CBootyState@@6B@+0x4), which calls it with `mov ecx,esi`.
    //
    // BuildBootyWalkingGruntz (0x1b450) is a BzState method (BootyWalkAnim.cpp).
};

#endif // GRUNTZ_GRUNTZ_CSTATE_H
