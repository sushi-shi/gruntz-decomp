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

// (0x039f20 was the `CWorker39f20` placeholder - RTTI-refuted: its vtable 0x1e971c
// carries the COL .?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@, so the body is the MFC
// playlist-array template dtor, now instantiated + pinned in ArraySerialize.cpp.
// 0x08c400 was the `CHolder8c400` placeholder - RTTI-refuted the same way: its vtable
// 0x1ea2a4 carries .?AVCRgn@@, so the body is ??1CRgn@@UAE@XZ, now emitted + pinned in
// CreditsState.cpp. Both fake views are dissolved; the "CImageList::DeleteImageList"
// teardown story was the FID AMBIG GDI/ImageList twin - 0x1c6a5c is
// CGdiObject::DeleteObject.)

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
