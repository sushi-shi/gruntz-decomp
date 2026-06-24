#include <rva.h>
// <Mfc.h> brings the real CObject/CRect types. This cluster's three spatial
// grids (ClassUnknown_64), the master CWwdObjMgr, and the grid iterator are all
// reloc-masked engine externs (no bodies here).
#include <Mfc.h>

// ===========================================================================
// CWwdSpatialMgr - the per-level object-bucket manager held at WwdFile+0xb0.
//
// Owns a master CWwdObjMgr (m_mgr @ +0x00) plus THREE spatial grids
// (CWwdGrid / ClassUnknown_64, one per plane: m_grid0/1/2 @ +0x04/+0x08/+0x0c).
// An object is routed into a grid by its flag bits (0x800000 -> grid1,
// 0x1000000 -> grid2, else grid0). Origin pairs at +0x40.. track each grid's
// scroll offset; the world bbox is +0x58.., the cached scroll pos +0x68/+0x6c.
//
// Field names are tomalla placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
// ===========================================================================

// --- reloc-masked engine externs -------------------------------------------

// CWwdObject - the engine sprite. flag word @ +0x08, callback worker @ +0x7c,
// region cache @ +0x9c, primary-map key @ +0x188; scalar deleting dtor @ vtbl+4.
class CWwdObject {
public:
    virtual void Slot00();
    virtual i32 ScalarDtor(i32 flag); // +0x04
    char m_pad08[0x9c - 0x08];
    char m_region9c[0x18]; // +0x9c
};

// CWwdObjMgr - master object manager (m_mgr). Only the cluster-called methods.
class CWwdObjMgr {
public:
    void InsertSorted_159e40(CWwdObject* obj, i32 addToMaps);
    i32 RemoveAll_15ab30(i32 pos, CWwdObject* obj);
    void RemoveByPosition_15ab70(i32 pos, CWwdObject* obj);
    void AddToMap48_15aba0(CWwdObject* obj);
    i32 PruneOrphans_15b1d0();
};

// A 16-byte rectangle passed by value into the grid scroll method.
struct WwdRect {
    i32 a, b, c, d;
};

// CWwdGrid (ClassUnknown_64) - one plane's spatial bucket index. Polymorphic
// CObject-style: scalar deleting dtor @ vtbl+4.
class CWwdGrid {
public:
    virtual void Slot00();
    virtual i32 ScalarDtor(i32 flag); // +0x04
    i32 Scroll_1918c0(WwdRect r, i32 flag);
    i32 Add_191840(void* region);
    i32 Remove_191890(CWwdObject* obj);
    i32 RemoveAll_191a70();
};

// Position-iterator over a CWwdGrid. The retail object's vptr is the shared
// engine table at 0x5f02a8 (its virtuals + GetFirst/GetNext live in another TU);
// stamp it via a reloc-masked DATA() extern (transitional manual vtable).
class CWwdGridIter {
public:
    CWwdGridIter();
    ~CWwdGridIter();                             // 0x191c70-ish engine teardown
    CWwdObject* Start(CWwdGrid* grid, i32 flag); // 0x191ad0 init cursor + first
    CWwdObject* GetNext();                       // 0x191c30 __thiscall

    void* m_vptr;              // +0x00  = &g_wwdGridIterVtbl (0x5f02a8)
    i32 m_04;                  // +0x04
    i32 m_08;                  // +0x08
    char m_pad0c[0x44 - 0x0c]; // internal cursor state, up to +0x40 (191c30/191b10)
};
DATA(0x005f02a8)
extern void* g_wwdGridIterVtbl[];

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

    void FreeGrids();
    i32 ScrollTo(i32 dx, i32 dy);
    i32 GetSize();
    i32 CountInRect(CWwdGrid* grid);
    i32 Relocate(i32 newX, i32 newY);
    i32 PruneCount();
    void RemoveObject(CWwdObject* obj);
};

// ===========================================================================
// 0x1682f0 - drop the three plane grids, null the master pointer.
// ===========================================================================
RVA(0x001682f0, 0x4a)
void CWwdSpatialMgr::FreeGrids() {
    if (m_grid0) {
        m_grid0->ScalarDtor(1);
        m_grid0 = 0;
    }
    if (m_grid1) {
        m_grid1->ScalarDtor(1);
        m_grid1 = 0;
    }
    if (m_grid2) {
        m_grid2->ScalarDtor(1);
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

// ===========================================================================
// 0x168430 - GetSize(): sum CountInRect over the three grids.
// ===========================================================================
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
    for (CWwdObject* obj = it.Start(grid, 0); obj != 0; obj = it.GetNext()) {
        CWwdObject* w = *(CWwdObject**)((char*)obj + 0x18);
        if ((*(u8*)((char*)w + 0x8) & 0x2) || (*(u8*)(*(char**)((char*)w + 0x7c) + 0x8) & 0x4)) {
            m_mgr->InsertSorted_159e40(w, 1);
            grid->Remove_191890(obj);
            ++count;
        }
    }
    return count;
}

// The iterator ctor stamps the shared engine vtable (0x5f02a8) and zeros the
// cursor fields inline; the dtor is the engine teardown that gives this TU its
// /GX EH frame. Both are reloc-masked w.r.t. the vtable address.
inline CWwdGridIter::CWwdGridIter() {
    m_vptr = g_wwdGridIterVtbl;
    m_04 = 0;
    m_08 = 0;
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
    i32 flags = *(i32*)((char*)obj + 0x8);
    if (flags & 0x800000) {
        m_grid1->Add_191840((char*)obj + 0x9c);
        m_mgr->AddToMap48_15aba0(obj);
    } else if (flags & 0x1000000) {
        m_grid2->Add_191840((char*)obj + 0x9c);
        m_mgr->AddToMap48_15aba0(obj);
    } else {
        m_grid0->Add_191840((char*)obj + 0x9c);
        m_mgr->AddToMap48_15aba0(obj);
    }
}
