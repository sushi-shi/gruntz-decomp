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
#include <Wap32/Object.h> // Wap::CObject - the real grand-base these placeholders modelled
#include <Gruntz/State.h> // real CState (the state base @0x5ea21c)

// 0x039f20 - ~CWorker39f20 (/GX): derived vtable stamp, RezFree the +0x04 heap
// buffer, then fold the CObject base subobject. Byte-shape == ~CRezBufferObject.
struct CWorker39f20
    : Wap::CObject { // was : WorkerBase39f20 (fake base view; folded to real Wap::CObject)
    char* m_4;       // +0x04  heap buffer
    ~CWorker39f20() OVERRIDE;
};
SIZE_UNKNOWN(CWorker39f20);

// 0x08c400 - /GX dtor: derived vtable stamp, run the +0x00 teardown (0x1c6a5c ==
// CImageList::DeleteImageList, MFC) -> owns an MFC CImageList; then fold the CObject base.
struct CHolder8c400
    : Wap::CObject {       // was : WorkerBase8c400 (fake base view; folded to real Wap::CObject)
    void Teardown1c6a5c(); // 0x1c6a5c
    ~CHolder8c400() OVERRIDE;
};
SIZE_UNKNOWN(CHolder8c400);

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
    ~CMenuState8d000() OVERRIDE;
};
SIZE_UNKNOWN(CMenuState8d000);

// 0x021310 / 0x021570 - CButeStore-family dtors (/GX, multiple inheritance): stamp both
// base vtables, run the body teardown (0x16e070 == CButeStore::ClearRecursive), then fold
// the +0x08 second base (dtor 0x16dfc0, MI this-adjust null guard) and the +0x00 first base
// (dtor 0x16da60 == ~CContainerErr; its vtable 0x5e94ac == ??_7zPTree, RTTI-confirmed). Two
// distinct derived classes share the base vtables (kept standalone to avoid dup base vtables).
struct CButeBase1_21 {
    ~CButeBase1_21();           // +0x00 vptr (0x5e94ac), dtor 0x16da60
    void Teardown16e070(i32 z); // 0x16e070
    i32 m_4;                    // +0x04 (pads first base to 8 so the second base lands at +0x08)
};
SIZE_UNKNOWN(CButeBase1_21);
struct CButeBase2_21 {
    ~CButeBase2_21(); // +0x08 vptr (0x5e949c), dtor 0x16dfc0
};
SIZE_UNKNOWN(CButeBase2_21);
struct CButeTree21a : CButeBase1_21, CButeBase2_21 {
    ~CButeTree21a();
};
SIZE_UNKNOWN(CButeTree21a);
struct CButeTree21b : CButeBase1_21, CButeBase2_21 {
    ~CButeTree21b() OVERRIDE;
};
SIZE_UNKNOWN(CButeTree21b);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_BOUNDARYLOWERDTORSVIEWS_H
