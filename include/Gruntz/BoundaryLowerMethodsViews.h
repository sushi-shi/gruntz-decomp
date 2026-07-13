// BoundaryLowerMethodsViews.h - shared referent/owner views for the lower-half
// engine_boundary leaf methods reconstructed in BoundaryLowerMethods.cpp.
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only the OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). These were per-TU inline views; consolidating them into this shared
// header is pure code motion (matching-neutral: identical layouts/sizes/vtable
// counts -> identical codegen) and gives a sibling/final-sweep TU one definition to
// reuse. The serialize/archive object folds to the canonical CSerialArchive (Read
// @ +0x2c / Write @ +0x30).
// AXE WORKLIST (user mandate 2026-07-05: every struct here is a fake view to be
// dissolved onto its real class). Folded so far: C99ba0/C9a420 -> CAreaMgr/
// CSpawnList, C8e880/C915d0 -> CGruntzMgr.
//
// HOMED (matcher-1 re-home passes, raw-vtable/thunk-verified):
//   Cd5e20   -> CImage::Slot17 (src/Image/CImage.cpp): vtbl slot 17 (+0x44) thunk
//                0x1d1b jmps to 0xd5e20; forwards arg to Slot15/Slot16.
//   Gate113860 -> src/Gruntz/TileTriggerContainer.cpp (the __stdcall mode-gate).
//   Fwd114ec0 -> src/Gruntz/GruntzMgrCmd.cpp (the __cdecl 6-arg toolbar forwarder).
//   C113e70  -> CTileTriggerSwitchLogic::DeserializeMatrix (thunk 0x3cd3).
//   C104c80  -> CSBI_WellGoo::Free; ParseSerial/0xd210 -> GruntzMgrCmd.cpp.
//   C77dc0   -> CLevelPlane::SetCell (BrickzCellFlags_077790.cpp): the flat grid-cell
//                setter; CTerrainTileLoader::Load reaches it via loader->m_24
//                (the world holder) -> m_24 -> m_mainPlane. Same grid as C112bf0.
//   Cfa150   -> CGameModeBase::BaseCleanup (src/Gruntz/GameModeBase.cpp, own .obj):
//                caller graph = every state ReleaseResources; +0x1c allocator is the
//                real CDDrawPtrCollections::RemoveItemA. 94.5% (cmp-order wall).
//   Ceb970   -> CSBI_WarlordHead::Serialize (SBI_WarlordHead.cpp): vtbl slot 1
//                (thunk 0x3cd8 -> 0xeb970) is authoritative. CONFLICT RESOLVED - the
//                0xe7cd0 formerly mis-named CSBI_WarlordHead::Serialize is actually
//                CSBI_ImageSetAni::Serialize (slot 1, thunk 0x2829, shared with
//                CSBI_StatzTabArrow) -> re-homed to src/Gruntz/SBI_ImageSetAni.cpp.
//   C50ca0   -> CGrunt::LoadTypeTableClearMove (src/Gruntz/Grunt.cpp): this==CGrunt
//                (RunEntranceMove mov ecx,esi), m_1a0==CGrunt::m_moveMode, Method@0x3bd9
//                == inherited CUserLogic::LoadGruntTypeTable.
//
// HOMED (matcher-7): C0b4c40 -> CUFO::SerializeMove (src/Gruntz/GameObjectCtors.cpp),
//   vtbl slot 1 (thunk 0x3fb7 -> 0xb4c40). The base slot-1 re-signature
//   (CUserBase::SerializeMove 4-arg) that blocked it is now done; the 0x3035 chain
//   resolves to CUFO::Serialize (0xb4d30), m_10 == the bound CGameObject.
//
// BLOCKED - vtable-attributed but the home needs a base-model/conflict fix a follow-up
// must do (each stays @early-stop in place):
//   C112bf0  IS CCheckpointTriggerSwitchLogic::M (vtbl slot 3, thunk 0x36fc); home
//                needs the grid-access chain (g_reg->m_world->m_24->m_5c ==
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
//   CTypeColl464 <- RegisterWarlordActions/RegisterActs_* (a CZArray2D-derived act/proj
//                type collection; the exact sibling class of CTypeKeyColl unrecovered).
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
#include <Gruntz/SerialArchive.h> // canonical Read@+0x2c / Write@+0x30 archive stream
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)

// 0x0213a0 - virtual-base field getter (reads +0x04 of the virtual base whose
// displacement lives in the vbtable's second slot).
struct C213a0 {
    i32 Get();
};
SIZE_UNKNOWN(C213a0);

// 0x0464e0 - type-id -> entry resolver (projectile/act fast-range + Find + grow).
struct CVariantSlot; // canonical <Bute/ButeTree.h> (Set @0x16d850); pointer-only use here
// Field layout is the attributed sibling CTypeKeyColl : CZArray2D (TypeKeyColl.cpp)
// - it shares Find (0x16da80); this is the projectile/act instance.
struct CTypeColl464 {
    void* m_0;                // +0x00  vptr
    CVariantSlot* m_4;        // +0x04  grow-path node inserter
    i32 m_lo;                 // +0x08  index low bound
    i32 m_hi;                 // +0x0c  index high bound
    char* m_buf;              // +0x10  primary element buffer (base)
    i32 m_buf2;               // +0x14  scratch element (returned as the miss fallback)
    i32 m_stride;             // +0x18  element size
    char pad1c[0x20 - 0x1c];  // +0x1c cursor (== m_buf, unused here)
    i32 m_20;                 // +0x20 (== m_count, but reset to 0 on entry; role unproven)
    i32 Find(i32 key, i32 z); // 0x16da80
    void* Resolve(i32 key);
};
SIZE_UNKNOWN(CTypeColl464);

// (0x050ca0 C50ca0::M re-homed to src/Gruntz/Grunt.cpp as
// CGrunt::LoadTypeTableClearMove - this==CGrunt, m_1a0==m_moveMode.)

// (0x077dc0 C77dc0::Set re-homed as CLevelPlane::SetCell (BrickzCellFlags_077790.cpp)
// - the flat grid-cell setter; reached via holder->m_24 -> m_mainPlane.)

// (C8e880/CState8e [0x8e880] and C915d0/CMid915 + the duplicate CGruntzSoundInnerZ
// [0x915d0/0x91620] were views of CGruntzMgr - m_2c == m_curState (slot +0x10 ==
// CState::Update, the state id), m_14 == the level-loaded gate, m_48 == m_sound
// (canonical CGruntzSoundZ, m_1c == m_pCurrent). Dissolved onto <Gruntz/GruntzMgr.h>
// (RegisterSetSkillDebugCmd / MuteMusicIfActive / RestoreMusicVolumeIfActive) +
// <Dsndmgr/GruntzSoundZ.h>.)

// (C99ba0/CSub99ba0 [0x99ba0] and C9a420/CNode9a420/CBack9a420 [0x9a420] were
// views of CAreaMgr / CSpawnList / CSpawnEntry - dissolved onto the canonicals;
// see <Gruntz/AreaMgr.h> + <Gruntz/SpawnList.h>.)

// 0x09cab0 - out-param wrapper over the +0x10 sub's method (0x1b8008).
struct CSub9cab0 {
    // Get @0x1b8008 IS CMapStringToOb::Lookup (band [0x1b7e17,0x1b8247); mfc_class).
    // This said CMapStringToPtr - inverted. CMapStringToPtr's Lookup is 0x1b8438, and
    // retail never calls it here. => this empty view is a real ::CMapStringToOb.
};
SIZE_UNKNOWN(CSub9cab0);
struct C9cab0 {
    char pad0[0x10];
    CSub9cab0 m_10; // +0x10
    i32 M(i32 arg);
};
SIZE_UNKNOWN(C9cab0);

// (0x0b4c40 C0b4c40::Handle re-homed to src/Gruntz/GameObjectCtors.cpp as the REAL
// CUFO::SerializeMove - vtable slot 1 (thunk 0x3fb7). 0x3035 == CUFO::Serialize
// (0xb4d30); m_10 == the bound CGameObject, +0x58/+0x50/+0x54 ==
// m_drawActive/m_drawFillCmd/m_fillFraction. See <Gruntz/Ufo.h>.)

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

// (0x0d5e20 Cd5e20 re-homed to src/Image/CImage.cpp as CImage::Slot17 - vtable
// slot-17 thunk 0x1d1b jmps to 0xd5e20. See <Image/CImage.h>.)

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

// 0x0db750 - "LEVEL" config sync through the +0x0c owner's +0x2c config.
struct CDDrawSubMgrLeaf : public CObject {
    i32 HasKeyPrefix_152c50(const char* key); // 0x152c50 (i32 ret; caller tests != 0)
    i32 RemoveKeysEqual_1527d0(
        const char* key,
        const char* v
    ); // 0x1527d0 (i32 ret, 2nd arg const char*)
    void ScanTree_152ad0(void* val, const char* key, void* v); // 0x152ad0
    class CString
    KeyOfValue_152d30(CObject* v); // 0x152d30 (id->name; ret CString; real arg CObject*)
    void* LookupValue_06b2a0(const char* key); // 0x6b2a0  (id lookup; main's fold)
};
SIZE_UNKNOWN(CDDrawSubMgrLeaf);
struct CHolderdb {
    char pad0[0x2c];
    CDDrawSubMgrLeaf* m_2c; // +0x2c
};
SIZE_UNKNOWN(CHolderdb);
struct CSymTab {
    void* ResolvePath(const char* arg); // 0x13bae0 (real: void* ret, const char* arg)
    i32 ResolveQualified(const char* name, void* arg); // 0x13a0e0 (same sig as <Bute/SymTab.h>)
};
SIZE_UNKNOWN(CSymTab);
struct Cdb750 {
    char pad0[0xc];
    CHolderdb* m_c; // +0x0c
    char pad10[0x28 - 0x10];
    CSymTab* m_28; // +0x28
    i32 M(void* arg);
};
SIZE_UNKNOWN(Cdb750);

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

// (0x0eb970 Ceb970::Serialize re-homed to src/Gruntz/SBI_WarlordHead.cpp as the REAL
// CSBI_WarlordHead::Serialize (vtable slot 1, thunk 0x3cd8). The conflicting 0xe7cd0
// is CSBI_ImageSetAni::Serialize -> src/Gruntz/SBI_ImageSetAni.cpp. Conflict RESOLVED.)

// (0x0fa150 Cfa150::Cleanup re-homed to src/Gruntz/GameModeBase.cpp as
// CGameModeBase::BaseCleanup - the mode-holder's +0x1c allocator is the real
// CDDrawPtrCollections (RemoveItemA @0x142160). See <Gruntz/GameModeBase.h>.)

// (0x104c80 C104c80 re-homed to src/Gruntz/SBI_WellGoo.cpp as CSBI_WellGoo::Free -
// vtable slot-3 thunk 0x30b7 jmps to 0x104c80. See <Gruntz/SBI_WellGoo.h>.)

// 0x104dd0 - lazy-create the StatusBarSprite (clamp then factory-build) through
// the canonical CSpriteFactory (<Gruntz/SpriteFactory.h>; the former local
// CSpriteFactory re-definition here collided with the canonical class name).
struct CHolder104 {
    char pad0[8];
    CSpriteFactory* m_8; // +0x08  the sprite factory (CreateSprite @0x1597b0)
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

// (0x113e70 C113e70 re-homed to src/Gruntz/TileTriggerSwitchLogic.cpp as
// CTileTriggerSwitchLogic::DeserializeMatrix. See <Gruntz/TileTriggerSwitchLogic.h>.)

// (0x114f00 forwarder re-homed to src/Gruntz/GruntzMgrCmd.cpp with its CArg114f/
// CObj114f command-context view chain; dissolved here.)

// (0x1181d0 bounds-grow re-homed to src/Gruntz/NameRecord.cpp with its CBox118/
// C1181d0 view; dissolved here.)

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
