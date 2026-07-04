#include <rva.h>
// CMoviePlayerEh.cpp - the /GX EH-framed destructor of the DDrawMgr movie/FMV
// player decode object (see include/Gruntz/MoviePlayer.h).
// Split off the frameless main cluster (the dtor lives at 0x038fc0, far from the
// 0x17b500..0x17c790 method cluster, so it is its own retail TU). MSVC5 /GX frames
// the member-teardown walk; the embedded store + scratch embed give it the frame.
//
//   0x038fc0  ~CMoviePlayer
#include <Mfc.h> // /GX EH-frame helpers

#include <Gruntz/MoviePlayer.h>

// The scratch embed's own vtable (foreign engine datum, reloc-masked DATA()).
DATA(0x001e971c)
extern void* g_movieScratchVtbl; // 0x5e971c
// The shared base object dtor vtable (also stamped by CDDrawWorker).
DATA(0x001e8cb4)
extern void* g_wapObjectDtorVtbl; // 0x5e8cb4

// The scratch embed (worker+0x868c) teardown: stamp its own vtable, RezFree the
// owned buffer if present, then restore the shared base dtor vtable. Marked inline
// so the worker dtor folds it (the embed is not a standalone retail function).
inline CMovieScratch::~CMovieScratch() {
    m_vptr = &g_movieScratchVtbl;
    if (m_4) {
        RezFree(m_4);
    }
    m_vptr = &g_wapObjectDtorVtbl;
}

// The decode store (worker+0x540) teardown: abort the active decode, then the
// CFile/CByteArray members destruct (reverse declaration order). Inline so the
// worker dtor folds it.
inline CMovieDecodeStore::~CMovieDecodeStore() {
    Abort();
}

// ===========================================================================
// 0x038fc0 - ~CMoviePlayer: run the worker Teardown, then the two embedded
// subobjects destruct in reverse declaration order (the +0x868c scratch embed,
// then the +0x540 decode store). /GX frames the whole walk.
// ===========================================================================
// @early-stop
// ~73.5% (eh-dtor-inline-member-vtable-stamp-thisadjust wall): the teardown logic
// is byte-faithful (Teardown, the embed stamp/RezFree/restore, the store Abort +
// ~CByteArray + ~CFile in the right order at the right offsets), but retail's /GX
// frame caches `&m_868c` in edi and writes the inline-member `this` into a frame
// slot ([esp+0xc]) per subobject, with EH states 1/2/4/3/-1; the manual-stamp
// model destroys the members in place (no edi cache, no member-this re-point,
// states 1/2/3/2/-1). Not steerable without a real polymorphic subobject hierarchy
// that would reshape the ctor and emit divergent vtables. Logic complete; deferred
// to the final whole-class sweep.
// docs/patterns/eh-dtor-inline-member-vtable-stamp-thisadjust.md.
RVA(0x00038fc0, 0xa5)
CMoviePlayer::~CMoviePlayer() {
    Teardown();
    // ~m_868c (scratch embed) then ~m_540 (decode store) fold here.
}
