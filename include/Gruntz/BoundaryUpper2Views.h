// BoundaryUpper2Views.h - shared referent/owner views for the second-pass upper-half
// (RVA >= 0x133370) engine_boundary leaf methods reconstructed in BoundaryUpper2.cpp
// (DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). Formerly per-TU inline views; consolidating them here is pure code
// motion (matching-neutral). Three placeholders were renamed off real class names
// they collided with (they are DIFFERENT classes): the page-table store (was the
// misleading "CDDPageMgr", the real one is the DirectDraw device mgr in
// CDirectDrawMgr.h) and the two flat volatile-tuned worker-reset views (were
// "CDDrawWorkerA/B", whose real polymorphic shape lives in CDDrawWorkerNode.h - this
// TU deliberately keeps the redundant-store `volatile` flat copy).
#ifndef GRUNTZ_BOUNDARYUPPER2VIEWS_H
#define GRUNTZ_BOUNDARYUPPER2VIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Blk6c.h> // the 0x6c-byte CImageOwned transform descriptor

// 0x184b70 - global-object tail-forward (tail-jump the shared teardown 0x185000).
struct CHashTail {}; // Clear @0x185000 IS CDebugConfig::InitFromEnv; cast at the call
SIZE_UNKNOWN(CHashTail);

// 0x133370 - CInputDevRoot grand-base dtor (stamps its C-level vftable 0x5ef670 ==
// ??_7CInputDevRoot, cl-emitted via VTBL in DirectInputMgr2.cpp). BaseTeardown 0x134d50 ==
// CInputDevRoot::ReleaseDevices. Kept standalone (binding ~CInputDevRoot here would dup
// DinMgr2's inline base dtor - deferred to the DirectInput chain-reunify sweep).
struct DICfgC {
    void BaseTeardown(); // 0x134d50 == CInputDevRoot::ReleaseDevices
    void DtorC();
};
SIZE_UNKNOWN(DICfgC);

// 0x1396f0 - CParseSource-area init. The 0x5ef740 vtable is a SECONDARY / embedded
// intrusive-hash-node vtable (stamped at +0x1c). Realized as an embedded polymorphic
// node member (placement-constructed so its implicit vptr stamp supplies the store).
struct HashNode1396f0 {
    virtual void v0();
    virtual void v1();
};
VTBL(HashNode1396f0, 0x001ef740); // ??_7HashNode1396f0 @ 0x5ef740 (reloc-masked)
SIZE_UNKNOWN(HashNode1396f0);
struct CParseSource {
    void* m_0; // +0x00
    i32 _4[(0x10 - 0x4) / 4];
    i32 m_10; // +0x10
    i32 _14[(0x1c - 0x14) / 4];
    HashNode1396f0 m_1c; // +0x1c (embedded polymorphic node; ctor stamps its vptr)
    i32 _20[(0x30 - 0x20) / 4];
    void* volatile m_30; // +0x30 (0 then self; volatile pins the dead store + order)
    i32 m_34;            // +0x34
    CParseSource* Init();
};
SIZE_UNKNOWN(CParseSource);

// 0x1570d0 / 0x157240 - flat volatile-tuned worker reset (redundant-store shape). This
// is NOT the polymorphic CDDrawWorkerA/B in CDDrawWorkerNode.h - renamed to avoid the
// collision. A has a byte +0x78; B a dword.
struct CDDWorkerFlatA {
    void* m_0; // +0x00 vptr
    i32 m_4;   // +0x04
    i32 m_8;   // +0x08
    i32 m_c;   // +0x0c
    i32 _10[(0x20 - 0x10) / 4];
    volatile i32 m_20; // +0x20 (written thrice - redundant stores kept in retail)
    i32 _24[(0x38 - 0x24) / 4];
    volatile i32 m_38; // +0x38
    i32 _3c[(0x5c - 0x3c) / 4];
    i32 m_5c; // +0x5c
    i32 _60[(0x78 - 0x60) / 4];
    i8 m_78; // +0x78 (byte)
    void Reset();
};
SIZE_UNKNOWN(CDDWorkerFlatA);
struct CDDWorkerFlatB {
    void* m_0; // +0x00 vptr
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 _10[(0x20 - 0x10) / 4];
    volatile i32 m_20;
    i32 _24[(0x38 - 0x24) / 4];
    volatile i32 m_38;
    i32 _3c[(0x5c - 0x3c) / 4];
    i32 m_5c;
    i32 _60[(0x78 - 0x60) / 4];
    i32 m_78; // +0x78 (dword)
    void Reset();
};
SIZE_UNKNOWN(CDDWorkerFlatB);

// CDDPageMgr-boundary page-table store (embedded polymorphic sub @+0x124, CObArray-like
// @+0x138, int* table @+0x13c). Renamed off the real "CDDPageMgr" (a different class in
// CDirectDrawMgr.h). Methods Init 0x17b510 / Close 0x17b570 / Free 0x17b5a0 / Lookup 0x17b840.
struct DDPageSub {
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
    virtual i32 v12(i32, i32); // slot 0x30
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21(); // slot 0x54
};
SIZE_UNKNOWN(DDPageSub);
struct DDPageArr {
    void RemoveAt(i32, i32); // 0x1b4bad
};
SIZE_UNKNOWN(DDPageArr);
struct CPageStore17b510 {
    i32 m_initialized; // +0x00  init-once guard
    i32 m_4;           // +0x04
    i32 m_8;           // +0x08
    i32 m_c;           // +0x0c
    i32 m_10;          // +0x10
    u32 m_count;       // +0x14  entry count (bounds the 1-based index)
    i32 m_18[0x43];    // +0x18 .. +0x123
    DDPageSub m_sub;   // +0x124  embedded polymorphic sub-object
    i32 m_128;         // +0x128
    i32 _12c[(0x134 - 0x12c) / 4];
    i32 m_134;           // +0x134
    DDPageArr m_pageArr; // +0x138  CObArray-like page store
    i32* m_slots;        // +0x13c  index table
    i32 Init();          // 0x17b510
    void Close();        // 0x17b570
    i32 Free();          // 0x17b5a0
    i32 Lookup(u32);     // 0x17b840
};
SIZE_UNKNOWN(CPageStore17b510);

// 0x174ed0 / 0x175780 - CImagePool free/clear pair: RE-HOMED to the imagepool unit
// (src/Image/ImagePool.cpp), reconciled onto the real CImagePool / CRezImage types.

// 0x137200 - SoundDevice::StartPrimary: RE-HOMED to the directsoundmgr unit
// (src/Dsndmgr/DirectSoundMgr.cpp, real SoundDevice in SoundDevice.h).

// 0x13e7d0 - CDDSurface restore (build a 0x64-byte descriptor + restore helper).
struct RestoreDesc {
    i32 size; // +0x00
    i32 _4[(0x50 - 0x4) / 4];
    i32 m_50; // +0x50
    i32 _54[(0x64 - 0x54) / 4];
};
SIZE_UNKNOWN(RestoreDesc);
struct CDDSurface {
    i32 Restore(void* arg1, i32 arg2);
    i32 H(void* a, i32 b, i32 c, i32 flags, RestoreDesc* d); // 0x13eef0
};
SIZE_UNKNOWN(CDDSurface);

// 0x143040 - CDDrawPtrCollections factory (allocate+zero a 0x38-byte node, Init, Add).
struct CDDPalette {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 _1c[(0x2c - 0x1c) / 4];
    i32 m_2c;
    i32 m_30;
    i32 m_34;
    CDDPalette() {
        m_4 = 0;
        m_0 = 0;
        m_8 = 0;
        m_c = 0;
        m_10 = 0;
        m_34 = 0;
        m_18 = 0;
        m_14 = 0;
        m_2c = 0;
        m_30 = 0;
    }
    i32 Init(i32, i32, i32); // 0x147390
    void Cleanup();          // 0x147530
};
SIZE_UNKNOWN(CDDPalette);
struct CDDrawPtrCollections {
    i32 m_0;               // +0x00
    void Add(CDDPalette*); // 0x142eb0
    CDDPalette* Create(i32 a, i32 b);
};
SIZE_UNKNOWN(CDDrawPtrCollections);

// 0x1509c0 - CWwdGameObject visibility test.
struct WwdExtent {
    i32 _0[0x18 / 4];
    i32 m_halfW; // +0x18  half-width
    i32 m_halfH; // +0x1c  half-height
};
SIZE_UNKNOWN(WwdExtent);
struct WwdCamRect {
    i32 a; // +0x40 (left)
    i32 b; // +0x44 (top)
    i32 c; // +0x48 (right)
    i32 d; // +0x4c (bottom)
};
SIZE_UNKNOWN(WwdCamRect);
struct WwdCamHolder {
    i32 _0[0x5c / 4];
    char* m_5c; // +0x5c -> &m_40 rect via +0x40
};
SIZE_UNKNOWN(WwdCamHolder);
struct WwdGridLim {
    i32 _0[0x10 / 4];
    i32 m_width;  // +0x10
    i32 m_height; // +0x14
};
SIZE_UNKNOWN(WwdGridLim);
struct WwdGridHolder {
    i32 _0[0x10 / 4];
    WwdGridLim* m_limits; // +0x10
};
SIZE_UNKNOWN(WwdGridHolder);
struct WwdCtx {
    i32 _0[1];             // +0x00
    WwdGridHolder* m_grid; // +0x04
    i32 _8[(0x24 - 0x8) / 4];
    WwdCamHolder* m_camera; // +0x24
};
SIZE_UNKNOWN(WwdCtx);
struct CWwdGameObject {
    i32 _0[2];     // +0x00
    u32 m_flags;   // +0x08
    WwdCtx* m_ctx; // +0x0c
    i32 _10[(0x5c - 0x10) / 4];
    i32 m_centerX; // +0x5c
    i32 m_centerY; // +0x60
    i32 _64[(0x198 - 0x64) / 4];
    WwdExtent* m_extent; // +0x198
    i32 Test();
};
SIZE_UNKNOWN(CWwdGameObject);

// 0x13a530 - CSymTab remove-entry.
struct CHashBase {
    void Unlink(void* p); // 0x184ab0
};
SIZE_UNKNOWN(CHashBase);
struct SymEntry2 {
    i32 _0[0xc / 4];
    i32 m_span; // +0x0c
    i32 _10[(0x1c - 0x10) / 4];
    i32 m_1c; // +0x1c
    // Teardown @0x1397a0 IS Obj1397a0::Teardown; cast at the call.
};
SIZE_UNKNOWN(SymEntry2);
struct SymEntry1 {
    i32 _0[0x24 / 4];
    CHashBase m_24; // +0x24
};
SIZE_UNKNOWN(SymEntry1);
struct SymList18 {
    i32 _0[2];
    i32 m_count; // +0x08
    // Drop @0x13c210 IS CSymParser::AddNode; cast at the call.
};
SIZE_UNKNOWN(SymList18);
struct CSymTab {
    i32 _0[4];
    i32 m_size;        // +0x10  running total span
    i32 _14;           // +0x14
    SymList18* m_list; // +0x18
    i32 Remove(SymEntry1* a1, SymEntry2* a2);
};
SIZE_UNKNOWN(CSymTab);

// 0x17e230 - destroy a by-value CDataBuffer-like parameter.
struct DBuf17e230 {
    void* p;
    ~DBuf17e230(); // 0x1b9cde
};
SIZE_UNKNOWN(DBuf17e230);

// 0x143950 - CDDrawPtrCollections palette upload.
struct CPalObj143950 {
    i32 _0[0x53c / 4];
    u8 m_pal[256][4]; // +0x53c
    i32 m_dirty;      // +0x93c
    i32 m_tag;        // +0x940
    i32 SetPalette(const u8* src, i32 tag);
};
SIZE_UNKNOWN(CPalObj143950);

// 0x148af0 - CImageOwned setup (zero the 0x6c-byte transform, fill, Apply, commit).
struct ImgOwnedX {
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
    virtual void Commit(); // slot 10 (+0x28)
    i32 _4[(0x10 - 0x4) / 4];
    Blk6c m_10;                           // +0x10
    i32 Apply(i32 mode, const void* src); // 0x13e0a0
    i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4);
};
SIZE_UNKNOWN(ImgOwnedX);

// 0x148a50 / 0x148c40 - CImageOwned blit-setup variants (stack-local descriptor).
struct ImgOwnedY {
    i32 Apply(i32 mode, const void* src); // 0x13e0a0
    i32 Blit7(i32 a1, i32 a2, i32 a3, i32 a4);
    i32 Blit47(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
};
SIZE_UNKNOWN(ImgOwnedY);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_BOUNDARYUPPER2VIEWS_H
