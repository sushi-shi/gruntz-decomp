// DDrawFrameNode.h - the worker/frame dispatch views used by DDrawSurfacePair.cpp.
//
// CDDrawWorkerObj: the object Lookup yields, viewed as a bounded element array (m_14
// elements over [m_64, m_68]). @identity-TODO reduced view.
//
// (CDDrawFrameNode DISSOLVED: the render object IS a CImage - RenderImage now typed
// (CResolveNode*, CDDrawSurfacePair*) on the real class; CBlitInfo unified onto
// CResolveNode.)
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef DDRAWMGR_DDRAWFRAMENODE_H
#define DDRAWMGR_DDRAWFRAMENODE_H

#include <rva.h>
#include <Ints.h>

class CDDrawWorkerB;
class CDDrawSurfacePair;

struct CDDrawWorkerObj {
    char pad_00[0x14];
    void** m_14; // +0x14  element array
    char pad_18[0x64 - 0x18];
    i32 m_64; // +0x64  lo index
    i32 m_68; // +0x68  hi index
};
SIZE_UNKNOWN();

#endif // DDRAWMGR_DDRAWFRAMENODE_H
