#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawWorkerCache.h>

// Opt-in inline visibility for CDDrawWorkerCache::Find (out of line at 0x9cab0
// in StreamRecordLoaders.cpp), same text on both sides.  This is a WORKAROUND
// for caller-side modelling error, not a proven era structure - no dev writes a
// per-TU visibility header.  Retested 2026-08-22 with a single in-class body
// and both other entities removed:
//   * the 2026-08-15 "no emitter" justification is FALSE - guardpoint.obj
//     emitted 0x9cab0 and it stayed 100.00 EXACT, so cl 5.0 DOES emit the
//     COMDAT from a TU that declined the expansion;
//   * what the split really buys is keeping lightfx.cpp from SEEING the body:
//     with visibility, CLightFx::CLightFx collapses 94.87 -> 0.00 because cl
//     expands a different number of its three Find tests than retail did.
// Retail's own lightfx DOES expand one of the three (the greedy per-function
// budget running out mid-body), so era source had visibility there and this
// header is the wrong shape for that TU - it just scores better than the
// alternative while the ctor's cb differs from retail's.
// REMOVAL CONDITION: model CLightFx::CLightFx accurately enough that its cb
// makes cl decline the 2nd and 3rd sites on its own; then one visible body
// reproduces the mixed shape and all three entities collapse to one.
// The release ASSERT (cb 38-40 -> ~44, over the /Ob1 0x28 exemption) is what
// makes declining possible at all.
// Ledger: docs/patterns/comdat-home-adjudicates-inline-spelling.md.
inline CObject* CDDrawWorkerCache::Find(const char* key) {
    CObject* found = NULL;
    ASSERT(key != NULL);
    m_workers.Lookup(key, found);
    return found;
}

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H
