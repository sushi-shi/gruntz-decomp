#include <rva.h>
#include <Io/MoviePlayer.h>
#include <Mfc.h>             // /GX EH-frame helpers
#include <Crypto/FecCrypt.h> // the +0x540 decode store IS a CFecFile (Init/ReadArchive/Lookup/Close)
#include <Gruntz/BoundaryLowerDtorsViews.h> // CCredits390a0 (the 0x390a0 /GX leaf dtor)
// MoviePlayer.cpp - the frameless slice of the DDrawMgr "DDraw worker"
// movie/stream decode object (placeholder name; see include/Io/MoviePlayer.h).
// One reconstructed method from the 0x17b500..0x17c790 cluster:
//   0x17c6f0  Open - orchestrate a decode open through the +0x540 store + OpenHi.
// The store/worker sibling methods (Begin/OpenA/Abort/OpenB/OpenHi) live elsewhere
// in the cluster; they are declared (no body) so their rel32 calls reloc-mask.

// ===========================================================================
// 0x17c6f0 - Open: bail if the worker is inactive (m_active == 0). Prepare the +0x540
// decode store (Begin); open the low-res source (OpenA(a1)); open the high-res
// source (OpenB(a2)); finalize through OpenHi with the OpenB handle + the four
// trailing args. Any failing step aborts the store and returns 0; full success
// returns 1.
// ===========================================================================
RVA(0x0017c6f0, 0x9c)
i32 CMoviePlayer::Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    if (m_active == 0) {
        return 0;
    }
    if (!((CFecFile*)&m_540)->Init()) {
        return 0;
    }
    if (!((CFecFile*)&m_540)->ReadArchive((const char*)a1)) {
        ((CFecFile*)&m_540)->Close();
        return 0;
    }
    i32 hi = ((CFecFile*)&m_540)->Lookup((unsigned int)a2);
    if (!hi) {
        ((CFecFile*)&m_540)->Close();
        return 0;
    }
    if (!OpenHi(hi, a3, a4, a5, a6)) {
        ((CFecFile*)&m_540)->Close();
        return 0;
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CMovieDecodeStore);
SIZE_UNKNOWN(CMoviePlayer);
SIZE_UNKNOWN(CMovieFile);

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

// The decode store (worker+0x540) teardown: abort the active decode, then the
// CFile/CByteArray members destruct (reverse declaration order). Inline so the
// worker dtor folds it.
inline CMovieDecodeStore::~CMovieDecodeStore() {
    ((CFecFile*)this)->Close();
}

// ===========================================================================
// 0x038fc0 - ~CMoviePlayer: run the worker Teardown, then the two embedded
// subobjects destruct in reverse declaration order (the +0x868c scratch embed,
// then the +0x540 decode store). /GX frames the whole walk.
// ===========================================================================
// @early-stop
// eh-dtor-inline-member-vtable-stamp-thisadjust wall: the teardown logic is byte-faithful
// (Teardown, the embed stamp/RezFree/restore, the store Abort + ~CByteArray + ~CFile in
// the right order at the right offsets), but retail's /GX frame caches `&m_868c` in edi
// and writes the inline-member `this` into a frame slot ([esp+0xc]) per subobject, with EH
// states 1/2/4/3/-1; our model destroys the members in place (no edi cache, no member-this
// re-point). docs/patterns/eh-dtor-inline-member-vtable-stamp-thisadjust.md.
// CLEAN-ROOM % NOTE: dissolving the local CPageStore17b510/CFecFile views onto the unified
// CFecFile (the +0x540 store's real class) shifted this /GX dtor's EH-state numbering
// (~73.5% -> ~55.6%). Accepted per the model-the-class mandate: the store Abort now calls
// the canonical CFecFile::Close; the residual is still the frame-slot/EH-state shape.
RVA(0x00038fc0, 0xa5)
CMoviePlayer::~CMoviePlayer() {
    Teardown();
    // ~m_868c (scratch embed) then ~m_540 (decode store) fold here.
}

// ---------------------------------------------------------------------------
// 0x0390a0 - ~CCredits390a0 (/GX): run the explicit cleanup (0x17b570 ==
// CFecFile::Close), then let the two owned members at +0x138 (~CByteArray) and +0x124
// (~CFile) fold in reverse declaration order. __thiscall. RVA-homed here (RVA-contiguous
// with ~CMoviePlayer @0x38fc0; a page/file store that owns an MFC CFile+CByteArray).
// @identity-TODO: the "CCredits" class name is unconfirmed (a file/page loader whose
// state owner is unrecovered); its cleanup casts through the canonical CFecFile.
RVA(0x000390a0, 0x5d)
CCredits390a0::~CCredits390a0() {
    ((CFecFile*)this)->Close();
}
