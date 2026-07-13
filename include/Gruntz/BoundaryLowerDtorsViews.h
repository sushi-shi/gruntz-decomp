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

// (0x0390a0 - identity RECOVERED 2026-07-13: the former `CCredits390a0` placeholder
// IS ??1CFecFile@@QAE@XZ - the canonical CFecFile's destructor (Close @0x17b570 +
// ~CDWordArray m_index @+0x138 + ~CFile m_stream @+0x124), defined in
// src/Io/MoviePlayer.cpp (same retail TU as ~CMoviePlayer @0x38fc0, which is why /O2
// also inlines the body there). The old "retail CFile is 0x14 B" pad theory was a
// mis-read of CFecFile::m_134, the write-path entry counter - toolchain sizeof(CFile)
// == 0x10 is byte-proven (Lookup returns m_stream.m_hFile @+0x128 at 100%%). The
// CDWordArray-not-CByteArray band proof (`mfc_class 0x1b4b76`) lives on in
// <Crypto/FecCrypt.h>. View dissolved.)

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
