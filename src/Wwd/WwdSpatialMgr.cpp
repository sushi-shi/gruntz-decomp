#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/WwdGameObject.h>
#include <Wwd/WwdSpatialMgr.h>
#include <Gruntz/WwdGrid.h>
#include <Gruntz/WwdGridIter.h>
#include <Mfc.h>
#include <Wap32/Object.h>

VTBL(CWwdGridIter, 0x001f02a8);
RVA_COMPGEN(0x00163a20, 0x1e, ??_GCWwdGridIter@@UAEPAXI@Z)
RVA(0x00163a40, 0x41)
CWwdSpatialMgr::~CWwdSpatialMgr() {
    FreeGrids();
}

RVA(0x001682f0, 0x4a)
void CWwdSpatialMgr::FreeGrids() {
    if (m_grid0) {
        delete m_grid0;
        m_grid0 = 0;
    }
    if (m_grid1) {
        delete m_grid1;
        m_grid1 = 0;
    }
    if (m_grid2) {
        delete m_grid2;
        m_grid2 = 0;
    }
    m_mgr = 0;
}

// @early-stop
RVA(0x00168340, 0xe1)
i32 CWwdSpatialMgr::ScrollTo(i32 dx, i32 dy) {
    if (m_scrollX == dx && m_scrollY == dy) {
        return 0;
    }
    m_scrollX = dx;
    m_scrollY = dy;

    WwdRect r;
    r.m_minX = dx - m_org0x;
    r.m_minY = dy - m_org0y;
    r.m_maxX = m_org0x + dx;
    r.m_maxY = m_org0y + dy;
    i32 n0 = m_grid0->Query(r, 1);

    r.m_minX = dx - m_org1x;
    r.m_minY = dy - m_org1y;
    r.m_maxX = m_org1x + dx;
    r.m_maxY = m_org1y + dy;
    i32 n1 = m_grid1->Query(r, 1);

    r.m_minX = dx - m_org2x;
    r.m_minY = dy - m_org2y;
    r.m_maxX = m_org2x + dx;
    r.m_maxY = m_org2y + dy;
    i32 n2 = m_grid2->Query(r, 1);

    return n0 + n1 + n2;
}

RVA(0x00168430, 0x2e)
i32 CWwdSpatialMgr::GetSize() {
    i32 n = CountInRect(m_grid0);
    n += CountInRect(m_grid1);
    n += CountInRect(m_grid2);
    return n;
}

RVA(0x00168460, 0x95)
i32 CWwdSpatialMgr::CountInRect(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CGameObject* w = obj->m_object;
        if ((w->m_flags & 0x2) || (w->m_animWorker->m_flags & 0x4)) {
            m_mgr->InsertSorted(w, 1);
            grid->Remove(obj);
            ++count;
        }
    }
    return count;
}

// @early-stop
RVA(0x00168500, 0x3af)
i32 CWwdSpatialMgr::Relocate(i32 newX, i32 newY) {
    i32 count = 0;
    i32 lo0x = newX - m_org0x;
    i32 hi0x = m_org0x + newX;
    i32 lo0y = newY - m_org0y;
    i32 hi0y = m_org0y + newY;
    i32 lo1x = newX - m_org1x;
    i32 lo1y = newY - m_org1y;
    i32 hi1x = newX + m_org1x;
    i32 hi1y = newY + m_org1y;
    i32 lo2x = newX - m_org2x;
    i32 lo2y = newY - m_org2y;
    i32 hi2x = newX + m_org2x;
    i32 hi2y = newY + m_org2y;

    POSITION pos = m_mgr->m_list.GetHeadPosition();
    while (pos != 0) {
        POSITION cur = pos;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_mgr->m_list.GetNext(pos));
        if (obj->m_flags & 0x40) {

            if (newX < m_bounds.left - 0x140 || newX > m_bounds.right + 0x140
                || newY < m_bounds.top - 0xdc || newY > m_bounds.bottom + 0xdc) {
                if (obj->m_flags & 0x80000) {
                    AnimWorkerObj* w = obj->m_animWorker;
                    w->SetActKey(0x1d);
                    w->m_notify(obj);
                }
                m_mgr->RemoveAll(cur, obj);
                if (obj != 0) {
                    delete obj;
                }
                obj = 0;
            }
        }
        if (obj != 0 && !(obj->m_flags & 0x2) && (obj->m_flags & 0x40000)) {
            i32 x = obj->m_screenX;
            i32 y = obj->m_screenY;
            WwdRegion* r = &obj->m_region;
            if (x < m_bounds.left) {
                x = m_bounds.left;
            }
            if (y < m_bounds.top) {
                y = m_bounds.top;
            }
            if (x >= m_bounds.right) {
                x = m_bounds.right;
            }
            if (y >= m_bounds.bottom) {
                y = m_bounds.bottom;
            }
            r->m_x = x;
            r->m_y = y;
            i32 flags = obj->m_flags;
            i32 result;
            if (flags & 0x800000) {
                CWwdGrid* grid = m_grid1;
                if (x >= lo1x && y >= lo1y && x <= hi1x && y <= hi1y) {
                    result = 0;
                } else if (flags & 0x20) {
                    if (flags & 0x80000) {
                        AnimWorkerObj* w = obj->m_animWorker;
                        w->SetActKey(0x1d);
                        w->m_notify(obj);
                    }
                    m_mgr->RemoveAll(cur, obj);
                    delete obj;
                    result = 1;
                } else {
                    if (flags & 0x100000) {
                        AnimWorkerObj* w = obj->m_animWorker;
                        i32 saved = w->ActKey();
                        w->SetActKey(0x1e);
                        w->m_notify(obj);
                        if (w->ActKey() == 0x1e) {
                            w->m_actKey = saved;
                        }
                    }
                    m_mgr->RemoveByPosition(cur, r->m_object);
                    grid->Add(r);
                    result = 1;
                }
            } else if (flags & 0x1000000) {
                CWwdGrid* grid = m_grid2;
                if (x >= lo2x && y >= lo2y && x <= hi2x && y <= hi2y) {
                    result = 0;
                } else if (flags & 0x20) {
                    if (flags & 0x80000) {
                        AnimWorkerObj* w = obj->m_animWorker;
                        w->SetActKey(0x1d);
                        w->m_notify(obj);
                    }
                    m_mgr->RemoveAll(cur, obj);
                    delete obj;
                    result = 1;
                } else {
                    if (flags & 0x100000) {
                        AnimWorkerObj* w = obj->m_animWorker;
                        i32 saved = w->ActKey();
                        w->SetActKey(0x1e);
                        w->m_notify(obj);
                        if (w->ActKey() == 0x1e) {
                            w->m_actKey = saved;
                        }
                    }
                    m_mgr->RemoveByPosition(cur, r->m_object);
                    grid->Add(r);
                    result = 1;
                }
            } else {
                CWwdGrid* grid = m_grid0;
                if (x >= lo0x && y >= lo0y && x <= hi0x && y <= hi0y) {
                    result = 0;
                } else if (flags & 0x20) {
                    if (flags & 0x80000) {
                        AnimWorkerObj* w = obj->m_animWorker;
                        w->SetActKey(0x1d);
                        w->m_notify(obj);
                    }
                    m_mgr->RemoveAll(cur, obj);
                    delete obj;
                    result = 1;
                } else {
                    if (flags & 0x100000) {
                        AnimWorkerObj* w = obj->m_animWorker;
                        i32 saved = w->ActKey();
                        w->SetActKey(0x1e);
                        w->m_notify(obj);
                        if (w->ActKey() == 0x1e) {
                            w->m_actKey = saved;
                        }
                    }
                    m_mgr->RemoveByPosition(cur, r->m_object);
                    grid->Add(r);
                    result = 1;
                }
            }
            count += result;
        }
    }
    return count;
}

RVA(0x001688b0, 0x40)
i32 CWwdSpatialMgr::PruneCount() {
    i32 n = 0;
    if (m_grid0) {
        n = m_grid0->Clear();
    }
    if (m_grid1) {
        n += m_grid1->Clear();
    }
    if (m_grid2) {
        n += m_grid2->Clear();
    }
    if (m_mgr) {
        m_mgr->PruneOrphans();
    }
    return n;
}

RVA(0x001688f0, 0x6d)
void CWwdSpatialMgr::RemoveObject(CWwdGameObject* obj) {
    i32 flags = obj->m_flags;
    if (flags & 0x800000) {
        m_grid1->Add(&obj->m_region);
        m_mgr->AddToMap48(obj);
    } else if (flags & 0x1000000) {
        m_grid2->Add(&obj->m_region);
        m_mgr->AddToMap48(obj);
    } else {
        m_grid0->Add(&obj->m_region);
        m_mgr->AddToMap48(obj);
    }
}

RVA(0x00168960, 0x2e)
i32 CWwdSpatialMgr::FlushAll() {
    i32 n = FlushGrid(m_grid0);
    n += FlushGrid(m_grid1);
    n += FlushGrid(m_grid2);
    return n;
}

RVA(0x00168990, 0x85)
i32 CWwdSpatialMgr::FlushGrid(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CGameObject* w = obj->m_object;
        m_mgr->InsertSorted(w, 1);
        grid->Remove(obj);
        ++count;
    }
    return count;
}

RVA(0x00168a20, 0x46)
i32 CWwdSpatialMgr::ForEach(void(__cdecl* cb)(CGameObject*)) {
    if (cb == 0) {
        return 0;
    }
    i32 n = ForEachGrid(m_grid0, cb);
    n += ForEachGrid(m_grid1, cb);
    n += ForEachGrid(m_grid2, cb);
    return n;
}

RVA(0x00168a70, 0x73)
i32 CWwdSpatialMgr::ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CGameObject*)) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        cb(obj->m_object);
        ++count;
    }
    return count;
}

RVA(0x00168af0, 0x6d)
CGameObject* CWwdSpatialMgr::GetFirstObject() {
    m_curGrid = m_grid0;
    WwdRegion* n = m_iter.Start(m_grid0, 0);
    if (n) {
        return n->m_object;
    }
    m_curGrid = m_grid1;
    n = m_iter.Start(m_grid1, 0);
    if (n) {
        return n->m_object;
    }
    m_curGrid = m_grid2;
    n = m_iter.Start(m_grid2, 0);
    if (n) {
        return n->m_object;
    }
    m_curGrid = 0;
    return 0;
}

RVA(0x00168b60, 0x85)
CGameObject* CWwdSpatialMgr::GetNextObject() {
    if (m_curGrid == 0) {
        return 0;
    }
    WwdRegion* n = m_iter.GetNext();
    if (n) {
        return n->m_object;
    }
    if (m_curGrid == m_grid0) {
        m_curGrid = m_grid1;
        n = m_iter.Start(m_grid1, 0);
        if (n) {
            return n->m_object;
        }
    }
    if (m_curGrid == m_grid1) {
        m_curGrid = m_grid2;
        n = m_iter.Start(m_grid2, 0);
        if (n) {
            return n->m_object;
        }
    }
    m_curGrid = 0;
    return 0;
}
