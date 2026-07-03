// BoundaryUpperViews.h - shared referent/owner views for the upper-half
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
struct SW_161460 {
    void Restamp();
};
SIZE_UNKNOWN(SW_161460);
struct SW_161560 {
    void Restamp();
};
SIZE_UNKNOWN(SW_161560);
struct SW_163a10 {
    void Restamp();
};
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

// 0x137300 - SoundDevice getter (Probe @0x137260, reloc-masked).
struct Snd_137300 {
    char _0[0x78];
    void* m_78; // 0x78
    char _7c[0x84 - 0x7c];
    i32 m_84;    // 0x84
    i32 Probe(); // 0x137260
    i32 Get();
};
SIZE_UNKNOWN(Snd_137300);

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

// 0x134360 / 0x1346d0 - DirectInput device-config teardown. Two identical leaves.
struct DevCfg {
    char _0[0x2a0];
    void* m_2a0;        // 0x2a0
    void* m_2a4;        // 0x2a4
    void ReleaseBase(); // 0x1342b0
    void Free360();
    void Free6d0();
};
SIZE_UNKNOWN(DevCfg);

// 0x1413b0 - manual-vtable dispatch `(*m_8->vtbl[0x80])(m_8, 0)`.
struct Obj1413;
struct Vtbl1413 {
    char _0[0x80];
    void(__stdcall* Op)(Obj1413*, i32); // +0x80
};
SIZE_UNKNOWN(Vtbl1413);
struct Obj1413 {
    Vtbl1413* vtbl;
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
struct IDDS_ee30;
struct IDDSVtbl_ee30 {
    char _0[0x48];
    u32(__stdcall* Flip)(IDDS_ee30*, i32); // +0x48
};
SIZE_UNKNOWN(IDDSVtbl_ee30);
struct IDDS_ee30 {
    IDDSVtbl_ee30* vtbl;
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
    void RemoveAll(); // 0x1b5a0b
};
SIZE_UNKNOWN(Arr1dc);
struct B_166810 {
    char _0[0x1dc];
    Arr1dc m_1dc;       // 0x1dc
    Node166810* m_head; // 0x1e0
    void Clear();
};
SIZE_UNKNOWN(B_166810);

// 0x13c8a0 - CRezItm scan retry loop.
struct RezOwner {
    virtual void v0();
    virtual void v1();
    virtual i32 v2(); // slot 2 (+8)
};
SIZE_UNKNOWN(RezOwner);
struct RezItm {
    char _0[0xc];
    RezOwner* m_owner; // 0xc
    void* m_handle;    // 0x10  resource handle (RezItmProbe)
    char _14[0x20 - 0x14];
    i32 m_20; // 0x20
    i32 Scan();
};
SIZE_UNKNOWN(RezItm);

// 0x13c8f0 - CRezDir check (lookup m_10, else dispatch virtual slot 4).
struct RezDir {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual i32 v4(i32, i32, i32); // slot 4 (+0x10)
    char _4[0x10 - 0x4];
    void* m_10; // 0x10
    i32 m_14;   // 0x14
    i32 m_18;   // 0x18
    char _1c[0x20 - 0x1c];
    i32 m_20; // 0x20
    i32 Check();
};
SIZE_UNKNOWN(RezDir);

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

// 0x13dec0 - millisecond frame-pacing busy-wait via the timeGetTime fn-ptr global.
// Minimal RezMgr view (the full RezMgr.h is /O2-sensitive in other TUs).
class RezMgr {
public:
    void SpinWaitUntil(i32 ms); // 0x13dec0
};

#endif // GRUNTZ_BOUNDARYUPPERVIEWS_H
