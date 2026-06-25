// CWwdGrid - one plane's spatial bucket-index grid (placeholder name; engine
// class is the "ClassUnknown_64" cluster the matched CWwdSpatialMgr holds three
// of). Abstract base: vtable @ 0x5f0328, scalar-deleting dtor @ vtbl+4, a pure
// virtual callback @ vtbl+0x14 invoked per object found inside a query rect.
// Derives from the engine "remus" base (g_remusBaseDtorVtbl @ 0x5e8cb4).
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

struct BucketHead;

// A grid region node: link pair @ +0x00, owning-bucket back-pointer @ +0x0c,
// pixel position @ +0x10/+0x14 (the engine's CWwdObject region sub-object @
// object+0x9c).
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

// The "remus" engine base: holds the vptr @ +0x00. Trivial inline ctor (no vptr
// stamp - a derived ctor stamps it), and a non-trivial dtor that restores the
// base vtable. Modeling it as a real base subobject gives ~CWwdGrid its /GX EH
// frame (the derived dtor folds ~CRemusBase's vptr-restore inline). The vptr is
// managed by manual stamps while the full vtable contents aren't reproduced.
struct CRemusBase {
    void* m_vptr; // +0x00
    CRemusBase() {}
    ~CRemusBase(); // INLINE-defined below (folds into every leaf dtor)
};

// CWwdGrid derives from CRemusBase (manual-vptr model: the ctor never stamps the
// vptr - a derived ctor does - and the dtor folds the base teardown).
class CWwdGrid : public CRemusBase {
public:
    // ctor: build the grid over rect (x0,y0,x1,y1) with cell sizes cellW/cellH.
    CWwdGrid(i32 x0, i32 y0, i32 x1, i32 y1, i32 cellW, i32 cellH);
    ~CWwdGrid();

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

// The grid's vtable: the per-object callback at +0x14 is a single-inheritance
// PMF (4 bytes, thiscall by default) - CWwdGrid is now complete so it stays 4B.
struct CWwdGridVtbl {
    void* m_slot00;                          // +0x00
    void* m_dtor;                            // +0x04 scalar-deleting dtor
    void* m_slot08[(0x14 - 0x08) / 4];       // +0x08..+0x10
    void (CWwdGrid::*m_onFound)(WwdRegion*); // +0x14 per-object callback
};

// The parent "remus" base vtable; ~CRemusBase restores it (DATA() in the .cpp).
extern CWwdGridVtbl g_remusBaseDtorVtbl; // 0x5e8cb4

// INLINE so every leaf dtor (~CWwdGrid) FOLDS the base teardown rather than
// emitting `call ??1CRemusBase`; the /GX frame still falls out of the real base.
inline CRemusBase::~CRemusBase() {
    m_vptr = &g_remusBaseDtorVtbl;
}

#endif // GRUNTZ_WWDGRID_H
