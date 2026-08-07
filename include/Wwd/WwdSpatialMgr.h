#ifndef GRUNTZ_WWD_WWDSPATIALMGR_H
#define GRUNTZ_WWD_WWDSPATIALMGR_H

#include <rva.h>

#include <Gruntz/WwdGridIter.h>
#include <Ints.h>

class CDDrawChildGroup;
class CWwdGrid;
class CWwdGameObject;

struct CWwdSpatialMgr {
    CDDrawChildGroup* m_mgr;
    CWwdGrid* m_grid0;
    CWwdGrid* m_grid1;
    CWwdGrid* m_grid2;

    RECT m_rect0;
    RECT m_rect2;
    RECT m_rect1;
    i32 m_org0x, m_org0y;
    i32 m_org1x, m_org1y;
    i32 m_org2x, m_org2y;
    RECT m_bounds;
    i32 m_scrollX;
    i32 m_scrollY;
    CWwdGridIter m_iter;
    CWwdGrid* m_curGrid;

    CWwdSpatialMgr();
    ~CWwdSpatialMgr();

    i32 Init(
        void* owner,
        RECT* rc,
        i32* cellA,
        i32* cellB,
        i32* cellC,
        i32* sizeA,
        i32* sizeB,
        i32* sizeC
    );
    void FreeGrids();
    i32 ScrollTo(i32 dx, i32 dy);
    i32 GetSize();
    i32 CountInRect(CWwdGrid* grid);
    i32 Relocate(i32 newX, i32 newY);
    i32 PruneCount();
    void RemoveObject(CWwdGameObject* obj);
    i32 FlushAll();
    i32 FlushGrid(CWwdGrid* grid);
    i32 ForEach(void(__cdecl* cb)(CGameObject*));
    i32 ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CGameObject*));
    CGameObject* GetFirstObject();
    CGameObject* GetNextObject();
};
SIZE(0xb8);
SIZE_UNKNOWN();

// Both are inline in retail: RebuildPlanes (0x1628f0) inlines the ctor's five
// NULL stores after `operator new`, and inlines the dtor (FreeGrids + the
// m_iter sub-object dtor, which is what raises its /GX trylevel). The one
// out-of-line COMDAT copy of the dtor is emitted by LevelPlane.cpp for Unload.
inline CWwdSpatialMgr::CWwdSpatialMgr() {
    m_mgr = NULL;
    m_grid0 = NULL;
    m_grid1 = NULL;
    m_grid2 = NULL;
    m_curGrid = NULL;
}

inline CWwdSpatialMgr::~CWwdSpatialMgr() {
    FreeGrids();
}

#endif // GRUNTZ_WWD_WWDSPATIALMGR_H
