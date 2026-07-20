#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h> // the shared object-collection manager class
#include <Gruntz/WwdGameObject.h>     // canonical CWwdGameObject (the managed sprites)
#include <Wwd/WwdSpatialMgr.h>        // the canonical class (was defined locally here)
#include <Gruntz/WwdGrid.h>     // canonical CWwdGrid (Add/Remove/Query/Clear @0x191840..0x191a70)
#include <Gruntz/WwdGridIter.h> // CWwdGridIter cursor + WwdRegion + WwdRect (shared;
#include <Mfc.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (iterator's CObject prefix)

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

// ===========================================================================
// 0x168340 - ScrollTo(dx, dy): if unchanged, return 0; else cache the new
// position and re-bucket every grid by the (origin -> new) delta, accumulating
// the three grid-scroll results.
// ===========================================================================
// @early-stop
// scheduling wall (+ dissolution cost) - MSVC floats the m_scrollX/m_scrollY
// (0x68/0x6c) member stores down into the first grid's struct setup to fill
// pipeline slots; retail emits them eagerly at the jne target. Body logic
// byte-faithful. NOTE (2026-07-14): dropped ~66% -> ~35% when the ex-view's
// Scroll_1918c0(WwdRect BY VALUE, flag) call was dissolved onto the canonical
// CWwdGrid::Query(x0,y0,x1,y1,doRemove) (5 scalars): retail's caller pushes the
// rect BY VALUE (the by-value model scored closer), so a faithful re-match wants
// canonical Query retyped to `Query(WwdRect, i32)` - a cross-TU unification with
// WwdGrid.cpp, deferred. The view was a divergent 2nd CWwdGrid definition (the
// anti-pattern); the canonical dissolution is correct, the % is the accepted cost.
RVA(0x00168340, 0xe1)
i32 CWwdSpatialMgr::ScrollTo(i32 dx, i32 dy) {
    if (m_scrollX == dx && m_scrollY == dy) {
        return 0;
    }
    m_scrollX = dx;
    m_scrollY = dy;

    WwdRect r0;
    r0.a = dx - m_org0x;
    r0.b = dy - m_org0y;
    r0.c = m_org0x + dx;
    r0.d = m_org0y + dy;
    i32 n = m_grid0->Query(r0.a, r0.b, r0.c, r0.d, 1);

    WwdRect r1;
    r1.a = dx - m_org1x;
    r1.b = dy - m_org1y;
    r1.c = m_org1x + dx;
    r1.d = m_org1y + dy;
    n += m_grid1->Query(r1.a, r1.b, r1.c, r1.d, 1);

    WwdRect r2;
    r2.a = dx - m_org2x;
    r2.b = dy - m_org2y;
    r2.c = m_org2x + dx;
    r2.d = m_org2y + dy;
    n += m_grid2->Query(r2.a, r2.b, r2.c, r2.d, 1);

    return n;
}

RVA(0x00168430, 0x2e)
i32 CWwdSpatialMgr::GetSize() {
    i32 n = CountInRect(m_grid0);
    n += CountInRect(m_grid1);
    n += CountInRect(m_grid2);
    return n;
}

// ===========================================================================
// 0x168460 - CountInRect(grid): walk the grid; for each object whose worker is
// flagged (vtbl+8 bit 0x2, or its +0x7c worker bit 0x4), re-insert it into the
// master manager and remove it from the grid; count how many were re-inserted.
// /GX frame from the on-stack iterator object.
// ===========================================================================
// @early-stop
// /GX EH-state wall - the body is byte-identical (every mov/call/jcc matches),
// but the scoped destructible CWwdGridIter local's frame numbers differently:
// retail `sub esp,0x44` + scope cookie `push 0x8`, recompile `sub esp,0x40` +
// `push 0x0`, shifting [esp+N] by 4. See docs/patterns/gx-scoped-local-eh-frame-size.md.
// ~86%.
RVA(0x00168460, 0x95)
i32 CWwdSpatialMgr::CountInRect(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CGameObject* w = obj->m_object;
        if ((w->m_flags & 0x2) || (w->m_7c->m_08 & 0x4)) {
            m_mgr->InsertSorted_159e40(w, 1);
            grid->Remove(obj);
            ++count;
        }
    }
    return count;
}

// ===========================================================================
// 0x168500 - Relocate(newX, newY): walk the master manager's object list and
// re-bucket every object across the three plane grids by its flags, using 12
// precomputed per-grid translated coordinate ranges. 943 bytes with a 3x
// replicated dispatch; deferred to the final sweep (leaf-first redo) per the
// >512 B "don't half-do it" rule.
// ===========================================================================
// @early-stop
// BODY NOT RECONSTRUCTED - 943 B (>512 B "don't half-do it" rule). Logic is
// mapped (head computes x-orgX/y-orgY/x+orgX/y+orgY per grid into 12 stack
// temporaries, then walks m_mgr's object list re-bucketing by flags
// 0x800000 -> grid2 / 0x1000000 -> grid1 / else grid0, calling grid Add/Remove
// + InsertSorted/RemoveAll); deferred to the final sweep as a leaf-first redo.
RVA(0x00168500, 0x3af)
i32 CWwdSpatialMgr::Relocate(i32 newX, i32 newY) {
    return 0;
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
        m_mgr->PruneOrphans_15b1d0();
    }
    return n;
}

RVA(0x001688f0, 0x6d)
void CWwdSpatialMgr::RemoveObject(CWwdGameObject* obj) {
    i32 flags = obj->m_flags;
    if (flags & 0x800000) {
        m_grid1->Add(&obj->m_region);
        m_mgr->AddToMap48_15aba0(obj);
    } else if (flags & 0x1000000) {
        m_grid2->Add(&obj->m_region);
        m_mgr->AddToMap48_15aba0(obj);
    } else {
        m_grid0->Add(&obj->m_region);
        m_mgr->AddToMap48_15aba0(obj);
    }
}

RVA(0x00168960, 0x2e)
i32 CWwdSpatialMgr::FlushAll() {
    i32 n = FlushGrid(m_grid0);
    n += FlushGrid(m_grid1);
    n += FlushGrid(m_grid2);
    return n;
}

// ===========================================================================
// 0x168990 - FlushGrid(grid): walk the grid with a scratch cursor; re-insert
// every object into the master manager and unlink it from the grid; return the
// count drained. /GX frame from the on-stack iterator.
// ===========================================================================
// @early-stop
// /GX scoped-local EH-frame wall - body byte-identical (every mov/call/jcc
// matches), but the destructible CWwdGridIter local frames differently: retail
// `sub esp,0x44` + scope cookie, recompile `sub esp,0x40`, shifting [esp+N] by 4.
// Same wall as CountInRect; docs/patterns/gx-scoped-local-eh-frame-size.md.
RVA(0x00168990, 0x85)
i32 CWwdSpatialMgr::FlushGrid(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdRegion* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CGameObject* w = obj->m_object;
        m_mgr->InsertSorted_159e40(w, 1);
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

// ===========================================================================
// 0x168a70 - ForEachGrid(grid, cb): walk the grid with a scratch cursor, firing
// the __cdecl callback on each object; return how many fired. /GX frame from the
// on-stack iterator.
// ===========================================================================
// @early-stop
// /GX scoped-local EH-frame wall (same as FlushGrid/CountInRect): body
// byte-identical, the iterator local's frame numbers off by 4. See
// docs/patterns/gx-scoped-local-eh-frame-size.md.
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

SIZE_UNKNOWN(CWwdObjWorker);
SIZE_UNKNOWN(CWwdSpatialMgr);
