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
// Its OWN vtable, binary-proven: ??_7CHolder8c400 @0x1ea2a4 slot 1 -> ILT thunk 0x373d ->
// sdd 0x8c3d0 -> this dtor (0x8c400). NOT an alias of CImgHolder's 0x1e8cd4 (see the
// Dialogs.cpp note): three distinct image-list-owning classes, one vtable each.
VTBL(CHolder8c400, 0x001ea2a4);

// 0x0390a0 - /GX dtor: explicit cleanup (0x17b570 == CPageStore17b510::Close), then fold the
// two owned members at +0x138 (dtor 0x1b4b76 == ~CByteArray, MFC) and +0x124 (dtor 0x1bf121 ==
// ~CFile, MFC) in reverse. Owns an MFC CFile (+0x124) + CByteArray (+0x138); the "CCredits"
// class name is unconfirmed (a file/page loader).
struct CCredits390a0 {
    char pad4[0x124];  // +0x00 .. +0x123
    CFile m_124;       // +0x124  real MFC CFile (dtor ??1CFile@@UAE@XZ @0x1bf121)
    char m_124tail[4]; // +0x134  retail's CFile is 0x14 B (BOOL m_bCloseOnDelete); the
                       //         toolchain's is 0x10 B (BYTE) - pad to hold m_138 @+0x138
    CByteArray m_138;  // +0x138  real MFC CByteArray (dtor ??1CByteArray@@UAE@XZ @0x1b4b76)
    ~CCredits390a0();
};
SIZE_UNKNOWN(CCredits390a0);

// 0x08d000 - CSplashState::~CSplashState (/GX): the out-of-line splash-state dtor.
// Identity recovered (its derived vtable @0x1e9d74 IS ??_7CSplashState); modeled as
// the real class via <Gruntz/SplashState.h>, dtor body in HelpState.cpp. (Was the
// CMenuState8d000 placeholder here.)

// 0x021310 / 0x021570 - the out-of-line /GX destructors of zPTree and CBSecStream
// (both byte-identical to CButeStore::~CButeStore @0x174d70; RTTI-proven distinct
// classes conflated onto CButeStore's base vtables). Re-modeled as real CButeStore-
// derived classes in <Bute/ButeStore.h>; dtor bodies in src/Bute/ButeMgr.cpp. The old
// fabricated CButeBase1_21/CButeBase2_21 fake-base view (+ its RELOC_VTBL placeholders)
// is deleted (matcher R54).

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_BOUNDARYLOWERDTORSVIEWS_H
