#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawWorkerCache.h>

// Opt-in inline visibility for CDDrawWorkerCache::Find (out of line at 0x9cab0 in
// StreamRecordLoaders.cpp).  A TU that includes this header expands the two-argument
// CMapStringToOb::Lookup in place (`add ecx,0x10` + the zeroed out-parameter); a TU
// that does not gets the `call Find` + `test eax,eax` shape.  Retail shows both, and
// which one a site carries is a property of the TU, not of the call.
inline CObject* CDDrawWorkerCache::Find(const char* key) {
    CObject* found = 0;
    m_workers.Lookup(key, found);
    return found;
}

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHEFINDINLINE_H
