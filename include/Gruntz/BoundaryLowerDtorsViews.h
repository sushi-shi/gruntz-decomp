// BoundaryLowerDtorsViews.h - the placeholder /GX destructor classes reconstructed in
// BoundaryLowerDtors.cpp (lower-half engine_boundary backlog, RVA < 0x133370).
//
// Each is a real C++ destructor whose destructible base/member subobjects force the
// synchronous-EH (/GX) frame the retail compiler emits. RTTI cannot attribute the
// COMDAT-folded methods, so the owning class names are placeholders; only OFFSETS +
// code bytes are load-bearing. Vtable stamps + member-dtor callees reloc-mask.
// Formerly declared inline per-TU; consolidating the class declarations here is pure
// code motion (matching-neutral). The dtor bodies (each carrying its RVA()) stay in
// BoundaryLowerDtors.cpp.
#ifndef GRUNTZ_BOUNDARYLOWERDTORSVIEWS_H
#define GRUNTZ_BOUNDARYLOWERDTORSVIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h> // CObject - the real grand-base these placeholders modelled
#include <Gruntz/State.h> // real CState (the state base @0x5ea21c)

// 0x039f20 - ~CWorker39f20 (/GX): derived vtable stamp, RezFree the +0x04 heap
// buffer, then fold the CObject base subobject. Byte-shape == ~CRezBufferObject.
struct CWorker39f20 : CObject { // was : WorkerBase39f20 (fake base view; folded to real CObject)
    char* m_4;                  // +0x04  heap buffer
    virtual ~CWorker39f20() OVERRIDE;
};
SIZE_UNKNOWN(CWorker39f20);
RELOC_VTBL(CWorker39f20, 0x001e971c); // vtable reloc-masks a bound datum (dtor-stamp verified)

// 0x08c400 - /GX dtor: derived vtable stamp, run the +0x00 teardown (0x1c6a5c ==
// CImageList::DeleteImageList, MFC) -> owns an MFC CImageList; then fold the CObject base.
struct CHolder8c400 : CObject { // was : WorkerBase8c400 (fake base view; folded to real CObject)
    void Teardown1c6a5c();      // 0x1c6a5c
    virtual ~CHolder8c400() OVERRIDE;
};
SIZE_UNKNOWN(CHolder8c400);
RELOC_VTBL(CHolder8c400, 0x001e8cd4); // vtable reloc-masks a bound datum (dtor-stamp verified)

// 0x0390a0 - /GX dtor: explicit cleanup (0x17b570 == CPageStore17b510::Close), then fold the
// two owned members at +0x138 (dtor 0x1b4b76 == ~CByteArray, MFC) and +0x124 (dtor 0x1bf121 ==
// ~CFile, MFC) in reverse. Owns an MFC CFile (+0x124) + CByteArray (+0x138); the "CCredits"
// class name is unconfirmed (a file/page loader).
struct Member124_390a0 {
    char pad0[0x14];    // 0x124..0x137 (size 0x14)
    ~Member124_390a0(); // 0x1bf121
};
SIZE_UNKNOWN(Member124_390a0);
struct Member138_390a0 {
    char pad0[0x14];
    ~Member138_390a0(); // 0x1b4b76
};
SIZE_UNKNOWN(Member138_390a0);
struct CCredits390a0 {
    char pad4[0x124];      // +0x00 .. +0x123
    Member124_390a0 m_124; // +0x124
    Member138_390a0 m_138; // +0x138
    ~CCredits390a0();
};
SIZE_UNKNOWN(CCredits390a0);

// 0x08d000 - ~CMenuState8d000 (/GX): derived vtable stamp, teardown body (0x2919),
// then fold the CState base subobject (base dtor 0x3f53).
struct CMenuState8d000
    : CState { // was : CStateBase8d000 (fake base view; folded to real CState @0x5ea21c)
    virtual ~CMenuState8d000() OVERRIDE;
};
SIZE_UNKNOWN(CMenuState8d000);
RELOC_VTBL(CMenuState8d000, 0x001ea21c); // aliases CState (dtor-stamp verified)

// 0x021310 / 0x021570 - the out-of-line /GX destructors of TWO distinct classes that each
// multiply-inherit the same two REAL bases (identities RTTI-confirmed; only the DERIVED
// class name is unrecovered - each dtor is out-of-line with no Complete-Object-Locator):
//   +0x00 first base  = CContainerErr (dtor 0x16da60; vtable 0x5e94ac == ??_7zPTree);
//                       it owns the recursive-clear body method 0x16e070 (the store root
//                       lives in this +0 subobject, so ClearRecursive runs on `this`+0).
//   +0x08 second base = the CButeStore vtable subobject (dtor 0x16dfc0; vtable 0x5e949c).
// The dtor stamps both base vtables, runs ClearRecursive(0), then folds the +0x08 base
// (MI this-adjust null guard) and the +0x00 base. Modeled with the real bases so no cast
// of `this` is needed; kept standalone (not one class) to avoid duplicating the base vtables.
struct CButeBase1_21 {                    // == CContainerErr subobject (+0x00)
    virtual ~CButeBase1_21();             // +0x00 vptr (0x5e94ac), dtor 0x16da60 (~CContainerErr)
    void ClearRecursive(void* node);      // 0x16e070 (== CButeStore::ClearRecursive)
    i32 m_4; // +0x04 (pads the first base to 8 so the second base lands at +0x08)
};
SIZE_UNKNOWN(CButeBase1_21);
struct CButeBase2_21 {        // == CButeStore vtable subobject (+0x08)
    virtual ~CButeBase2_21(); // +0x08 vptr (0x5e949c), dtor 0x16dfc0
};
SIZE_UNKNOWN(CButeBase2_21);
struct CButeTree21a : CButeBase1_21, CButeBase2_21 {
    virtual ~CButeTree21a() OVERRIDE;
};
SIZE_UNKNOWN(CButeTree21a);
RELOC_VTBL(CButeTree21a, 0x001e949c); // aliases CButeStore (dtor-stamp verified)
struct CButeTree21b : CButeBase1_21, CButeBase2_21 {
    virtual ~CButeTree21b() OVERRIDE;
};
SIZE_UNKNOWN(CButeTree21b);
RELOC_VTBL(CButeTree21b, 0x001e949c); // aliases CButeStore (dtor-stamp verified)

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_BOUNDARYLOWERDTORSVIEWS_H
