#include <rva.h>
#include <Io/MoviePlayer.h>
#include <Mfc.h>             // /GX EH-frame helpers
#include <Crypto/FecCrypt.h> // the +0x540 decode store IS a CFecFile (Init/ReadArchive/Lookup/Close)

// MoviePlayer.cpp - the frameless slice of the DDrawMgr "DDraw worker"
// movie/stream decode object (placeholder name; see include/Io/MoviePlayer.h).
// One reconstructed method from the 0x17b500..0x17c790 cluster:
//   0x17c6f0  Open - orchestrate a decode open through the +0x540 store + OpenHi.
// The store/worker sibling methods (Begin/OpenA/Abort/OpenB/OpenHi) live elsewhere
// in the cluster; they are declared (no body) so their rel32 calls reloc-mask.

// ===========================================================================
// 0x17c6f0 - Open: bail if the worker is inactive (m_initialized == 0). Prepare the +0x540
// decode store (Begin); open the low-res source (OpenA(a1)); open the high-res
// source (OpenB(a2)); finalize through OpenHi with the OpenB handle + the four
// trailing args. Any failing step aborts the store and returns 0; full success
// returns 1.
// ===========================================================================
RVA(0x0017c6f0, 0x9c)
i32 CMoviePlayer::Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    if (m_initialized == 0) {
        return 0;
    }
    if (!m_540.Init()) {
        return 0;
    }
    if (!m_540.ReadArchive((const char*)a1)) {
        m_540.Close();
        return 0;
    }
    i32 hi = m_540.Lookup((unsigned int)a2);
    if (!hi) {
        m_540.Close();
        return 0;
    }
    if (!OpenHi(hi, a3, a4, a5, a6)) {
        m_540.Close();
        return 0;
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CMoviePlayer);

// ============================================================================
// merged from MoviePlayerEh.cpp (the /GX EH-frame sibling; unit flags -> eh)
// ============================================================================
// MoviePlayerEh.cpp - the /GX EH-framed destructor of the DDrawMgr movie/FMV
// player decode object (see include/Io/MoviePlayer.h).
// Split off the frameless main cluster (the dtor lives at 0x038fc0, far from the
// 0x17b500..0x17c790 method cluster, so it is its own retail TU). MSVC5 /GX frames
// the member-teardown walk; the embedded store + scratch embed give it the frame.
//
//   0x038fc0  ~CMoviePlayer

// The playlist embed (worker+0x868c) is the REAL MFC CArray<PLAYLISTINFOSTRUCT*>
// (header typedef CMoviePlaylist; RTTI at 0x1e971c). Its inline afxtempl dtor
// (`if (m_pData != NULL) { DestructElements (no-op for T*); delete[] (BYTE*)m_pData; }`)
// folds into ~CMoviePlayer below exactly as the former hand-rolled CMovieScratch
// teardown did: the delete[] lowers to a call to ??3@YAXPAX@Z @0x1b9b82 - the
// SAME rva the old `RezFree(m_pData)` reloc bound (FID names 0x1b9b82 both
// ??3@YAXPAX@Z and _RezFree: the game's Rez free IS operator delete). The vtable
// 0x1e971c auto-names ??_7?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@6B@ from
// config/vtable_names.csv when the instantiating objs emit the ??_7 COMDAT; the
// old VTBL(CMovieScratch, 0x001e971c) fabrication is gone.

// ===========================================================================
// 0x038fc0 - ~CMoviePlayer: run the worker Teardown, then the two embedded
// subobjects destruct in reverse declaration order (the +0x868c playlist embed,
// then the +0x540 CFecFile decode store - whose inline ~CFecFile below /O2-folds
// in: Close + ~CDWordArray + ~CFile with EH states 1/2/4/3/-1). /GX frames the walk.
// (The former "eh-dtor-inline-member-vtable-stamp-thisadjust wall" @early-stop
// (~55.6-73.5%) was a MISDIAGNOSED FAKE VIEW: with m_540 typed as the real CFecFile
// and its dtor defined `inline` in this TU, the edi cache, the member-this frame
// slots and the EH-state numbering all fall out - 100.00 EXACT, 2026-07-13.)
// ===========================================================================
RVA(0x00038fc0, 0xa5)
CMoviePlayer::~CMoviePlayer() {
    Teardown();
    // ~m_868c (playlist embed) then ~m_540 (CFecFile decode store) fold here.
}

// ---------------------------------------------------------------------------
// 0x0390a0 - ~CFecFile (/GX): the decode store's real destructor - the explicit
// cleanup (Close @0x17b570), then the two owned members fold in reverse declaration
// order: ~CDWordArray m_index (+0x138, 0x1b4b76) and ~CFile m_stream (+0x124,
// 0x1bf121). __thiscall; RVA-contiguous with ~CMoviePlayer @0x38fc0 (same retail TU,
// which is why /O2 also INLINES this body into 0x38fc0's m_540 teardown).
// (Identity recovered 2026-07-13: the former "CCredits390a0" placeholder - and the
// CMovieDecodeStore/CMovieFile views - were all this one class; the old note's
// "retail CFile is 0x14 B" was a mis-read of CFecFile::m_134, the entry counter.)
RVA(0x000390a0, 0x5d)
inline CFecFile::~CFecFile() {
    Close();
}
