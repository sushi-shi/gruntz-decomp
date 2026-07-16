// WwdSpatialMgr.h - THE single definition of CWwdSpatialMgr, the per-level
// object-bucket / camera worker the WWD plane owns at plane+0xb0 (CDDrawWorkerHost::
// m_scroll). `new`-size 0xb8 (RebuildPlanes), out-of-line complete dtor 0x163a40.
//
// It was defined TWICE - a full canonical in WwdSpatialMgr.cpp (grids/origins/bbox/
// iterator) and a "CPlaneScroll" scroll-rect view in the plane headers (WwdFile.h,
// then DDrawWorkerHost.h). The two READINGS AGREE on every offset and are unified
// here as the union of both sides' knowledge:
//   * m_mgr @+0x00 + the three per-plane spatial grids @+0x04/+0x08/+0x0c
//     (an object is routed by its flag bits: 0x800000 -> grid1, 0x1000000 -> grid2,
//     else grid0).
//   * the three per-grid world RECTS @+0x10/+0x30/+0x20 (grid0/grid1/grid2 - note
//     the grid1/grid2 rects sit in the "swapped" order retail writes them; the
//     matched InitScrollRects @0x163420 is the ground truth) - these filled what the
//     canonical had as raw padding.
//   * the per-grid scroll ORIGIN pairs @+0x40/+0x48/+0x50 (the ex-view's "centers":
//     InitScrollRects seeds each to its grid's half-extent), the world bbox @+0x58,
//     and the cached scroll position @+0x68 (the ex-view's "target").
//   * the embedded CWwdGridIter cursor @+0x70 (a MEMBER, CWwdGridIter : CObject -
//     the "+0x70 vptr" the ex-C163a40 placeholder mistook for a secondary base).
// Field names are placeholders where the role is unproven; only offsets + emitted
// bytes are load-bearing.
#ifndef GRUNTZ_WWD_WWDSPATIALMGR_H
#define GRUNTZ_WWD_WWDSPATIALMGR_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/WwdGridIter.h> // CWwdGridIter - the embedded cursor member (+0x70)

struct tagRECT; // the Init grid rect (Win32 RECT; completed via <Mfc.h>/<Win32.h> in the .cpp)
typedef struct tagRECT RECT;

class CDDrawChildGroup; // the master object manager (+0x00)
class CWwdGrid;         // one plane's spatial bucket index (each TU supplies its own def)
class CWwdGameObject;   // the engine sprite the grids hold (the canonical managed object)

SIZE(CWwdSpatialMgr, 0xb8); // RebuildPlanes' `operator new(0xb8)`
struct CWwdSpatialMgr {
    CDDrawChildGroup* m_mgr; // +0x00
    CWwdGrid* m_grid0;       // +0x04
    CWwdGrid* m_grid1;       // +0x08
    CWwdGrid* m_grid2;       // +0x0c
    // +0x10..+0x3c: each grid's world rect (seeded to (0,0,w-1,h-1) from the plane
    // geometry's three dimension pairs). grid1's rect is at +0x30 and grid2's at
    // +0x20 - the write order of the matched InitScrollRects.
    i32 m_rect0Left, m_rect0Top, m_rect0Right, m_rect0Bottom; // +0x10  (grid0)
    i32 m_rect2Left, m_rect2Top, m_rect2Right, m_rect2Bottom; // +0x20  (grid2)
    i32 m_rect1Left, m_rect1Top, m_rect1Right, m_rect1Bottom; // +0x30  (grid1)
    i32 m_org0x, m_org0y; // +0x40 / +0x44  grid0 scroll origin (seeded to its centre)
    i32 m_org1x, m_org1y; // +0x48 / +0x4c  grid1 scroll origin
    i32 m_org2x, m_org2y; // +0x50 / +0x54  grid2 scroll origin
    i32 m_bbMinX;         // +0x58
    i32 m_bbMinY;         // +0x5c
    i32 m_bbMaxX;         // +0x60
    i32 m_bbMaxY;         // +0x64
    i32 m_scrollX;        // +0x68  cached scroll position (InitScrollRects parks it at
    i32 m_scrollY;        // +0x6c  -22222 so the first SetTarget always moves)
    CWwdGridIter m_iter;  // +0x70  embedded cursor for the GetFirst/GetNext API
    CWwdGrid* m_curGrid;  // +0xb4  which grid the embedded cursor is walking

    // The out-of-line complete dtor (0x163a40, body in WwdSpatialMgr.cpp): FreeGrids,
    // then the compiler destructs m_iter (the CObject-derived member at +0x70 - the
    // ??_7CObject stamp retail emits there) under the /GX frame. An explicit
    // `p->~CWwdSpatialMgr()` emits that call; ~CDDrawWorkerHost instead reproduces
    // retail's INLINED copy explicitly (see DDrawWorkerHost.cpp).
    ~CWwdSpatialMgr();

    // Bring-up (0x168080, ret 0x20 = 8 args): allocate the three grids and two-phase-
    // construct each (raw CWwdGridShell alloc -> CWwdGrid ctor via Setup), then seed the
    // per-grid rects (0,0,dim-1) + origins (dim/2) from p6/p7/p8, the bbox from `rc`, and
    // park the cached scroll at -22222. `src`->m_mgr, `rc` is the shared grid rect (used in
    // all three Setups + the bbox), p3/p4/p5 are the three Setup size pairs, p6/p7/p8 the
    // three rect/origin dim pairs. Called by CDDrawWorkerHost::RebuildPlanes.
    i32 Init(void* src, RECT* rc, i32* p3, i32* p4, i32* p5, i32* p6, i32* p7, i32* p8); // 0x168080
    i32 SetTargetA(i32 a, i32 b); // 0x168340  (the plane's CenterScrollA target)
    i32 SetTargetB(i32 a, i32 b); // 0x168500  (the plane's CenterScrollB target)
    void FreeGrids();             // 0x1682f0
    void ListDtor();              // 0x163a10  the m_iter member teardown
    i32 ScrollTo(i32 dx, i32 dy);
    i32 GetSize(); // 0x168430
    i32 CountInRect(CWwdGrid* grid);
    i32 Relocate(i32 newX, i32 newY);
    i32 PruneCount();                       // 0x1688b0
    void RemoveObject(CWwdGameObject* obj); // 0x1688f0
    i32 FlushAll();
    i32 FlushGrid(CWwdGrid* grid);
    i32 ForEach(void(__cdecl* cb)(CWwdGameObject*));
    i32 ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CWwdGameObject*));
    CWwdGameObject* GetFirstObject();
    CWwdGameObject* GetNextObject();
};

typedef CWwdSpatialMgr CPlaneScroll; // the ex-WwdFile.h spelling

#endif // GRUNTZ_WWD_WWDSPATIALMGR_H
