#include <rva.h>
// <Mfc.h> brings the real CObject/CRect types. This cluster's three spatial
// grids (tomalla-64), the master CDDrawChildGroup, and the grid iterator are all
// reloc-masked engine externs (no bodies here).
#include <DDrawMgr/DDrawChildGroup.h> // the shared object-collection manager class
#include <Gruntz/WwdGameObject.h>     // canonical CWwdGameObject (the managed sprites)
#include <Wwd/WwdSpatialMgr.h>        // the canonical class (was defined locally here)
#include <Gruntz/WwdGrid.h>     // canonical CWwdGrid (Add/Remove/Query/Clear @0x191840..0x191a70)
                                // + WwdRegion + BucketHead - was a reduced .cpp-local view here
#include <Gruntz/WwdGridIter.h> // CWwdGridIter cursor + WwdGridNode + WwdRect (shared;
                                // the cursor's Start/Init/GetNext bodies live in WwdGrid.cpp)
#include <Mfc.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (iterator's CObject prefix)

// ===========================================================================
// CWwdSpatialMgr - the per-level object-bucket manager held at WwdFile+0xb0.
//
// Owns a master CDDrawChildGroup (m_mgr @ +0x00) plus THREE spatial grids
// (CWwdGrid / tomalla-64, one per plane: m_grid0/1/2 @ +0x04/+0x08/+0x0c).
// An object is routed into a grid by its flag bits (0x800000 -> grid1,
// 0x1000000 -> grid2, else grid0). Origin pairs at +0x40.. track each grid's
// scroll offset; the world bbox is +0x58.., the cached scroll pos +0x68/+0x6c.
//
// Field names are tomalla placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
// ===========================================================================

// --- reloc-masked engine externs -------------------------------------------

// WwdGridNode (the region sub-object @ CWwdGameObject+0x9c), WwdRect, and the cursor
// CWwdGridIter now live in <Gruntz/WwdGridIter.h> (shared with WwdGrid.cpp, which
// carries the cursor's Start/Init/GetNext bodies - same obj as CWwdGrid).
//
// CWwdGrid + WwdBucketHead ARE DISSOLVED (2026-07-14): the ex-local reduced view of
// CWwdGrid (which named its methods Scroll_1918c0/Remove_191890 to take the iterator's
// WwdGridNode directly, and its buckets WwdBucketHead) IS the canonical polymorphic
// CWwdGrid in <Gruntz/WwdGrid.h> - VERIFIED by RVA: Add @0x191840, Remove @0x191890,
// Query @0x1918c0 (the ex-Scroll_1918c0 - a WwdRect passed by value == 4 scalars),
// Clear @0x191a70 all match. FreeGrids' `delete m_grid0` dispatches the same slot-1
// scalar-deleting dtor (vtable layout identical: vptr@0, m_buckets@0x40). The
// iterator's WwdGridNode* is cast to the canonical WwdRegion* at the two Remove sites
// (same engine node, two shared-header names).

// The sprites ARE the canonical CWwdGameObject (<Gruntz/WwdGameObject.h>): the
// ex-`CWwdGameObject`/`CWwdObjWorker` views' every field lands on it - m_flags
// (+0x08), the +0x7c worker (whose +0x08 flag word is WwdAnimWorker::m_08), and
// the embedded +0x9c WwdGridNode region (m_region).

// CDDrawChildGroup - master object manager (m_mgr) is the shared <Gruntz/WwdObjMgr.h>
// class; the cluster calls InsertSorted_159e40 / AddToMap48_15aba0 / PruneOrphans_15b1d0.

// CWwdGridIter (the rect-restricted position cursor over a CWwdGrid) is defined
// in <Gruntz/WwdGridIter.h>; its Start/Init/GetNext bodies live in WwdGrid.cpp
// (0x191ad0..0x191c30, the same obj/.text run as CWwdGrid). This TU embeds one
// (m_iter @ +0x70) for the GetFirst/GetNext API and stack-locals it for the
// grid-walk methods below.

// --- the cluster class -----------------------------------------------------

// (The class definition moved to the shared <Wwd/WwdSpatialMgr.h> - it was defined
// twice: here and as the plane headers' "CPlaneScroll" scroll-rect view. One class.)

// 0x163a40 (re-homed from src/Stub/BoundaryUpperEh.cpp): the class's out-of-line
// complete dtor. [The former C163a40 placeholder identity is dissolved: the "+0x70
// destructible CObject subobject" is the m_iter member (CWwdGridIter : CObject at
// +0x70) - a MEMBER, not an MI base - and the teardown IS ~CWwdSpatialMgr:
// FreeGrids() then the compiler's inline ~m_iter (its own empty dtor's derived
// stamp dead-store-elided, leaving the base ??_7CObject stamp retail shows).]
RVA(0x00163a40, 0x41)
CWwdSpatialMgr::~CWwdSpatialMgr() {
    FreeGrids();
}

// ===========================================================================
// 0x1682f0 - drop the three plane grids, null the master pointer.
// ===========================================================================
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

// CWwdSpatialMgr::GetSize (0x168430) - total object count across the 3 grids.
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
    for (WwdGridNode* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CWwdGameObject* w = obj->m_object;
        if ((w->m_flags & 0x2) || (w->m_7c->m_08 & 0x4)) {
            m_mgr->InsertSorted_159e40(w, 1);
            grid->Remove(reinterpret_cast<WwdRegion*>(obj));
            ++count;
        }
    }
    return count;
}

// (The CWwdGridIter ctor/dtor - the implicit vptr-FIRST stamp + the WwdGridIter_
// Release_191c70 engine teardown that gives every user its /GX EH frame - are
// inline in <Gruntz/WwdGridIter.h>.)

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

// ===========================================================================
// 0x1688b0 - PruneCount(): RemoveAll over the three grids (summing the counts),
// then PruneOrphans on the master manager (result discarded).
// ===========================================================================
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

// ===========================================================================
// 0x1688f0 - RemoveObject(obj): route by the object's plane flag bits into the
// matching grid (0x800000 -> grid1, 0x1000000 -> grid2, else grid0), drop it
// from that grid's region, then fully remove it from the master manager.
// ===========================================================================
RVA(0x001688f0, 0x6d)
void CWwdSpatialMgr::RemoveObject(CWwdGameObject* obj) {
    i32 flags = obj->m_flags;
    if (flags & 0x800000) {
        m_grid1->Add(reinterpret_cast<WwdRegion*>(&obj->m_region));
        m_mgr->AddToMap48_15aba0(obj);
    } else if (flags & 0x1000000) {
        m_grid2->Add(reinterpret_cast<WwdRegion*>(&obj->m_region));
        m_mgr->AddToMap48_15aba0(obj);
    } else {
        m_grid0->Add(reinterpret_cast<WwdRegion*>(&obj->m_region));
        m_mgr->AddToMap48_15aba0(obj);
    }
}

// ===========================================================================
// 0x168960 - FlushAll(): drain every plane grid back into the master manager,
// summing the per-grid counts.
// ===========================================================================
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
    for (WwdGridNode* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CWwdGameObject* w = obj->m_object;
        m_mgr->InsertSorted_159e40(w, 1);
        grid->Remove(reinterpret_cast<WwdRegion*>(obj));
        ++count;
    }
    return count;
}

// ===========================================================================
// 0x168a20 - ForEach(cb): fire the __cdecl callback on every object across all
// three plane grids (via ForEachGrid), summing the counts. Null cb -> 0.
// ===========================================================================
RVA(0x00168a20, 0x46)
i32 CWwdSpatialMgr::ForEach(void(__cdecl* cb)(CWwdGameObject*)) {
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
i32 CWwdSpatialMgr::ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CWwdGameObject*)) {
    i32 count = 0;
    CWwdGridIter it;
    for (WwdGridNode* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        cb(obj->m_object);
        ++count;
    }
    return count;
}

// ===========================================================================
// 0x168af0 - GetFirstObject(): seed the embedded cursor on the first non-empty
// plane grid (grid0 -> grid1 -> grid2), remembering which grid in m_curGrid;
// return its first object, or 0 when all three are empty.
// ===========================================================================
RVA(0x00168af0, 0x6d)
CWwdGameObject* CWwdSpatialMgr::GetFirstObject() {
    m_curGrid = m_grid0;
    WwdGridNode* n = m_iter.Start(m_grid0, 0);
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

// ===========================================================================
// 0x168b60 - GetNextObject(): advance the embedded cursor; when its current grid
// is exhausted, roll over to the next plane grid (grid0 -> grid1 -> grid2) and
// re-seed. Returns the next object, or 0 when all grids are done.
// ===========================================================================
RVA(0x00168b60, 0x85)
CWwdGameObject* CWwdSpatialMgr::GetNextObject() {
    if (m_curGrid == 0) {
        return 0;
    }
    WwdGridNode* n = m_iter.GetNext();
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

// (The CWwdGridIter cursor methods Start/Init/GetNext @ 0x191ad0/0x191b10/0x191c30
// were re-homed to WwdGrid.cpp - the same obj/.text run as CWwdGrid, matcher-2 D6.)
SIZE_UNKNOWN(CWwdObjWorker);
SIZE_UNKNOWN(CWwdSpatialMgr);
