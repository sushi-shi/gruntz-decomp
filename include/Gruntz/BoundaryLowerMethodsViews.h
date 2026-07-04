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
#ifndef GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H
#define GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H

#include <Ints.h>
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
struct CVariantSlot {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (ret 0xc)
};
SIZE_UNKNOWN(CVariantSlot);
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

// 0x050ca0 - dispatch then reset the +0x1a0/+0x1a4 pair.
struct C50ca0 {
    char pad0[0x1a0];
    i32 m_1a0;                               // +0x1a0
    i32 m_1a4;                               // +0x1a4
    void Method(i32 a, i32 b, i32 c, i32 d); // 0x3bd9
    void M(i32 arg);
};
SIZE_UNKNOWN(C50ca0);

// 0x077dc0 - cell setter: m_20[ m_24[idx] + base ] = value.
struct C77dc0 {
    char pad0[0x20];
    i32* m_20; // +0x20
    i32* m_24; // +0x24
    void Set(i32 base, i32 idx, i32 value);
};
SIZE_UNKNOWN(C77dc0);

// 0x08e880 - debug command hook: if the +0x2c sub-object's state slot (vtbl +0x10)
// reports 3, register the DEBUG_SETSKILL command.
struct CState8e {
    virtual i32 v0();
    virtual i32 v1();
    virtual i32 v2();
    virtual i32 v3();
    virtual i32 GetState(); // slot +0x10
};
SIZE_UNKNOWN(CState8e);
struct C8e880 {
    char pad0[0x2c];
    CState8e* m_2c;                                  // +0x2c
    void Cmd2bb7(const char* name, void* fn, i32 n); // 0x2bb7
    i32 M();
};
SIZE_UNKNOWN(C8e880);

// 0x0915d0 / 0x091620 - guarded dispatch through the +0x48 sub's +0x1c probe.
struct CGruntzSoundInnerZ {
    i32 IsBusy();                 // 0x138f60
    void SetVolume(i32 a, i32 b); // 0x138fd0
};
SIZE_UNKNOWN(CGruntzSoundInnerZ);
struct CMid915 {
    char pad0[0x1c];
    CGruntzSoundInnerZ* m_1c; // +0x1c
};
SIZE_UNKNOWN(CMid915);
struct C915d0 {
    char pad0[0x14];
    void* m_14; // +0x14
    char pad18[0x48 - 0x18];
    CMid915* m_48; // +0x48
    void M0(void* arg);
    void M64(void* arg);
};
SIZE_UNKNOWN(C915d0);

// 0x099ba0 - ctor: build the +0x04 sub-object, seed the bookkeeping fields.
struct CSub99ba0 {
    char pad0[0x1c];
    i32 m_1c;         // +0x1c (== owner +0x20)
    i32 m_20;         // +0x20 (== owner +0x24)
    void Init(i32 n); // 0x1b4867
};
SIZE_UNKNOWN(CSub99ba0);
struct C99ba0 {
    void* m_0;       // +0x00
    CSub99ba0 m_sub; // +0x04
    C99ba0* Ctor();
};
SIZE_UNKNOWN(C99ba0);

// 0x09a420 - walk the +0x04 linked list, clear each node's back-pointer +0x04.
struct CBack9a420 {
    char pad0[4];
    i32 m_4; // +0x04
};
SIZE_UNKNOWN(CBack9a420);
struct CNode9a420 {
    CNode9a420* m_next; // +0x00
    char pad4[8 - 4];
    CBack9a420* m_8; // +0x08
};
SIZE_UNKNOWN(CNode9a420);
struct C9a420 {
    char pad0[4];
    CNode9a420* m_head; // +0x04
    void Clear();
};
SIZE_UNKNOWN(C9a420);

// 0x09cab0 - out-param wrapper over the +0x10 sub's method (0x1b8008).
struct CSub9cab0 {
    void Get(i32 a, i32* out); // 0x1b8008
};
SIZE_UNKNOWN(CSub9cab0);
struct C9cab0 {
    char pad0[0x10];
    CSub9cab0 m_10; // +0x10
    i32 M(i32 arg);
};
SIZE_UNKNOWN(C9cab0);

// 0x0b4c40 - dispatch a 4-arg action (0x3035); on success + arg2==8 arm the +0x10 sub.
struct CSubB4 {
    char pad0[0x50];
    i32 m_50; // +0x50
    i32 m_54; // +0x54
    i32 m_58; // +0x58
};
SIZE_UNKNOWN(CSubB4);
struct C0b4c40 {
    char pad0[0x10];
    CSubB4* m_10;                                 // +0x10
    i32 Dispatch3035(i32 a, i32 b, i32 c, i32 d); // 0x3035
    i32 Handle(i32 a1, i32 a2, i32 a3, i32 a4);
};
SIZE_UNKNOWN(C0b4c40);

// 0x0bd450 - init: run the base ctor (0x3625) then open the "c:\gruntz.log" log.
struct Cbd450 {
    void Base3625();                 // 0x3625
    void OpenLog1983(const char* s); // 0x1983
    void Init();
};
SIZE_UNKNOWN(Cbd450);

// 0x0cef50 - teardown of the +0x04 owner + inner close chain.
struct CSubC8 {
    void M1b9c69(); // 0x1b9c69
};
SIZE_UNKNOWN(CSubC8);
struct CObjC {
    void M201d(i32 a); // 0x201d
};
SIZE_UNKNOWN(CObjC);
struct CDDrawWorkerMgr {
    i32 M158d20();  // 0x158d20
    void M158e40(); // 0x158e40
};
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

// 0x0d5e20 - forward an arg through two virtuals (vtbl +0x3c then +0x40).
struct Cd5e20 {
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
    virtual void v14();
    virtual void v15(void* a); // slot +0x3c
    virtual void v16(void* a); // slot +0x40
    void M(void* arg);
};
SIZE_UNKNOWN(Cd5e20);

// 0x0db200 - swap the +0x08 holder to `arg` (validate + toggle old/new).
struct Cdb200 {
    char pad0[8];
    void* m_8; // +0x08
    i32 M(void* arg);
};
SIZE_UNKNOWN(Cdb200);

// 0x0db2f0 - finalize: run the +0x38 teardown iff +0x14 clear, then reset +0x20.
struct CSubdb2f0 {
    void Teardown40c5(); // 0x40c5
};
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
struct CDDrawSubMgrLeaf {
    void* HasKeyPrefix_152c50(const char* key);                // 0x152c50
    void RemoveKeysEqual_1527d0(const char* key, void* v);     // 0x1527d0
    void ScanTree_152ad0(void* val, const char* key, void* v); // 0x152ad0
};
SIZE_UNKNOWN(CDDrawSubMgrLeaf);
struct CHolderdb {
    char pad0[0x2c];
    CDDrawSubMgrLeaf* m_2c; // +0x2c
};
SIZE_UNKNOWN(CHolderdb);
struct CSymTab {
    i32 ResolvePath(void* arg); // 0x13bae0
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

// 0x0eb970 - Serialize: transfer +0x3c via the archive Write (mode 4) / Read (mode 7),
// then chain the base serializer.
struct Ceb970 {
    char pad0[0x3c];
    i32 m_3c;                                                 // +0x3c
    i32 Base3ca1(CSerialArchive* ar, i32 a2, i32 a3, i32 a4); // 0x3ca1
    i32 Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4);
};
SIZE_UNKNOWN(Ceb970);

// 0x0fa150 - release the four owned blits through the +0x0c owner's +0x1c allocator.
struct CDDrawPtrCollections {
    void Free(void* p); // 0x142160
};
SIZE_UNKNOWN(CDDrawPtrCollections);
struct CMidFa {
    char pad0[0x1c];
    CDDrawPtrCollections* m_1c; // +0x1c
};
SIZE_UNKNOWN(CMidFa);
struct Cfa150 {
    char pad0[0xc];
    CMidFa* m_c; // +0x0c
    char pad10[0x14 - 0x10];
    void* m_14; // +0x14
    void* m_18; // +0x18
    char pad1c[0x3c - 0x1c];
    i32 m_3c; // +0x3c
    char pad40[0x160 - 0x40];
    void* m_160; // +0x160
    void* m_164; // +0x164
    void Cleanup();
};
SIZE_UNKNOWN(Cfa150);

// 0x104c80 - release the +0x34 blit through the +0x24 owner's +0x1c allocator.
struct CMid104 {
    char pad0[0x1c];
    CDDrawPtrCollections* m_1c; // +0x1c
};
SIZE_UNKNOWN(CMid104);
struct C104c80 {
    char pad0[0x24];
    CMid104* m_24; // +0x24
    char pad28[0x34 - 0x28];
    void* m_34; // +0x34
    void Free();
};
SIZE_UNKNOWN(C104c80);

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
struct CHandler112 {
    void Notify(i32 a, i32 b, i32 c); // 0x33f0
};
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

// 0x113e70 - Serialize: transfer +0xc0/+0xc4 and the 3x3 dword block via the archive
// Read slot (+0x2c).
struct C113e70 {
    char pad0[0x9c];
    i32 m_9c[9]; // +0x9c .. +0xbc
    i32 m_c0;    // +0xc0
    i32 m_c4;    // +0xc4
    i32 Serialize(CSerialArchive* ar);
};
SIZE_UNKNOWN(C113e70);

// 0x114f00 - guarded forwarder: resolve a2->m_30->m_4->m_10->m_2c and forward.
struct CObj114f {
    char pad0[0x2c];
    void* m_2c; // +0x2c
};
SIZE_UNKNOWN(CObj114f);
struct CMid114f {
    char pad0[0x10];
    CObj114f* m_10; // +0x10
};
SIZE_UNKNOWN(CMid114f);
struct CHolder114f {
    char pad0[4];
    CMid114f* m_4; // +0x04
};
SIZE_UNKNOWN(CHolder114f);
struct CArg114f {
    char pad0[0x30];
    CHolder114f* m_30; // +0x30
};
SIZE_UNKNOWN(CArg114f);

// 0x1181d0 - bounds-grow (store the new (+0x04,+0x08) pair + notify).
struct CBox118 {
    void* m_0;
    u32 m_4;
    u32 m_8;
};
SIZE_UNKNOWN(CBox118);
struct C1181d0 {
    char pad0[0xb8];
    CBox118 m_bounds; // +0xb8
    char padd4[0xd4 - 0xb8 - 0xc];
    i32 m_d4; // +0xd4
    i32 Update(i32 a1, i32 a2, i32 a3);
};
SIZE_UNKNOWN(C1181d0);

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

#endif // GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H
