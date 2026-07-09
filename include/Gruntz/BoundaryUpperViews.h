// BoundaryUpperViews.h - shared referent/owner views for the upper-half
#include <Wap32/Object.h> // CObject grand-base (real virtual dtor)
// (RVA >= 0x133370) engine_boundary leaf methods reconstructed in BoundaryUpper.cpp
// (DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). Formerly per-TU inline views; consolidating them here is pure code
// motion (matching-neutral) and gives the /GX EH-frame sibling (BoundaryUpperEh.cpp)
// / a final sweep one definition to reuse.
#ifndef GRUNTZ_BOUNDARYUPPERVIEWS_H
#define GRUNTZ_BOUNDARYUPPERVIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Blk6c.h> // the 0x6c-byte CImageOwned transform descriptor

// Embedded base-subobject vptr restamp (member dtor of the grand-base): the 7-byte
// `mov [this],&g_wapObjectDtorVtbl; ret` leaf. Three distinct leaf classes share it.
struct SW_161460 : CObject {};
SIZE_UNKNOWN(SW_161460);
struct SW_161560 : CObject {};
SIZE_UNKNOWN(SW_161560);
struct SW_163a10 : CObject {};
SIZE_UNKNOWN(SW_163a10);

// 0x1413c0 - `return m_20 * n;` (CDirectDrawMgr-area scale helper).
struct B_1413c0 {
    i32 _0[8];
    i32 m_20; // 0x20
    i32 Scale(i32 n);
};
SIZE_UNKNOWN(B_1413c0);

// 0x1614b0 - `if(m_14) RezFree(m_14); m_14 = 0;` (CImageSet1-area buffer release).
struct B_1614b0 {
    i32 _0[5];
    void* m_14; // 0x14
    void Release();
};
SIZE_UNKNOWN(B_1614b0);

// (0x137300 SoundDevice::GetPrimary re-homed to src/Dsndmgr/DirectSoundMgr.cpp;
// SoundDevice view dissolved onto the canonical <Dsndmgr/SoundDevice.h>.)

// 0x1433d0 - CDdObArray ordered compare (unsigned m_c then m_8, then m_54 0/1).
struct DdOb_1433d0 {
    u32 _0[2];
    u32 m_8; // 0x8
    u32 m_c; // 0xc
    u32 _10[(0x54 - 0x10) / 4];
    u32 m_54; // 0x54
};
SIZE_UNKNOWN(DdOb_1433d0);

// 0x1847a0 - trivial setter `m_70 = arg;`.
struct B_1847a0 {
    char _0[0x70];
    i32 m_70; // 0x70
    void Set(i32 v);
};
SIZE_UNKNOWN(B_1847a0);

// 0x17fc40 - `if(m_50) RezFree(m_50);` (no zero-out).
struct B_17fc40 {
    char _0[0x50];
    void* m_50; // 0x50
    void Free();
};
SIZE_UNKNOWN(B_17fc40);

// (0x134360/0x1346d0 device-config teardowns re-homed to DirectInputMgr2.cpp onto
// CDeviceConfigB/CDeviceConfigC; the DevCfg view is dissolved.)

// 0x1413b0 - manual-vtable dispatch `(*m_8->vtbl[0x80])(m_8, 0)`.
struct Obj1413 { // real polymorphic; Op is slot 32 (+0x80)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Slot18();
    virtual void Slot19();
    virtual void Slot20();
    virtual void Slot21();
    virtual void Slot22();
    virtual void Slot23();
    virtual void Slot24();
    virtual void Slot25();
    virtual void Slot26();
    virtual void Slot27();
    virtual void Slot28();
    virtual void Slot29();
    virtual void Slot30();
    virtual void Slot31();
    virtual void __stdcall Op(i32); // slot 32 (+0x80)
};
SIZE_UNKNOWN(Obj1413);
struct Owner1413 {
    char _0[8];
    Obj1413* m_8; // 0x8
    void Thunk();
};
SIZE_UNKNOWN(Owner1413);

// CDdObArray mode-table search (m_4b8 = Entry*[], m_4bc = count).
struct DdEntry {
    char _0[8];
    u32 m_8; // 0x8
    u32 m_c; // 0xc
    char _10[0x54 - 0x10];
    i32 m_54; // 0x54
};
SIZE_UNKNOWN(DdEntry);
struct ModeArr {
    char _0[0x4b8];
    DdEntry** m_4b8; // 0x4b8
    i32 m_4bc;       // 0x4bc
    i32 FindIndex(i32 k0, i32 k1, i32 k2);
    i32 FindLast(u32 k0, u32 k1, i32 k2);
    void FindFwd(struct Pair2* out, i32 k0, i32 k1, i32 k2);
    void FindBack(struct Pair2* out, i32 k0, i32 k1, i32 k2);
};
SIZE_UNKNOWN(ModeArr);

// 0x13dee0 - `m_1c = v; if(v > 0) m_28 = 1000 / v;` (CFileImage frame timing).
struct B_13dee0 {
    char _0[0x1c];
    i32 m_1c; // 0x1c
    char _20[0x28 - 0x20];
    i32 m_28; // 0x28
    void Set(i32 v);
};
SIZE_UNKNOWN(B_13dee0);

// 0x13ee30 - COM wait-flip loop (IDirectDrawSurface-style manual vtable, slot 0x48).
struct IDDS_ee30 { // real polymorphic; Flip is slot 18 (+0x48)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual u32 __stdcall Flip(i32); // slot 18 (+0x48)
};
SIZE_UNKNOWN(IDDS_ee30);
struct B_13ee30 {
    char _0[8];
    IDDS_ee30* m_8; // 0x8
    void WaitFlip();
};
SIZE_UNKNOWN(B_13ee30);

// 0x151e70 - clear: zero m_10, release +0x14, scalar-delete the +0x18 object.
struct Killable0 {
    virtual void Destroy(i32); // slot 0
};
SIZE_UNKNOWN(Killable0);
struct B_151e70 {
    char _0[0x10];
    i32 m_10;        // 0x10
    void* m_14;      // 0x14
    Killable0* m_18; // 0x18
    char _1c[0x170 - 0x1c];
    i32 m_170; // 0x170
    char _174[0x178 - 0x174];
    i32 m_178; // 0x178
    void Clear();
};
SIZE_UNKNOWN(B_151e70);

// 0x166810 - destroy a singly-linked node list, then RemoveAll the +0x1dc array.
struct Killable1 {
    virtual void V0();
    virtual void Destroy(i32); // slot 1
};
SIZE_UNKNOWN(Killable1);
struct Node166810 {
    Node166810* m_next; // 0x0 link
    char _4[4];
    Killable1* m_payload; // 0x8 payload
};
SIZE_UNKNOWN(Node166810);
struct Arr1dc {
}; // RemoveAll @0x1b5a0b IS an MFC array RemoveAll (CObArray-family); cast at the call
SIZE_UNKNOWN(Arr1dc);
struct B_166810 {
    char _0[0x1dc];
    Arr1dc m_1dc;       // 0x1dc
    Node166810* m_head; // 0x1e0
    void Clear();
};
SIZE_UNKNOWN(B_166810);

// (0x13c8a0 CRezItm::Scan re-homed to src/Rez/RezMgr.cpp; RezItm/RezOwner views
// dissolved onto CRezItm/CRezItmOwner.)

// (0x13c8f0 CRezItm::Check re-homed to src/Rez/RezMgr.cpp; RezDir view dissolved onto
// the canonical CRezItm.)

// CDdObArray neighbour lookup out pair (0x143510 fwd / 0x143590 back).
struct Pair2 {
    i32 a, b;
};
SIZE_UNKNOWN(Pair2);

// 0x138f20 - DSound voice gate.
struct Snd138f20 {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual i32 v8();          // slot 8 (+0x20)
    virtual void v9(i32, i32); // slot 9 (+0x24)
    char _4[0x44 - 0x4];
    i32 m_44;     // 0x44
    i32 m_48;     // 0x48
    i32 m_4c;     // 0x4c
    i32 Helper(); // 0x138f60
    i32 Gate();
};
SIZE_UNKNOWN(Snd138f20);

// 0x16be60 - ButeMgr helper append.
struct C16be60 {
    i32 Ready();                     // 0x16bd10
    void Emit(const void* s, i32 a); // 0x16c2d0
    void Flush();                    // 0x16bd90
    C16be60* Append(i32 arg);
};
SIZE_UNKNOWN(C16be60);

// 0x151d20 - notify a hooked callback (stash/replace m_7c->m_1c).
struct Cb151d20 {
    char _0[0x10];
    void(__cdecl* fn)(void*); // +0x10
    char _14[0x1c - 0x14];
    void* m_1c; // 0x1c
};
SIZE_UNKNOWN(Cb151d20);
struct B_151d20 {
    char _0[0x7c];
    Cb151d20* m_7c; // 0x7c
    i32 Notify(void* arg);
};
SIZE_UNKNOWN(B_151d20);

// Global-object tail-forwards (load the global's addr into ecx, tail-jump 0x1b4f0b).
class CImageCache {
public:
    void RemoveAll(); // 0x1b4f0b
};
SIZE_UNKNOWN(CImageCache);
struct CDdObArray {
    void RemoveAll(); // 0x1b4f0b
};
SIZE_UNKNOWN(CDdObArray);

// CImageOwned apply/setup cluster (vptr slot 8 = +0x20 transform, slot 10 = +0x28).
struct ImgOwned {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual i32 v8(i32); // slot 8 (+0x20)
    virtual void v9();
    virtual void v10(); // slot 10 (+0x28)
    char _4[0x10 - 0x4];
    Blk6c m_transform; // 0x10 (0x6c-byte transform descriptor)
    i32 Apply(i32 mode, const void* src);
    i32 Forward(i32 a0, const void* a1);
    i32 Commit(i32 a0, const void* a1);
};
SIZE_UNKNOWN(ImgOwned);

// (0x13dec0 RezMgr::SpinWaitUntil re-homed to src/Rez/RezMgr.cpp; RezMgr view dissolved.)

// --- vtable catalog ---

#endif // GRUNTZ_BOUNDARYUPPERVIEWS_H
