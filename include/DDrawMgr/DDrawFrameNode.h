// DDrawFrameNode.h - the worker/frame dispatch views used by DDrawSurfacePair.cpp.
//
// CDDrawWorkerObj: the object Lookup yields, viewed as a bounded element array (m_14
// elements over [m_64, m_68]). @identity-TODO reduced view.
//
// CDDrawFrameNode: the render object CDDrawWorkerB::m_78 holds IS a CImage frame (its
// vtable slots match CImage's 0x1eaa2c ground truth). @identity-TODO fold-deferred:
// folding onto CImage cast-free requires retyping RenderImage to (CResolveNode*,
// CDDrawSurfacePair*) AND every Blit* helper off CBlitInfo->CResolveNode across the whole
// blit subsystem - one coordinated change, not a per-TU cast. Slot names are CImage's own.
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef DDRAWMGR_DDRAWFRAMENODE_H
#define DDRAWMGR_DDRAWFRAMENODE_H

#include <rva.h>
#include <Ints.h>

class CDDrawWorkerB;
class CDDrawSurfacePair;

// The object Lookup yields, viewed as a bounded element array.
struct CDDrawWorkerObj {
    char pad_00[0x14];
    void** m_14; // +0x14  element array
    char pad_18[0x64 - 0x18];
    i32 m_64; // +0x64  lo index
    i32 m_68; // +0x68  hi index
};
SIZE_UNKNOWN(CDDrawWorkerObj);

// The frame node (a CImage dispatch view; vtable 0x1eaa2c ground truth).
struct CDDrawFrameNode {
    virtual void GetRuntimeClass(); // [0]  CObject slot (0x1bef01)
    virtual void ScalarDtor();      // [1]  0x002adb
    virtual void Serialize();       // [2]  CObject slot (0x0028ec)
    virtual void AssertValid();     // [3]  CObject slot (0x00106e)
    virtual void Dump();            // [4]  CObject slot (0x004034)
    virtual void IsLoaded();        // [5]  0x0013b6 (CWapObj default)
    virtual void IsReady();         // [6]  0x001c08 (CWapObj default)
    virtual void FreeAll();         // [7]  0x153260
    virtual void GetClassId();      // [8]  0x0042aa
    virtual void Create24();        // [9]  0x1530e0
    virtual void LoadDispatch();    // [10] 0x152fb0
    virtual void Resolve();         // [11] 0x152f20
    virtual void Create();          // [12] 0x152e90
    virtual void Reload();          // [13] 0x153380
    virtual void RenderImage(CDDrawWorkerB* worker, CDDrawSurfacePair* target); // [14] 0x153470
};
SIZE_UNKNOWN(CDDrawFrameNode);

#endif // DDRAWMGR_DDRAWFRAMENODE_H
