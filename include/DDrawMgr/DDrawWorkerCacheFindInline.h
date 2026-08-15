#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawWorkerCache.h>

// Opt-in inline visibility for CDDrawWorkerCache::Find (out of line at 0x9cab0
// in StreamRecordLoaders.cpp) - a surviving per-TU visibility split, same text
// on both sides.  A single visible body is refuted by measurement (2026-08-15):
// cl 5.0 plants the four point-logic ctors' declined nested calls WITHOUT
// emitting the COMDAT, so 0x9cab0 would have no emitter.  The release ASSERT
// (cb 38-40 -> ~44, over the 0x28 exemption) is what makes those ctors decline
// at all - and is why CLightFx's ctor can afford exactly ONE of its three
// sites (retail's mixed shape).  Ledger: docs/patterns/comdat-home-adjudicates-inline-spelling.md.
inline CObject* CDDrawWorkerCache::Find(const char* key) {
    CObject* found = 0;
    ASSERT(key != NULL);
    m_workers.Lookup(key, found);
    return found;
}

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H
