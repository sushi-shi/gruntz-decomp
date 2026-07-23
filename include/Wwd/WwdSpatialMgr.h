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

    // Bring-up (0x168080, ret 0x20 = 8 args): allocate three CWwdGridShells, call the
    // inherited CWwdGrid::Setup on each, then seed the
    // per-grid rects (0,0,dim-1) + origins (dim/2) from p6/p7/p8, the bbox from `rc`, and
    // park the cached scroll at -22222. `src`->m_mgr, `rc` is the shared grid rect (used in
    // all three Setups + the bbox), p3/p4/p5 are the three Setup size pairs, p6/p7/p8 the
    // three rect/origin dim pairs. Called by CDDrawWorkerHost::RebuildPlanes.
    i32 Init(void* src, RECT* rc, i32* p3, i32* p4, i32* p5, i32* p6, i32* p7, i32* p8); // 0x168080
    void FreeGrids();                                                                    // 0x1682f0
    i32 ScrollTo(i32 dx, i32 dy);
    i32 GetSize(); // 0x168430
    i32 CountInRect(CWwdGrid* grid);
    i32 Relocate(i32 newX, i32 newY);
    i32 PruneCount();                       // 0x1688b0
    void RemoveObject(CWwdGameObject* obj); // 0x1688f0
    i32 FlushAll();
    i32 FlushGrid(CWwdGrid* grid);
    i32 ForEach(void(__cdecl* cb)(CGameObject*));
    i32 ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CGameObject*));
    CGameObject* GetFirstObject();
    CGameObject* GetNextObject();
};
SIZE(0xb8); // RebuildPlanes' `operator new(0xb8)`
SIZE_UNKNOWN();

#endif // GRUNTZ_WWD_WWDSPATIALMGR_H
