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

// (0x133370 DICfgC::DtorC re-homed to src/DinMgr2/DirectInputMgr2.cpp next to
// CInputDevRoot; the DICfgC view is dissolved.)

// (0x1396f0 CParseSource::Init re-homed to src/Gruntz/ParseSource.cpp; the HashNode1396f0
// + CParseSource views dissolved onto the real CParseSource (ParseSource.h), which now
// models the embedded hash-node + self-link and carries VTBL(HashNode1396f0, 0x1ef740).)

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

// (0x13e7d0 CDDSurface::Restore re-homed to src/DDrawMgr/DirectDrawMgr.cpp; the
// RestoreDesc/CDDSurface views dissolved onto the real CDDSurface (DDSurface.h) +
// the real DDBLTFX.)

// (CDDPalette / CDDrawPtrCollections factory views dissolved: Create @0x143040
// re-homed to src/DDrawMgr/DDrawPtrCollections.cpp onto the canonical classes.)

// (0x1509c0 CWwdGameObject::Test re-homed to src/Wwd/WwdGameObject.cpp onto the real
// CWwdGameObject; the WwdExtent/WwdCamRect/WwdCamHolder/WwdGridLim/WwdGridHolder/WwdCtx/
// CWwdGameObject views dissolved - the 0x1f0020 vtable is bound by the real
// VTBL(CWwdGameObjectE) in WwdGameObject.cpp, so the RELOC_VTBL alias here is redundant.)

// (0x13a530 CSymTab::AddNodeSubEntry re-homed to src/Bute/SymTab.cpp; the CHashBase/
// SymEntry1/SymEntry2/SymList18/CSymTab views dissolved onto the canonical CSymTab/
// CSymRec/CHashTable/CSymParser.)

// 0x17e230 - destroy a by-value CDataBuffer-like parameter.
struct DBuf17e230 {
    void* p;
    ~DBuf17e230(); // 0x1b9cde
};
SIZE_UNKNOWN(DBuf17e230);

// (0x143950 CDDrawPtrCollections::Make950 re-homed to DDrawPtrCollections.cpp; the
// CPalObj143950 view dissolved onto the real CDDrawPtrCollections palette fields.)

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
