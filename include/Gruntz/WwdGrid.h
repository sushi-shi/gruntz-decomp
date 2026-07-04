// CWwdGrid - one plane's spatial bucket-index grid (placeholder name; engine
// class is the "tomalla-64" cluster the matched CWwdSpatialMgr holds three
// of). Abstract base: vtable @ 0x5f0328, scalar-deleting dtor @ vtbl+4, a pure
// virtual callback @ vtbl+0x14 invoked per object found inside a query rect.
// Derives from the engine Wap::CObject base (g_wapObjectDtorVtbl @ 0x5e8cb4).
//
// The grid covers a world rectangle [minX,minY]..[maxX,maxY] split into cells of
// 2^shift size; each cell is an 8-byte intrusive doubly-linked list head
// (BucketHead = {head, tail}). Objects are region nodes (WwdRegion) linked
// through their +0x00 {next,prev} pair, carrying their pixel position at
// +0x10/+0x14 and a back-pointer to their owning bucket at +0x0c.
//
// Field names are placeholders; only offsets + emitted bytes are load-bearing.
#ifndef GRUNTZ_WWDGRID_H
#define GRUNTZ_WWDGRID_H

#include <Ints.h>
#include <Wap32/Object.h> // Wap::CObject - the shared engine grand-base
#include <rva.h>

struct BucketHead;

// A grid region node: link pair @ +0x00, owning-bucket back-pointer @ +0x0c,
// pixel position @ +0x10/+0x14 (the engine's CWwdObject region sub-object @
// object+0x9c).
SIZE_UNKNOWN(WwdRegion);
struct WwdRegion {
    WwdRegion* m_next; // +0x00
    WwdRegion* m_prev; // +0x04
    char m_pad08[0x0c - 0x08];
    BucketHead* m_bucket; // +0x0c  cached owning bucket
    i32 m_x;              // +0x10
    i32 m_y;              // +0x14
};

// 8-byte intrusive list head: {head, tail} of WwdRegion nodes, with the engine's
// __thiscall link/unlink ops (reloc-masked externs at 0x1390e0/0x1391e0). The
// non-trivial ctor (zero the pair) + user dtor make `new BucketHead[n]` lower to
// the MSVC array-cookie alloc + __ehvec_ctor/__ehvec_dtor pair (and a /GX frame),
// matching the retail grid's bucket-array build/teardown.
SIZE_UNKNOWN(BucketHead);
struct BucketHead {
    WwdRegion* m_head; // +0x00
    WwdRegion* m_tail; // +0x04
    BucketHead() {
        m_head = 0;
        m_tail = 0;
    }
    ~BucketHead() {}
    void AddNode_1390e0(WwdRegion* node);
    void Unlink_1391e0(WwdRegion* node);
};

// The Wap::CObject engine base (CObject-like, vtable @0x5e8cb4): the implicit vptr
// @+0x00 + the 5-slot CObject-style interface (GetRuntimeClass/dtor/Serialize/
// AssertValid/Dump -> the shared sub_1bef01/scalar-dtor/sub_0028ec/sub_00106e/
// sub_004034). Real polymorphic: the empty inline virtual dtor makes cl emit the
// implicit ??_7Wap::CObject grand-base re-stamp (reloc-masks 0x5e8cb4) folded into
// every leaf dtor, and the destructible base subobject gives ~CWwdGrid its /GX
// frame. The 4 non-dtor virtuals live in sibling TUs (declared, reloc-masked).
// NO VTBL: ??_7Wap@@CObject masks the SHARED CObject vtable @0x5e8cb4 (already
// bound as g_wapObjectDtorVtbl in reconbatch2) - a per-class VTBL would dup-DATA.
// The grand-base is Wap::CObject (Wap32/Object.h).

// CWwdGrid derives from Wap::CObject. Real polymorphic now: the ctor gets the
// implicit ??_7CWwdGrid vptr stamp, ~CWwdGrid the implicit stamp-first re-stamp,
// and the per-object query callback is the pure virtual OnFound at slot 5 (vtbl
// +0x14, == retail's __purecall slot). cl emits ??_7CWwdGrid (slot relocs mask
// the 0x5f0328 target); VTBL pairs the emitted ??_7CWwdGrid with the delinked
// datum (0x1f0328 was unbound). Exact size 0x44 (grid-setup RezAlloc(0x44) x3).
SIZE(CWwdGrid, 0x44);
VTBL(CWwdGrid, 0x001f0328);
class CWwdGrid : public Wap::CObject {
public:
    // ctor: build the grid over rect (x0,y0,x1,y1) with cell sizes cellW/cellH.
    CWwdGrid(i32 x0, i32 y0, i32 x1, i32 y1, i32 cellW, i32 cellH);
    virtual ~CWwdGrid() OVERRIDE;
    virtual void OnFound(WwdRegion* r) = 0; // slot 5 (vtbl+0x14, __purecall)

    void FreeBuckets();
    i32 Add(WwdRegion* r);
    void Remove(WwdRegion* r);
    i32 Query(i32 x0, i32 y0, i32 x1, i32 y1, i32 doRemove);
    i32 Clear();

    i32 m_allocated;       // +0x04  buckets-allocated flag
    i32 m_count;           // +0x08  live object count
    i32 m_cols;            // +0x0c  width/cellH + 1
    i32 m_rows;            // +0x10  height/cellW + 1
    i32 m_shiftY;          // +0x14  log2(cellW)
    i32 m_shiftX;          // +0x18  log2(cellH)
    i32 m_cellCount;       // +0x1c  numCells = cols*rows
    i32 m_width;           // +0x20
    i32 m_height;          // +0x24
    i32 m_minX;            // +0x28
    i32 m_minY;            // +0x2c
    i32 m_maxX;            // +0x30
    i32 m_maxY;            // +0x34
    i32 m_cellH;           // +0x38  2^shiftY
    i32 m_cellW;           // +0x3c  2^shiftX
    BucketHead* m_buckets; // +0x40
};

#endif // GRUNTZ_WWDGRID_H
