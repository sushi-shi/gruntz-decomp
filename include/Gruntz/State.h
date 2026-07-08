// State.h - the WAP32 base game-state class (C:\Proj\Gruntz). One canonical
// definition, shared by GameMode.h (the leaf states CMenuState/CCreditsState/
// CBootyState + the gamemode CPlay::Update match) and CPlay.h (the in-game PLAY
// state whose Render drives the high vtable slots).
//
// Layout is the ctor ground truth (CState::CState at 0x08c750 zeroes a flat
// scalar list and seeds four fields to 0x40). The two reconstructions modeled
// the same owner/view sub-objects under different names: the +0x04 owner back-
// ptr is the game-manager singleton `CGruntzMgr *` (gamemode/cplay downcast it to
// their local CGMOwner/CWorld facet views); the +0x0c holder is `CSpriteFactoryHolder *`
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

struct CSpriteFactoryHolder; // +0x0c render/resource holder == CGameRegistry::m_world;
                             // defined in <Gruntz/GameRegistry.h> (its render sub-object
                             // facets CRenderer/CDrawSurface in <Gruntz/View.h>). Opaque here.
struct CBankMgr;             // +0x08 asset-bank manager (resolves a "GRUNTZ"/"GAME"/level bank)
struct CResSource;           // +0x28/+0x30/+0x34 resolved asset banks (LookupSet a named set)
class CGruntzMgr;            // +0x04 owner back-ptr: the game-manager singleton (*g_64556c).
                             // Forward-declared (MFC-free) so this widely-included header stays
                             // afx-neutral; GruntzMgr.h/GameRegistry.h complete the two views.

// The base game-state vtable (RTTI ??_7CState@@6B@ @0x005ea21c, 26 slots); the retail
// CState ctor @0x08c750 (reconstructed in GameMode.cpp) stamps it. Explicit VTBL()
// catalog entry (was named only via the RTTI-derived vtable_names.csv).
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
    virtual i32 Vfunc3() { return m_ready; }
    RVA(0x0008c4b0, 0x6)
    virtual GameStateId Update() { return GAMESTATE_BASE; }
    RVA(0x0008c4d0, 0x6)
    virtual i32 Render() { return 1; }
    virtual i32 Vslot06();             // slot 6  (+0x18)  activation-ready poll
    virtual i32 Vslot07();             // slot 7  (+0x1c)  lobby-host-ready poll
    virtual i32 InputVirtual();        // slot 8  (+0x20)  per-frame input poll
    virtual i32 Vslot09(i32);          // slot 9  (+0x24)  notify w/ state id
    virtual i32 FrameSlot28(i32);      // slot 10 (+0x28)  per-frame poll (leaf override)
    // CGruntzMgr's per-state forwarders (0x8d9d0..0x8dbe0) dispatch a 2-arg or
    // 3-arg notification into these slots; the int return / arg shapes are what
    // those forwarders' push/ret-N codegen needs (vtables not diffed).
    virtual i32 Vslot0b(i32, i32);      // slot 11 (+0x2c)
    virtual i32 Vslot0c(i32, i32);      // slot 12 (+0x30)
    virtual i32 Vslot0d(i32, i32);      // slot 13 (+0x34)
    virtual i32 Vslot0e(i32, i32, i32); // slot 14 (+0x38)
    virtual i32 Vslot0f(i32, i32, i32); // slot 15 (+0x3c)
    virtual i32 Vslot10(i32, i32, i32); // slot 16 (+0x40)
    RVA(0x0008c610, 0x5)
    virtual i32 Vslot11(i32, i32, i32) { return 0; }
    virtual i32 Vslot12(i32, i32, i32); // slot 18 (+0x48)
    virtual i32 Vslot13(i32, i32, i32); // slot 19 (+0x4c)
    virtual i32 Vslot14(i32, i32, i32); // slot 20 (+0x50)
    // slot 21 (+0x54): HandleCommand's 0x800e path polls it as a veto gate
    // (`if (m_curState->Vslot15()) return 1;`) - the i32 return is proven there.
    virtual i32 Vslot15();
    virtual void Vslot16();
    virtual void Vslot17();
    virtual void Vslot18();
    virtual void Vslot19();

    // Non-virtual leaf (matched): seeds the begin-clear params.
    RVA(0x0008c970, 0x1c)
    i32 SetBeginClearParams(i32 unused, i32 arg2, i32 arg3) {
        m_cursorX = arg2;
        m_cursorY = arg3;
        return 1;
    }
    // Non-virtual exit notification (reloc-masked; called by ExitModalUI).
    void NotifyExit(i32 code);

    // --- scalar members, at the offsets CState::CState pins ---
    // +0x04  owner back-ptr == the game-manager singleton (*g_64556c). PROVEN one
    // object: m_4->m_c is the SAME field as g_64556c->m_c (the active-selection gate),
    // m_4->m_48 the same sound bank, m_4->m_8c..m_98 the same live video mode. The
    // gamemode/cplay TUs still downcast it to their local CGMOwner/CWorld facet views.
    CGruntzMgr* m_4;
    CBankMgr* m_8; // +0x08  asset-bank manager (CPlay loaders: Lookup GRUNTZ/GAME banks)
    // +0x0c  render/resource context. VERIFIED (matcher-2, sema): the SAME object as
    // CGameRegistry::m_world (+0x30) == the canonical CSpriteFactoryHolder - non-polymorphic;
    // its +0x04 sub-object is the DDraw worker manager (CDDrawSubMgrPages::Method_158ee0
    // @0x158ee0) and its +0x10 registrar is CImageRegistry (Install/LoadTree +0x48,
    // LoadNamespace +0x4c). The state activators (CBootyState/CMultiBootyState/CImageState
    // slot-8 loaders) reach it through this one holder; their old per-TU StateMgr/BootyAssetRoot
    // shadows are folded away. (The former `CView`/`CSpriteFactoryHolder` render view is folded onto
    // CSpriteFactoryHolder; its render sub-object facets live in <Gruntz/View.h>.)
    CSpriteFactoryHolder* m_c; // +0x0c
    char m_pad10[0x14 - 0x10];
    i32 m_14;         // +0x14
    i32 m_18;         // +0x18
    i32 m_levelIndex; // +0x1c  play-state level index 1..0x28 (CGruntzMgr::GoToNext/PrevLevel)
    i32 m_levelType;  // +0x20  level terrain-class id; CProjectile::LoadProjectileEffects
                      //         switches on it (4/5/8 land-death, 6 no-death) to pick the
                      //         level death effect
    i32 m_24;         // +0x24
    CResSource* m_levelBank; // +0x28  level asset bank (TILEZ/IMAGEZ/SOUNDZ/MIDIZ source)
    // +0x2c  the resolved asset source a state loader caches (CBankMgr::Lookup
    // result): CSplashState/CHelpState store the "STATEZ_*" namespace here and
    // (splash) LoadGroup its "SOUNDZ" set; the attract path stashes its resolved
    // TITLE state here. A 4-byte pointer slot (was modeled i32 + per-site casts).
    CResSource* m_2c;         // +0x2c
    CResSource* m_gruntzBank; // +0x30  GRUNTZ asset bank (LoadImageBanks caches here)
    CResSource* m_gameBank;   // +0x34  GAME asset bank (GAME-namespace loaders' source)
    i32 m_38;                 // +0x38
    i32 m_ready;              // +0x3c  active/ready gate (Vfunc3 returns it)
    i32 m_40;                 // +0x40  notify latch (HandleCommand 0x8006 sets 1 before the
                              //         menu transition)
    char m_pad44[0x4c - 0x44];
    char m_4c; // +0x4c (byte)
    char m_pad4d[0x150 - 0x4d];
    i32 m_cursorX;     // +0x150 live cursor X (ResetForMode GetCursorPos); BeginFrameClear arg
    i32 m_cursorY;     // +0x154 live cursor Y
    i32 m_snapOriginX; // +0x158 drag/select snap origin X
    i32 m_snapOriginY; // +0x15c drag/select snap origin Y
    // +0x160..+0x1a4: the per-axis scroll/input state block StepInputA walks
    // (two mirrored halves; four extents seeded to 0x40 by the ctor).
    i32 m_160; // +0x160 first-half axis value
    i32 m_164; // +0x164 second-half axis value
    i32 m_168; // +0x168 first-half block (addr taken)
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

    // BuildWarpStoneGlitterAnimation (0x19540) re-homed to its real owner
    // CMultiBootyState (GameMode.h); LoadGruntEffectSprites (0x1a040) is a
    // CPlay-layout method the trace mis-homed on the base (kept CState-homed).
    i32 LoadGruntEffectSprites();
    // BuildBootyWalkingGruntz (0x1b450) re-homed to BzState (BootyWalkAnim.cpp).
};

#endif // GRUNTZ_GRUNTZ_CSTATE_H
