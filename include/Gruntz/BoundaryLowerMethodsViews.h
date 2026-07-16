// BoundaryLowerMethodsViews.h - shared referent/owner views for the lower-half
// engine_boundary leaf methods reconstructed in BoundaryLowerMethods.cpp.
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only the OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). The serialize/archive object folds to the canonical CSerialArchive
// (Read @ +0x2c / Write @ +0x30).
// BLOCKED - vtable-attributed but the home needs a base-model/conflict fix a follow-up
// must do (each stays @early-stop in place):
//   C112bf0  IS CCheckpointTriggerSwitchLogic::M (vtbl slot 3, thunk 0x36fc); home
//                needs the grid-access chain (g_reg->m_world->m_level->m_5c ==
//                holder->CGameLevel->m_mainPlane, C77dc0's grid) modeled on the
//                CheckpointSwitchBuild g_statzGameReg view + 0x24556c dual-view fold.
//   C104dd0  this == an unmodeled +0x2c status-bar-sprite holder (CSBI_RectOnly's m_2c;
//                SetState calls `mov ecx,[ecx+0x2c]; call 0x3f8a`) - owner class unrecovered.
//
// ORPHAN - caller known but `this`'s CLASS genuinely unrecovered (a sub-object reached
// by an offset/return, no RTTI, no named owner); cannot home as a method of a class:
//   C10bbe0  this == [[0x24556c+0x2c]+0x2dc] - a game-registry sub-object getter
//                (LoadPickupSprites: mov eax,ds:0x64556c; mov ebp,[eax+0x2c];
//                mov ecx,[ebp+0x2dc]). NOT CGrunt (the +0x528 table lives on reg->m_2c's
//                +0x2dc member, identity unrecovered).
//   C213a0   <- CChatBoxOwner::ProcessCheatInput (virtual-base field getter; owner is
//                the class holding that vbase - unmodeled).
//   CTypeColl464 RESOLVED: it was the shared CActReg archetype (this=g_actionTable
//                @0x644610 / g_reg_644af0@0x644af0); Resolve @0x464e0 is the standalone
//                copy of CActReg::ResolveEntry (now in <Gruntz/ActReg.h>/FortressFlag.cpp).
//   C9cab0   registry Lookup (m_10 sub's Get @0x1b8008 = CMapStringToOb::Lookup - the
//                0x1b8438 one is CMapStringToPtr; mfc_class. This pair was recorded
//                INVERTED here, which is what made the registry look unidentifiable).
//   Cbd450   no direct caller (c:\gruntz.log opener init; owner unrecovered).
//   Ccef50   <- Gap_0b63f0/Gap_0c8b80 (~CLobbySlot/CPlay teardown; owner unrecovered).
//   Cdb200   <- CNetMgr::DispatchRecvMsg but this==eax (a returned net sub-object, NOT
//                CNetMgr); the returned holder's class is unrecovered.
//   Cdb2f0   no direct caller (custom-level map-file object; owner unrecovered).
//   Cdb750   <- CPlayLevelLoad::LoadByMode (this==CPlayLevelLoad, mov ecx,esi) BUT
//                CPlayLevelLoad is a 3-way-CONFLATED placeholder (LoadLevelByMode /
//                BridgeMoveSprites / PyramidBridgeSprites views disagree on +0xc's
//                type) - the conflation must be split before a clean home.
//   Cea170   <- CStatusBarMgr::LoadTabSprites but this==ebp (a sub-object, NOT
//                CStatusBarMgr); the +0x38-virtual dispatch object's class is unrecovered.
//   C1181d0 / C118260  no direct caller (bounds-grow updaters; orphans).
#ifndef GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H
#define GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H

#include <Ints.h>
#include <Wap32/Object.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>     // canonical Read@+0x2c / Write@+0x30 archive stream
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)

// 0x0213a0 - virtual-base field getter (reads +0x04 of the virtual base whose
// displacement lives in the vbtable's second slot).
struct C213a0 {
    i32 Get();
};
SIZE_UNKNOWN(C213a0);

// 0x0bd450 - init: run the base ctor (0x3625) then open the "c:\gruntz.log" log.
struct Cbd450 {
    void Base3625();                 // 0x3625
    void OpenLog1983(const char* s); // 0x1983
    void Init();
};
SIZE_UNKNOWN(Cbd450);

// 0x0cef50 - teardown of the +0x04 owner + inner close chain.
struct CSubC8 {
    // M1b9c69 @0x1b9c69 IS CPtrList::~CPtrList; cast at the call.
};
SIZE_UNKNOWN(CSubC8);
struct CObjC {};
SIZE_UNKNOWN(CObjC);
struct CDDrawWorkerMgr {};
SIZE_UNKNOWN(CDDrawWorkerMgr);
struct CMidC {
    char pad0[4];
    CDDrawWorkerMgr* m_4; // +0x04
};
SIZE_UNKNOWN(CMidC);
struct Ccef50 {
    char pad0[4];
    char* m_4; // +0x04
    char pad8[0xc - 8];
    CMidC* m_c; // +0x0c
    char pad10[0x1c0 - 0x10];
    i32 m_1c0; // +0x1c0
    i32 M();
};
SIZE_UNKNOWN(Ccef50);

// 0x0db200 - swap the +0x08 holder to `arg` (validate + toggle old/new).
struct Cdb200 {
    char pad0[8];
    void* m_8; // +0x08
    i32 M(void* arg);
};
SIZE_UNKNOWN(Cdb200);

// 0x0db2f0 - finalize: run the +0x38 teardown iff +0x14 clear, then reset +0x20.
struct CSubdb2f0 {};
SIZE_UNKNOWN(CSubdb2f0);
struct Cdb2f0 {
    char pad0[0x14];
    i32 m_14; // +0x14
    char pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    char pad24[0x38 - 0x24];
    CSubdb2f0 m_38; // +0x38
    i32 M();
};
SIZE_UNKNOWN(Cdb2f0);

// 0x0db750 is rehomed as CPlay::LoadLevelAnims (Play.cpp) on the canonical
// CDDrawSubMgrLeaf/CSymTab.

// 0x0ea170 - 2-bit selector over a +0x38 virtual.
struct Cea170 {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void Dispatch(i32 a, i32 b, i32 c, i32 d, i32 e); // slot +0x38
    void M(i32 a1, i32 a2);
};
SIZE_UNKNOWN(Cea170);

// 0x104dd0 - lazy-create the StatusBarSprite (clamp then factory-build) through
// the canonical CDDrawChildGroup (<Gruntz/SpriteFactory.h>).
struct CHolder104 {
    char pad0[8];
    CDDrawChildGroup* m_8; // +0x08  the sprite factory (CreateSprite @0x1597b0)
};
SIZE_UNKNOWN(CHolder104);
struct C104dd0 {
    char pad0[8];
    CGameObject* m_sprite;       // +0x08  the created StatusBarSprite instance
    CHolder104* m_factoryHolder; // +0x0c
    char pad10[0x24 - 0x10];
    i32 m_x; // +0x24
    i32 m_y; // +0x28
    i32 Create();
};
SIZE_UNKNOWN(C104dd0);

// 0x10bbe0 - getter over the +0x528 gate / active-cell table.
struct C10bbe0 {
    char pad0[0x4cc];
    i32 m_fallback; // +0x4cc
    char pad4d0[0x528 - 0x4d0];
    i32 m_528;   // +0x528
    i32 m_index; // +0x52c
    char pad530[0x534 - 0x530];
    i32** m_entries; // +0x534
    i32 m_count;     // +0x538
    i32 M();
};
SIZE_UNKNOWN(C10bbe0);

// 0x112bf0 - decrement the active grid cell (manager-owned plane) + re-publish.
// CGridOuter/CGridHolder/CGridData/CHandler112 are per-use facet views of the
// singleton's m_world / m_tileGrid sub-objects (not yet modeled canonically).
struct CGridData {
    char pad0[0x20];
    i32* cells; // +0x20
    i32* rows;  // +0x24
};
SIZE_UNKNOWN(CGridData);
struct CGridHolder {
    char pad0[0x5c];
    CGridData* m_5c; // +0x5c
};
SIZE_UNKNOWN(CGridHolder);
struct CGridOuter {
    char pad0[0x24];
    CGridHolder* m_24; // +0x24
};
SIZE_UNKNOWN(CGridOuter);
struct CHandler112 {};
SIZE_UNKNOWN(CHandler112);
struct C112bf0 {
    char pad0[8];
    i32 m_col; // +0x08
    i32 m_row; // +0x0c
    char pad10[0x14 - 0x10];
    i32 m_14; // +0x14
    i32 M();
};
SIZE_UNKNOWN(C112bf0);

// 0x118260 - copy-if-grow (copy the 7-dword box in + stash +0xd4).
struct CRect118 {
    void* m_0;
    u32 m_4;
    u32 m_8;
    char pad[0x1c - 0xc]; // 7 dwords total
};
SIZE_UNKNOWN(CRect118);
struct C118260 {
    char pad0[0xb8];
    CRect118 m_bounds; // +0xb8 (7 dwords, ends at 0xd4)
    i32 m_d4;          // +0xd4
    i32 Update(CRect118* src, i32 arg2);
};
SIZE_UNKNOWN(C118260);

// --- vtable catalog ---

#endif // GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H
