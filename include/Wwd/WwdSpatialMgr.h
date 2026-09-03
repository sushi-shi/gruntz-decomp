#ifndef GRUNTZ_WWD_WWDSPATIALMGR_H
#define GRUNTZ_WWD_WWDSPATIALMGR_H

#include <rva.h>

#include <Gruntz/WwdGridIter.h>
#include <Gruntz/CoordNode.h>
#include <Ints.h>
#include <Wwd/WwdGameObjectFlags.h>

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
    SIZE
    m_defaultRegionHalfSize;
    SIZE
    m_largeRegionHalfSize;
    SIZE
    m_smallRegionHalfSize;
    RECT m_levelBounds;
    Coord m_activeCenter;
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
    void SetActiveCenter(i32 x, i32 y) {
        m_activeCenter.Set(x, y);
    }
    i32 ActivateAt(i32 centerX, i32 centerY);
    i32 ActivateKeepActiveObjects();
    i32 ActivateKeepActiveFromGrid(CWwdGrid* grid);
    i32 DeactivateRegionObject(
        CWwdGrid* grid,
        POSITION pos,
        CWwdGameObject* obj,
        WwdRegion* region,
        WwdGameObjectFlags flags
    );
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
