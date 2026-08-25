#ifndef GRUNTZ_WWD_WWDSPATIALMGR_H
#define GRUNTZ_WWD_WWDSPATIALMGR_H

#include <rva.h>

#include <Gruntz/WwdGridIter.h>
#include <Ints.h>

class CDDrawChildGroup;
class CWwdGrid;
class CWwdGameObject;

struct CWwdSpatialMgr {
    CDDrawChildGroup* m_activeGroup;
    CWwdGrid* m_defaultRegionGrid;
    CWwdGrid* m_largeRegionGrid;
    CWwdGrid* m_smallRegionGrid;

    RECT m_defaultRegionRect;
    RECT m_smallRegionRect;
    RECT m_largeRegionRect;
    i32 m_defaultRegionHalfWidth, m_defaultRegionHalfHeight;
    i32 m_largeRegionHalfWidth, m_largeRegionHalfHeight;
    i32 m_smallRegionHalfWidth, m_smallRegionHalfHeight;
    RECT m_levelBounds;
    i32 m_activeCenterX;
    i32 m_activeCenterY;
    CWwdGridIter m_iter;
    CWwdGrid* m_iterationGrid;

    CWwdSpatialMgr();
    ~CWwdSpatialMgr();

    i32 Init(
        CDDrawChildGroup* owner,
        RECT* levelBounds,
        i32* defaultGridCellSize,
        i32* largeGridCellSize,
        i32* smallGridCellSize,
        i32* defaultRegionSize,
        i32* largeRegionSize,
        i32* smallRegionSize
    );
    void FreeGrids();
    i32 ActivateAt(i32 centerX, i32 centerY);
    i32 ActivateKeepActiveObjects();
    i32 ActivateKeepActiveFromGrid(CWwdGrid* grid);
    i32 DeactivateOutside(i32 centerX, i32 centerY);
    i32 PruneCount();
    void ParkObject(CWwdGameObject* obj);
    i32 FlushAll();
    i32 FlushGrid(CWwdGrid* grid);
    i32 ForEach(void(__cdecl* cb)(CGameObject*));
    i32 ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CGameObject*));
    CGameObject* GetFirstObject();
    CGameObject* GetNextObject();
};

// Both are inline in retail: RebuildPlanes (0x1628f0) inlines the ctor's five
// NULL stores after `operator new`, and inlines the dtor (FreeGrids + the
// m_iter sub-object dtor, which is what raises its /GX trylevel). The one
// out-of-line COMDAT copy of the dtor is emitted by LevelPlane.cpp for Unload.
inline CWwdSpatialMgr::CWwdSpatialMgr() {
    m_activeGroup = NULL;
    m_defaultRegionGrid = NULL;
    m_largeRegionGrid = NULL;
    m_smallRegionGrid = NULL;
    m_iterationGrid = NULL;
}

inline CWwdSpatialMgr::~CWwdSpatialMgr() {
    FreeGrids();
}

#endif // GRUNTZ_WWD_WWDSPATIALMGR_H
