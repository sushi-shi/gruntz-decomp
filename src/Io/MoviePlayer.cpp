#include <rva.h>
#include <Io/MoviePlayer.h>
#include <Mfc.h>             // /GX EH-frame helpers
#include <Crypto/FecCrypt.h> // the +0x540 decode store IS a CFecFile (Init/ReadArchive/Lookup/Close)

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

RVA(0x000390a0, 0x5d)
inline CFecFile::~CFecFile() {
    Close();
}
