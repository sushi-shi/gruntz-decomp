#include <rva.h>

#include <Wwd/WwdSpatialMgr.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/WwdGameObject.h>
#include <Gruntz/WwdGrid.h>
#include <Gruntz/WwdGridIter.h>
#include <Wap32/Object.h>
#include <Wwd/LogicRecordEvent.h>

inline i32 CWwdSpatialMgr::DeactivateRegionObject(
    CWwdGrid* grid,
    POSITION pos,
    CWwdGameObject* obj,
    WwdRegion* region,
    WwdGameObjectFlags flags
) {
    if (HAS(flags, WWD_GAME_OBJECT_FLAG_DELETE_ON_DEACTIVATE)) {
        if (HAS(flags, WWD_GAME_OBJECT_FLAG_DISPATCH_OBJECT_REMOVED)) {
            CLogicRecord* record = obj->m_logicRecord;
            record->SetLogicEvent(ACT_OBJECT_REMOVED);
            record->m_dispatch(obj);
        }
        m_activeGroup->RemoveAll(pos, obj);
        delete obj;
    } else {
        if (HAS(flags, WWD_GAME_OBJECT_FLAG_DISPATCH_LEAVE_ACTIVE_REGION)) {
            CLogicRecord* record = obj->m_logicRecord;
            i32 saved = record->EventCode();
            record->SetLogicEvent(ACT_LEAVE_ACTIVE_REGION);
            record->m_dispatch(obj);
            if (record->LogicEvent() == ACT_LEAVE_ACTIVE_REGION) {
                record->SetEventCode(saved);
            }
        }
        m_activeGroup->RemoveByPosition(pos, region->m_object);
        grid->Add(region);
    }
    return 1;
}

RVA_COMPGEN(0x00163d00, 0x1e, ??_GCWwdGridIter@@UAEPAXI@Z)
RVA(0x001685d0, 0x4a)
void CWwdSpatialMgr::FreeGrids() {
    if (m_defaultRegionGrid) {
        delete m_defaultRegionGrid;
        m_defaultRegionGrid = NULL;
    }
    if (m_largeRegionGrid) {
        delete m_largeRegionGrid;
        m_largeRegionGrid = NULL;
    }
    if (m_smallRegionGrid) {
        delete m_smallRegionGrid;
        m_smallRegionGrid = NULL;
    }
    m_activeGroup = NULL;
}

// @early-stop
RVA(0x00168620, 0xe1)
i32 CWwdSpatialMgr::ActivateAt(i32 centerX, i32 centerY) {
    if (m_activeCenterX == centerX && m_activeCenterY == centerY) {
        return 0;
    }
    m_activeCenterX = centerX;
    m_activeCenterY = centerY;

    WwdRect r;
    r.m_minX = centerX - m_defaultRegionHalfWidth;
    r.m_minY = centerY - m_defaultRegionHalfHeight;
    r.m_maxX = m_defaultRegionHalfWidth + centerX;
    r.m_maxY = m_defaultRegionHalfHeight + centerY;
    i32 n0 = m_defaultRegionGrid->Query(r, 1);

    r.m_minX = centerX - m_largeRegionHalfWidth;
    r.m_minY = centerY - m_largeRegionHalfHeight;
    r.m_maxX = m_largeRegionHalfWidth + centerX;
    r.m_maxY = m_largeRegionHalfHeight + centerY;
    i32 n1 = m_largeRegionGrid->Query(r, 1);

    r.m_minX = centerX - m_smallRegionHalfWidth;
    r.m_minY = centerY - m_smallRegionHalfHeight;
    r.m_maxX = m_smallRegionHalfWidth + centerX;
    r.m_maxY = m_smallRegionHalfHeight + centerY;
    i32 n2 = m_smallRegionGrid->Query(r, 1);

    return n0 + n1 + n2;
}

RVA(0x00168710, 0x2e)
i32 CWwdSpatialMgr::ActivateKeepActiveObjects() {
    i32 n = ActivateKeepActiveFromGrid(m_defaultRegionGrid);
    n += ActivateKeepActiveFromGrid(m_largeRegionGrid);
    n += ActivateKeepActiveFromGrid(m_smallRegionGrid);
    return n;
}

RVA(0x00168740, 0x95)
i32 CWwdSpatialMgr::ActivateKeepActiveFromGrid(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != NULL; obj = it.GetNext()) {
        CGameObject* record = obj->m_object;
        if (HAS(static_cast<WwdGameObjectFlags>(record->m_flags), WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE)
            || HAS(
                static_cast<LogicRecordFlags>(record->m_logicRecord->m_flags),
                LOGIC_RECORD_FLAG_KEEP_ACTIVE
            )) {
            m_activeGroup->InsertSorted(record, 1);
            grid->Remove(obj);
            ++count;
        }
    }
    return count;
}

// @early-stop
RVA(0x001687e0, 0x3af)
i32 CWwdSpatialMgr::DeactivateOutside(i32 centerX, i32 centerY) {
    i32 count = 0;
    WwdRect defaultBounds;
    defaultBounds.m_minX = centerX - m_defaultRegionHalfWidth;
    defaultBounds.m_minY = centerY - m_defaultRegionHalfHeight;
    defaultBounds.m_maxX = m_defaultRegionHalfWidth + centerX;
    defaultBounds.m_maxY = m_defaultRegionHalfHeight + centerY;
    WwdRect largeBounds;
    largeBounds.m_minX = centerX - m_largeRegionHalfWidth;
    largeBounds.m_minY = centerY - m_largeRegionHalfHeight;
    largeBounds.m_maxX = centerX + m_largeRegionHalfWidth;
    largeBounds.m_maxY = centerY + m_largeRegionHalfHeight;
    WwdRect smallBounds;
    smallBounds.m_minX = centerX - m_smallRegionHalfWidth;
    smallBounds.m_minY = centerY - m_smallRegionHalfHeight;
    smallBounds.m_maxX = centerX + m_smallRegionHalfWidth;
    smallBounds.m_maxY = centerY + m_smallRegionHalfHeight;

    POSITION pos = m_activeGroup->m_list.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_activeGroup->NextChild(pos));
        if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags),
                WWD_GAME_OBJECT_FLAG_DELETE_IF_VIEW_OUTSIDE_LEVEL)) {

            if (centerX < m_levelBounds.left - 0x140 || centerX > m_levelBounds.right + 0x140
                || centerY < m_levelBounds.top - 0xdc || centerY > m_levelBounds.bottom + 0xdc) {
                if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags),
                        WWD_GAME_OBJECT_FLAG_DISPATCH_OBJECT_REMOVED)) {
                    CLogicRecord* record = obj->m_logicRecord;
                    record->SetLogicEvent(ACT_OBJECT_REMOVED);
                    record->m_dispatch(obj);
                }
                m_activeGroup->RemoveAll(cur, obj);
                if (obj != NULL) {
                    delete obj;
                }
                obj = NULL;
            }
        }
        if (obj != NULL
            && !HAS(static_cast<WwdGameObjectFlags>(obj->m_flags), WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE)
            && HAS(
                static_cast<WwdGameObjectFlags>(obj->m_flags),
                WWD_GAME_OBJECT_FLAG_WORLD_SPACE
            )) {
            i32 x = obj->m_screenX;
            i32 y = obj->m_screenY;
            WwdRegion* r = &obj->m_region;
            if (x < m_levelBounds.left) {
                x = m_levelBounds.left;
            }
            if (y < m_levelBounds.top) {
                y = m_levelBounds.top;
            }
            if (x >= m_levelBounds.right) {
                x = m_levelBounds.right;
            }
            if (y >= m_levelBounds.bottom) {
                y = m_levelBounds.bottom;
            }
            r->m_x = x;
            r->m_y = y;
            WwdGameObjectFlags flags = static_cast<WwdGameObjectFlags>(obj->m_flags);
            i32 result;
            if (HAS(flags, WWD_GAME_OBJECT_FLAG_LARGE_ACTIVE_REGION)) {
                CWwdGrid* grid = m_largeRegionGrid;
                if (x >= largeBounds.m_minX && y >= largeBounds.m_minY && x <= largeBounds.m_maxX
                    && y <= largeBounds.m_maxY) {
                    result = 0;
                } else {
                    result = DeactivateRegionObject(grid, cur, obj, r, flags);
                }
            } else if (HAS(flags, WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION)) {
                CWwdGrid* grid = m_smallRegionGrid;
                if (x >= smallBounds.m_minX && y >= smallBounds.m_minY && x <= smallBounds.m_maxX
                    && y <= smallBounds.m_maxY) {
                    result = 0;
                } else {
                    result = DeactivateRegionObject(grid, cur, obj, r, flags);
                }
            } else {
                CWwdGrid* grid = m_defaultRegionGrid;
                if (x >= defaultBounds.m_minX && y >= defaultBounds.m_minY
                    && x <= defaultBounds.m_maxX && y <= defaultBounds.m_maxY) {
                    result = 0;
                } else {
                    result = DeactivateRegionObject(grid, cur, obj, r, flags);
                }
            }
            count += result;
        }
    }
    return count;
}

RVA(0x00168b90, 0x40)
i32 CWwdSpatialMgr::PruneCount() {
    i32 n = 0;
    if (m_defaultRegionGrid) {
        n = m_defaultRegionGrid->Clear();
    }
    if (m_largeRegionGrid) {
        n += m_largeRegionGrid->Clear();
    }
    if (m_smallRegionGrid) {
        n += m_smallRegionGrid->Clear();
    }
    if (m_activeGroup) {
        m_activeGroup->PruneOrphans();
    }
    return n;
}

RVA(0x00168bd0, 0x6d)
void CWwdSpatialMgr::ParkObject(CWwdGameObject* obj) {
    WwdGameObjectFlags flags = static_cast<WwdGameObjectFlags>(obj->m_flags);
    if (HAS(flags, WWD_GAME_OBJECT_FLAG_LARGE_ACTIVE_REGION)) {
        m_largeRegionGrid->Add(&obj->m_region);
        m_activeGroup->RegisterObjectId(obj);
    } else if (HAS(flags, WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION)) {
        m_smallRegionGrid->Add(&obj->m_region);
        m_activeGroup->RegisterObjectId(obj);
    } else {
        m_defaultRegionGrid->Add(&obj->m_region);
        m_activeGroup->RegisterObjectId(obj);
    }
}

RVA(0x00168c40, 0x2e)
i32 CWwdSpatialMgr::FlushAll() {
    i32 n = FlushGrid(m_defaultRegionGrid);
    n += FlushGrid(m_largeRegionGrid);
    n += FlushGrid(m_smallRegionGrid);
    return n;
}

RVA(0x00168c70, 0x85)
i32 CWwdSpatialMgr::FlushGrid(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != NULL; obj = it.GetNext()) {
        CGameObject* record = obj->m_object;
        m_activeGroup->InsertSorted(record, 1);
        grid->Remove(obj);
        ++count;
    }
    return count;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00168d00, 0x46)
i32 CWwdSpatialMgr::ForEach(void(__cdecl* cb)(CGameObject*)) {
    if (cb == NULL) {
        return 0;
    }
    i32 n = ForEachGrid(m_defaultRegionGrid, cb);
    n += ForEachGrid(m_largeRegionGrid, cb);
    n += ForEachGrid(m_smallRegionGrid, cb);
    return n;
}

RVA(0x00168d50, 0x73)
i32 CWwdSpatialMgr::ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CGameObject*)) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != NULL; obj = it.GetNext()) {
        cb(obj->m_object);
        ++count;
    }
    return count;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00168dd0, 0x6d)
CGameObject* CWwdSpatialMgr::GetFirstObject() {
    m_iterationGrid = m_defaultRegionGrid;
    WwdRegion* n = m_iter.Start(m_defaultRegionGrid, 0);
    if (n) {
        return n->m_object;
    }
    m_iterationGrid = m_largeRegionGrid;
    n = m_iter.Start(m_largeRegionGrid, 0);
    if (n) {
        return n->m_object;
    }
    m_iterationGrid = m_smallRegionGrid;
    n = m_iter.Start(m_smallRegionGrid, 0);
    if (n) {
        return n->m_object;
    }
    m_iterationGrid = NULL;
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00168e40, 0x85)
CGameObject* CWwdSpatialMgr::GetNextObject() {
    if (m_iterationGrid == NULL) {
        return NULL;
    }
    WwdRegion* n = m_iter.GetNext();
    if (n) {
        return n->m_object;
    }
    if (m_iterationGrid == m_defaultRegionGrid) {
        m_iterationGrid = m_largeRegionGrid;
        n = m_iter.Start(m_largeRegionGrid, 0);
        if (n) {
            return n->m_object;
        }
    }
    if (m_iterationGrid == m_largeRegionGrid) {
        m_iterationGrid = m_smallRegionGrid;
        n = m_iter.Start(m_smallRegionGrid, 0);
        if (n) {
            return n->m_object;
        }
    }
    m_iterationGrid = NULL;
    return NULL;
}
