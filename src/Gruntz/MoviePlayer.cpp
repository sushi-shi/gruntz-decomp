#include <rva.h>
// CMoviePlayer.cpp - the frameless slice of the DDrawMgr "DDraw worker"
// movie/stream decode object (placeholder name; see include/Gruntz/MoviePlayer.h).
// One reconstructed method from the 0x17b500..0x17c790 cluster:
//   0x17c6f0  Open - orchestrate a decode open through the +0x540 store + OpenHi.
// The store/worker sibling methods (Begin/OpenA/Abort/OpenB/OpenHi) live elsewhere
// in the cluster; they are declared (no body) so their rel32 calls reloc-mask.

#include <Gruntz/MoviePlayer.h>

// ===========================================================================
// 0x17c6f0 - Open: bail if the worker is inactive (m_4 == 0). Prepare the +0x540
// decode store (Begin); open the low-res source (OpenA(a1)); open the high-res
// source (OpenB(a2)); finalize through OpenHi with the OpenB handle + the four
// trailing args. Any failing step aborts the store and returns 0; full success
// returns 1.
// ===========================================================================
RVA(0x0017c6f0, 0x9c)
i32 CMoviePlayer::Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    if (m_4 == 0) {
        return 0;
    }
    if (!m_540.Begin()) {
        return 0;
    }
    if (!m_540.OpenA(a1)) {
        m_540.Abort();
        return 0;
    }
    i32 hi = m_540.OpenB(a2);
    if (!hi) {
        m_540.Abort();
        return 0;
    }
    if (!OpenHi(hi, a3, a4, a5, a6)) {
        m_540.Abort();
        return 0;
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CMovieScratch);
SIZE_UNKNOWN(CMovieDecodeStore);
SIZE_UNKNOWN(CMoviePlayer);
SIZE_UNKNOWN(CMovieByteArray);
SIZE_UNKNOWN(CMovieFile);
