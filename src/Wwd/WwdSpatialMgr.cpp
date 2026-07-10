#include <rva.h>
// <Mfc.h> brings the real CObject/CRect types. This cluster's three spatial
// grids (tomalla-64), the master CWwdObjMgr, and the grid iterator are all
// reloc-masked engine externs (no bodies here).
#include <Gruntz/WwdObjMgr.h> // the shared object-collection manager class
#include <Mfc.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (iterator's CObject prefix)

// ===========================================================================
// CWwdSpatialMgr - the per-level object-bucket manager held at WwdFile+0xb0.
//
// Owns a master CWwdObjMgr (m_mgr @ +0x00) plus THREE spatial grids
// (CWwdGrid / tomalla-64, one per plane: m_grid0/1/2 @ +0x04/+0x08/+0x0c).
// An object is routed into a grid by its flag bits (0x800000 -> grid1,
// 0x1000000 -> grid2, else grid0). Origin pairs at +0x40.. track each grid's
// scroll offset; the world bbox is +0x58.., the cached scroll pos +0x68/+0x6c.
//
// Field names are tomalla placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
// ===========================================================================

// --- reloc-masked engine externs -------------------------------------------

// A grid bucket list node (the region sub-object embedded at CWwdObject+0x9c):
// intrusive link pair @ +0x00/+0x04, owning-bucket back-pointer @ +0x0c, pixel
// position @ +0x10/+0x14, owning-object back-pointer @ +0x18. The iterator walks
// these and reads their position.
struct WwdBucketHead;
struct WwdGridNode {
    WwdGridNode* m_next; // +0x00
    WwdGridNode* m_prev; // +0x04
    char m_pad08[0x0c - 0x08];
    WwdBucketHead* m_bucket;    // +0x0c  cached owning bucket head
    i32 m_x;                    // +0x10
    i32 m_y;                    // +0x14
    class CWwdObject* m_object; // +0x18  owning sprite back-pointer
};

// A worker held off the sprite at +0x7c, carrying its own flag word at +0x08.
struct CWwdObjWorker {
    char m_pad00[0x08];
    i32 m_flags; // +0x08
};

// CWwdObject - the engine sprite. flag word @ +0x08, callback worker @ +0x7c,
// region cache @ +0x9c, primary-map key @ +0x188; scalar deleting dtor @ vtbl+4.
class CWwdObject {
public:
    virtual void Slot00();
    virtual ~CWwdObject(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    char m_pad04[0x08 - 0x04];
    i32 m_flags; // +0x08  plane/active flag word
    char m_pad0c[0x7c - 0x0c];
    CWwdObjWorker* m_worker; // +0x7c  per-object worker (its own +0x08 flags)
    char m_pad80[0x9c - 0x80];
    WwdGridNode m_region; // +0x9c  embedded grid region sub-object
};

// CWwdObjMgr - master object manager (m_mgr) is the shared <Gruntz/WwdObjMgr.h>
// class; the cluster calls InsertSorted_159e40 / AddToMap48_15aba0 / PruneOrphans_15b1d0.

// A 16-byte rectangle passed by value into the grid scroll method.
struct WwdRect {
    i32 a, b, c, d;
};

// 8-byte intrusive list head {head, tail} of WwdGridNode; one per grid cell.
// The unlink op is __thiscall on the head (the node passed as the lone stack
// arg, callee-cleanup ret 4) - a reloc-masked engine extern.
struct WwdBucketHead {
    WwdGridNode* m_head; // +0x00
    WwdGridNode* m_tail; // +0x04
    void Unlink_1391e0(WwdGridNode* node);
};

// CWwdGrid (tomalla-64) - one plane's spatial bucket index. The CANONICAL
// polymorphic model (CObject base, real ~CWwdGrid OVERRIDE at vtbl slot 1,
// pure-virtual OnFound at slot 5) lives in <Gruntz/WwdGrid.h>; this is the
// spatial-mgr's reduced local view. It intentionally models slot 1 as an
// explicit scalar-dtor(i32) method rather than the real destructor because
// FreeGrids/CountInRect invoke the engine's scalar-DELETING-dtor thunk directly
// (`mov eax,[ecx]; push 1; call [eax+4]`); a C++ `delete grid` cannot reproduce
// that (it adds its own null-check + nulls the pointer unconditionally, where
// retail nulls inside the taken branch), so this correct-bytes view is retained.
// (The vtable_hierarchy --audit "CWwdGrid 1 override, 0 OVERRIDE" line is a
// _body_counts artifact of this reduced view - the header carries the OVERRIDE.)
// Data fields (offsets per WwdGrid.h) are read by the iterator: the rect bounds,
// the log2 cell shifts, the column count, and the 8-byte bucket-head array.
class CWwdGrid {
public:
    virtual void Slot00();
    virtual ~CWwdGrid(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    i32 Scroll_1918c0(WwdRect r, i32 flag);
    i32 Add_191840(void* region);
    i32 Remove_191890(WwdGridNode* region);
    i32 RemoveAll_191a70();

    i32 m_allocated; // +0x04  buckets-allocated flag
    i32 m_count;     // +0x08  live object count
    i32 m_cols;      // +0x0c  columns
    i32 m_rows;      // +0x10  rows
    i32 m_shiftY;    // +0x14  log2(cellW)
    i32 m_shiftX;    // +0x18  log2(cellH)
    char m_pad1c[0x28 - 0x1c];
    i32 m_minX; // +0x28
    i32 m_minY; // +0x2c
    i32 m_maxX; // +0x30
    i32 m_maxY; // +0x34
    char m_pad38[0x40 - 0x38];
    WwdBucketHead* m_buckets; // +0x40  array of 8-byte {head,tail} bucket heads
};

// Position-iterator over a CWwdGrid: a rect-restricted cursor that walks every
// grid cell overlapping a query rectangle and visits each node truly inside it
// (optionally unlinking it). REAL POLYMORPHIC now: the retail object's vtable @
// 0x5f02a8 is the 5-slot CObject-style interface (slot0 sub_1bef01, slot1 the
// scalar-deleting dtor 0x163a20, slots 2/3/4 the shared sub_0028ec/sub_00106e/
// sub_004034) - i.e. the shared engine grand-base CObject. Deriving from
// CObject supplies slots 0/2/3/4 by inheritance and slot 1 is the class's own
// dtor override; cl auto-emits ??_7CWwdGridIter + the implicit vptr-FIRST ctor
// stamp (== the old `m_vptr = g_wwdGridIterVtbl` first-store shape). This TU OWNS
// the 0x5f02a8 vtable catalog name via VTBL below:
// WwdFile::RebuildPlanes only INLINE-stamps this table into its worker's +0x70
// embedded cursor (genuine inline-construction, not a ctor call), and now
// references g_planeRenderVtbl as an UNPINNED extern so its stamp reloc-masks
// against this real ??_7CWwdGridIter (the manual g_planeRenderVtbl DATA pin is
// drained). 5 slots with the dtor at slot 1, matching retail exactly.
//
// Cursor layout (offsets from 0x191b10/0x191c30): implicit vptr @ +0x00, the grid
// @ +0x04, the current matched node @ +0x08, the saved-next node @ +0x0c, the
// clamped query rect @ +0x10..+0x1c, the cell-range corners @ +0x20..+0x2c, and
// the live cell-walk counters @ +0x30..+0x3c, with the remove flag @ +0x40.
SIZE(CWwdGridIter, 0x44);
VTBL(CWwdGridIter, 0x001f02a8);
class CWwdGridIter : public CObject {
public:
    // slots 0/2/3/4 (0x1bef01 / 0x0028ec / 0x00106e / 0x004034) inherited from
    // CObject; slot 1 is the class's own scalar-deleting dtor.
    virtual ~CWwdGridIter() OVERRIDE; // slot 1 (scalar-deleting dtor 0x163a20; engine teardown)

    CWwdGridIter();
    WwdGridNode* Start(CWwdGrid* grid, i32 remove); // 0x191ad0 init cursor + first
    WwdGridNode* Init(CWwdGrid* grid, WwdRect rect,
                      i32 remove); // 0x191b10 set rect + first
    WwdGridNode* GetNext();        // 0x191c30 advance the cursor

    // implicit vptr @ +0x00  (= 0x5f02a8, shared g_planeRenderVtbl)
    CWwdGrid* m_grid;    // +0x04
    WwdGridNode* m_cur;  // +0x08  current matched node
    WwdGridNode* m_next; // +0x0c saved-next (cell head cursor)
    WwdRect m_rect;      // +0x10  clamped query rect (x0,y0,x1,y1)
    i32 m_rowStart;      // +0x20  (x0-minX)>>shiftY
    i32 m_colStart;      // +0x24  (y0-minY)>>shiftX
    i32 m_rowEnd;        // +0x28  (x1-minX)>>shiftY
    i32 m_colEnd;        // +0x2c  (y1-minY)>>shiftX
    i32 m_cell;          // +0x30  current linear cell index
    i32 m_row;           // +0x34  current row counter
    i32 m_col;           // +0x38  current col counter
    i32 m_rowBase;       // +0x3c  current row-base linear index
    i32 m_remove;        // +0x40  unlink-as-visited flag
};

// --- the cluster class -----------------------------------------------------

struct CWwdSpatialMgr {
    CWwdObjMgr* m_mgr; // +0x00
    CWwdGrid* m_grid0; // +0x04
    CWwdGrid* m_grid1; // +0x08
    CWwdGrid* m_grid2; // +0x0c
    char m_pad10[0x40 - 0x10];
    i32 m_org0x, m_org0y; // +0x40 / +0x44  grid0 scroll origin
    i32 m_org1x, m_org1y; // +0x48 / +0x4c  grid1 scroll origin
    i32 m_org2x, m_org2y; // +0x50 / +0x54  grid2 scroll origin
    i32 m_bbMinX;         // +0x58
    i32 m_bbMinY;         // +0x5c
    i32 m_bbMaxX;         // +0x60
    i32 m_bbMaxY;         // +0x64
    i32 m_scrollX;        // +0x68
    i32 m_scrollY;        // +0x6c
    CWwdGridIter m_iter;  // +0x70  embedded cursor for the GetFirst/GetNext API
    CWwdGrid* m_curGrid;  // +0xb4  which grid the embedded cursor is walking

    void FreeGrids();
    i32 ScrollTo(i32 dx, i32 dy);
    i32 GetSize(); // 0x168430
    i32 CountInRect(CWwdGrid* grid);
    i32 Relocate(i32 newX, i32 newY);
    i32 PruneCount();
    void RemoveObject(CWwdObject* obj);
    i32 FlushAll();
    i32 FlushGrid(CWwdGrid* grid);
    i32 ForEach(void(__cdecl* cb)(CWwdObject*));
    i32 ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CWwdObject*));
    CWwdObject* GetFirstObject();
    CWwdObject* GetNextObject();
};

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

// 0x163a40 (re-homed from src/Stub/BoundaryUpperEh.cpp): a /GX dtor whose destructible
// CObject base subobject lives at +0x70 and whose member teardown is CWwdSpatialMgr::
// FreeGrids on `this` - i.e. it destructs a CWwdSpatialMgr. Co-located here next to
// FreeGrids; kept a distinct placeholder identity (C163a40, most-derived vptr not at
// offset 0) since it is not the same symbol as the class's inline dtor.
struct Base163a40 {
    virtual ~Base163a40();
};
SIZE_UNKNOWN(Base163a40);
inline Base163a40::~Base163a40() {}
struct C163a40 {
    char _0[0x70];
    Base163a40 m_70; // +0x70  destructible CObject base subobject
    ~C163a40();
};
SIZE_UNKNOWN(C163a40);
RVA(0x00163a40, 0x41)
C163a40::~C163a40() {
    ((CWwdSpatialMgr*)this)->FreeGrids();
}

// ===========================================================================
// 0x168340 - ScrollTo(dx, dy): if unchanged, return 0; else cache the new
// position and re-bucket every grid by the (origin -> new) delta, accumulating
// the three grid-scroll results.
// ===========================================================================
// @early-stop
// scheduling wall - MSVC floats the m_scrollX/m_scrollY (0x68/0x6c) member
// stores down into the first grid's struct setup to fill pipeline slots; retail
// emits them eagerly at the jne target. Body otherwise byte-identical; the
// inserted-then-shifted store pair caps fuzzy% (statement-schedule-faithful,
// not steerable from C). ~66%.
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
    i32 n = m_grid0->Scroll_1918c0(r0, 1);

    WwdRect r1;
    r1.a = dx - m_org1x;
    r1.b = dy - m_org1y;
    r1.c = m_org1x + dx;
    r1.d = m_org1y + dy;
    n += m_grid1->Scroll_1918c0(r1, 1);

    WwdRect r2;
    r2.a = dx - m_org2x;
    r2.b = dy - m_org2y;
    r2.c = m_org2x + dx;
    r2.d = m_org2y + dy;
    n += m_grid2->Scroll_1918c0(r2, 1);

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
        CWwdObject* w = obj->m_object;
        if ((w->m_flags & 0x2) || (w->m_worker->m_flags & 0x4)) {
            m_mgr->InsertSorted_159e40(w, 1);
            grid->Remove_191890(obj);
            ++count;
        }
    }
    return count;
}

// The iterator ctor: cl auto-emits the implicit vptr-FIRST stamp (??_7CWwdGridIter
// -> reloc-masks the shared 0x5f02a8), then zeros the cursor fields inline; the
// dtor is the engine teardown that gives this TU its /GX EH frame.
inline CWwdGridIter::CWwdGridIter() {
    m_grid = 0;
    m_cur = 0;
}
// Non-trivial dtor: the engine release that, under /GX, registers the unwind
// funclet (handler 0x5e2518) and gives this method its EH frame.
extern "C" void WwdGridIter_Release_191c70(CWwdGridIter* it);
inline CWwdGridIter::~CWwdGridIter() {
    WwdGridIter_Release_191c70(this);
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

// ===========================================================================
// 0x1688b0 - PruneCount(): RemoveAll over the three grids (summing the counts),
// then PruneOrphans on the master manager (result discarded).
// ===========================================================================
RVA(0x001688b0, 0x40)
i32 CWwdSpatialMgr::PruneCount() {
    i32 n = 0;
    if (m_grid0) {
        n = m_grid0->RemoveAll_191a70();
    }
    if (m_grid1) {
        n += m_grid1->RemoveAll_191a70();
    }
    if (m_grid2) {
        n += m_grid2->RemoveAll_191a70();
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
void CWwdSpatialMgr::RemoveObject(CWwdObject* obj) {
    i32 flags = obj->m_flags;
    if (flags & 0x800000) {
        m_grid1->Add_191840(&obj->m_region);
        m_mgr->AddToMap48_15aba0(obj);
    } else if (flags & 0x1000000) {
        m_grid2->Add_191840(&obj->m_region);
        m_mgr->AddToMap48_15aba0(obj);
    } else {
        m_grid0->Add_191840(&obj->m_region);
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
        CWwdObject* w = obj->m_object;
        m_mgr->InsertSorted_159e40(w, 1);
        grid->Remove_191890(obj);
        ++count;
    }
    return count;
}

// ===========================================================================
// 0x168a20 - ForEach(cb): fire the __cdecl callback on every object across all
// three plane grids (via ForEachGrid), summing the counts. Null cb -> 0.
// ===========================================================================
RVA(0x00168a20, 0x46)
i32 CWwdSpatialMgr::ForEach(void(__cdecl* cb)(CWwdObject*)) {
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
i32 CWwdSpatialMgr::ForEachGrid(CWwdGrid* grid, void(__cdecl* cb)(CWwdObject*)) {
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
CWwdObject* CWwdSpatialMgr::GetFirstObject() {
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
CWwdObject* CWwdSpatialMgr::GetNextObject() {
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

// ===========================================================================
// CWwdGridIter cursor methods (the tomalla-67 cluster). The iterator walks
// every grid cell overlapping the query rect, visiting each node truly inside it
// (optionally unlinking it). Self-contained / reloc-free apart from the bucket
// Unlink helper.
// ===========================================================================

// 0x191ad0 - Start(grid, remove): seed the cursor over the grid's ENTIRE bounds
// rect (grid->minX..maxY passed by value) and return the first in-rect node.
RVA(0x00191ad0, 0x34)
WwdGridNode* CWwdGridIter::Start(CWwdGrid* grid, i32 remove) {
    // The grid's full bounds rect (minX,minY,maxX,maxY @ +0x28..+0x34) copied
    // as a contiguous 16-byte block - a struct assignment through a pointer, so
    // the four fields move through one register (matching the retail lea+copy).
    // authentic: the four bounds ints ARE the query rect; overlaying them as a
    // WwdRect for the block copy is the real operation (int-block-as-struct view).
    WwdRect full = *(WwdRect*)&grid->m_minX;
    return Init(grid, full, remove);
}

// 0x191b10 - Init(grid, rect, remove): cache the grid + clamped query rect, fail
// fast if the rect is fully outside the grid, derive the cell-range corners and
// the live cell-walk counters, prime the cursor at the first cell head, then
// advance to the first in-rect node.
// @early-stop
// ~95.5% - imul regalloc wall: body byte-identical (the rect block-copy + all 8
// clamp guards + the cell-range corners match), but the final
// base=colStart*cols+rowStart keeps colStart in a different reg than retail
// (retail imul edi,ecx + m_col=ecx between imul/add; recompile imul ecx,esi),
// a 3-instr operand-register choice. Not source-steerable (see
// docs/patterns/zero-register-pinning.md / statement-schedule-faithful.md).
RVA(0x00191b10, 0x111)
WwdGridNode* CWwdGridIter::Init(CWwdGrid* grid, WwdRect rect, i32 remove) {
    m_grid = grid;
    m_rect = rect;
    m_remove = remove;
    if (m_rect.a > grid->m_maxX) {
        return 0;
    }
    if (m_rect.c < grid->m_minX) {
        return 0;
    }
    if (m_rect.b > grid->m_maxY) {
        return 0;
    }
    if (m_rect.d < grid->m_minY) {
        return 0;
    }
    if (m_rect.a < grid->m_minX) {
        m_rect.a = grid->m_minX;
    }
    if (m_rect.c > grid->m_maxX) {
        m_rect.c = grid->m_maxX;
    }
    if (m_rect.b < grid->m_minY) {
        m_rect.b = grid->m_minY;
    }
    if (m_rect.d > grid->m_maxY) {
        m_rect.d = grid->m_maxY;
    }
    m_colStart = (m_rect.b - grid->m_minY) >> grid->m_shiftX;
    m_rowStart = (m_rect.a - grid->m_minX) >> grid->m_shiftY;
    m_colEnd = (m_rect.d - grid->m_minY) >> grid->m_shiftX;
    m_rowEnd = (m_rect.c - grid->m_minX) >> grid->m_shiftY;
    i32 base = m_colStart * grid->m_cols + m_rowStart;
    m_col = m_colStart;
    m_row = m_rowStart;
    m_rowBase = base;
    m_cell = base;
    m_next = grid->m_buckets[base].m_head;
    return GetNext();
}

// 0x191c30 - GetNext(): resume the cell walk. Faithful goto transcription of the
// retail cursor: reload the scan node, advance cells until a non-empty bucket,
// then walk the bucket testing each node against the query rect; on a hit,
// optionally unlink it and return it.
// @early-stop
// ~93.7% - LICM/regalloc wall: the cell-advance block + the whole control flow
// are byte-identical, but cl hoists the loop-invariant query bound m_rect.a
// (+0x10) into a callee-saved register (extra push ebx) where retail reloads it
// from memory each iteration; this cascades the walk's m_y0 reg (ebx vs edi) and
// the remove-block reg assignment. Not source-steerable (member-bound LICM
// choice; see docs/patterns/zero-register-pinning.md).
RVA(0x00191c30, 0xcc)
WwdGridNode* CWwdGridIter::GetNext() {
    WwdGridNode* node;
top:
    node = m_next;
    m_cur = node;
    if (node == 0) {
    nextcell:
        if (m_row < m_rowEnd) {
            ++m_cell;
            ++m_row;
        } else {
            if (m_col >= m_colEnd) {
                return 0;
            }
            m_rowBase += m_grid->m_cols;
            m_cell = m_rowBase;
            m_row = m_rowStart;
            ++m_col;
        }
        m_cur = m_grid->m_buckets[m_cell].m_head;
        if (m_cur == 0) {
            goto nextcell;
        }
    }
    if (m_cur == 0) {
        goto top;
    }
walk:
    m_next = m_cur->m_next;
    if (m_cur->m_x >= m_rect.a && m_cur->m_y >= m_rect.b && m_cur->m_x <= m_rect.c
        && m_cur->m_y <= m_rect.d) {
        if (m_remove) {
            m_grid->m_buckets[m_cell].Unlink_1391e0(m_cur);
            m_cur->m_bucket = 0;
            --m_grid->m_count;
        }
        return m_cur;
    }
    if (m_cur != 0) {
        goto walk;
    }
    goto top;
}
SIZE_UNKNOWN(CWwdObjWorker);
SIZE_UNKNOWN(CWwdSpatialMgr);
SIZE_UNKNOWN(WwdBucketHead);
SIZE_UNKNOWN(WwdGridNode);
SIZE_UNKNOWN(WwdRect);
