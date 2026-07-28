#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (iterator's base)
#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h> // DSoundLink - the node IS the link (unbiased)

class CWwdGrid;    // the canonical grid (<Gruntz/WwdGrid.h>); the cursor holds a pointer
struct BucketHead; // a grid cell's {head,tail} bucket (<Gruntz/WwdGrid.h>)

struct WwdRect {
    i32 m_minX; // +0x00
    i32 m_minY; // +0x04
    i32 m_maxX; // +0x08
    i32 m_maxY; // +0x0c
};
SIZE_UNKNOWN();

// The grid node splits in two, and the split is BYTE-PROVEN, not a guess: retail
// emits TWO distinct default ctors for the SAME +0x9c sub-object - 0x15b2a0 zeroes
// {+0x0c,+0x08} and 0x15b2b0 zeroes {+0x0c,+0x08,+0x18}. A class has exactly one
// default ctor, so the smaller one is a BASE and the larger is the derived (which
// inlines it). CDDrawChildGroup::CreateObject_159250/159440 call the base copy and
// spell the `m_object = 0` store inline; CreateObject_159600 calls the derived copy.
struct WwdGridNode : DSoundLink { // {m_next,m_prev} @ +0x00/+0x04 from DSoundLink
    WwdGridNode();                // 0x15b2a0 (COMDAT copy in WwdObjMgr.cpp)
    i32 m_08;                     // +0x08
    BucketHead* m_bucket;         // +0x0c  cached owning bucket
    i32 m_x;                      // +0x10
    i32 m_y;                      // +0x14
};
SIZE(0x18);

struct WwdRegion : WwdGridNode {
    WwdRegion(); // 0x15b2b0 (COMDAT copy in WwdFactoryObject.cpp)
    // Empty but user-declared - it earns the embedded +0x9c member its own /GX
    // unwind-map index, which is what makes retail's ctor-in-flight state walk in
    // CDDrawChildGroup::CreateObject_159250/159440/159600 run 0..5 instead of 0..3.
    ~WwdRegion() {}
    struct CGameObject* m_object; // +0x18  owning wide-object back-pointer
};
SIZE(0x1c); // == the +0x9c..+0xb7 embedded m_region span

// Both node ctors are INLINE (retail's are header-defined: every one of them is a
// /Gy COMDAT that some call sites fold and others call). The OOL_CTOR guards are
// the per-TU switch that reproduces retail's per-site decision, because MSVC5's
// /O2 is /Ob1 and inlining is a per-TU property of the definition, not per site:
// a TU that #defines the guard sees a DECLARATION only, so every construction
// there is a CALL - and that TU supplies the out-of-line body (the COMDAT).
// This whole cluster (also CRESOLVENODE_/ANIMWORKEROBJ_/WWDDIRTYRECT_/CGAMEOBJECT_
// OOL_CTOR) is written up in docs/patterns/ob1-inline-budget-divergence.md, together
// with the #pragma inline_depth(0) forcer that recovers a COMDAT for a ctor a TU has
// to keep inline.
//   WWDGRIDNODE_OOL_CTOR -> WwdObjMgr.cpp (0x15b2a0; its factories CALL it)
//   WWDREGION_OOL_CTOR   -> WwdFactoryObject.cpp (0x15b2b0). Retail folds WwdRegion
//     into CGameObject::CGameObject @0x15b390, which lives in that same TU - but
//     guarding it there is deliberate: it frees the two /Ob1 expansions that let
//     `??0CWapObj` fold instead of being pruned into a proven-absent ??_7CWapObj.
#ifndef WWDGRIDNODE_OOL_CTOR
inline WwdGridNode::WwdGridNode() {
    m_bucket = 0;
    m_08 = 0;
}
#endif

#ifndef WWDREGION_OOL_CTOR
inline WwdRegion::WwdRegion() {
    m_object = 0;
}
#endif

class CWwdGridIter : public CObject {
public:
    // slots 0/2/3/4 (0x1bef01 / 0x0028ec / 0x00106e / 0x004034) inherited from
    // CObject; slot 1 is the class's own scalar-deleting dtor.
    virtual ~CWwdGridIter() OVERRIDE; // slot 1 (scalar-deleting dtor 0x163a20; engine teardown)

    CWwdGridIter();
    WwdRegion* Start(CWwdGrid* grid, i32 remove);              // 0x191ad0 init cursor + first
    WwdRegion* Init(CWwdGrid* grid, WwdRect rect, i32 remove); // 0x191b10 set rect + first
    WwdRegion* GetNext();                                      // 0x191c30 advance the cursor

    // implicit vptr @ +0x00  (= 0x5f02a8, shared g_planeRenderVtbl)
    CWwdGrid* m_grid;  // +0x04
    WwdRegion* m_cur;  // +0x08  current matched node
    WwdRegion* m_next; // +0x0c saved-next (cell head cursor)
    WwdRect m_rect;    // +0x10  clamped query rect (x0,y0,x1,y1)
    i32 m_rowStart;    // +0x20  (x0-minX)>>shiftY
    i32 m_colStart;    // +0x24  (y0-minY)>>shiftX
    i32 m_rowEnd;      // +0x28  (x1-minX)>>shiftY
    i32 m_colEnd;      // +0x2c  (y1-minY)>>shiftX
    i32 m_cell;        // +0x30  current linear cell index
    i32 m_row;         // +0x34  current row counter
    i32 m_col;         // +0x38  current col counter
    i32 m_rowBase;     // +0x3c  current row-base linear index
    i32 m_remove;      // +0x40  unlink-as-visited flag
};
SIZE(0x44);

inline CWwdGridIter::CWwdGridIter() {
    m_grid = 0;
    m_cur = 0;
}
inline CWwdGridIter::~CWwdGridIter() {}

#endif // GRUNTZ_WWDGRIDITER_H
