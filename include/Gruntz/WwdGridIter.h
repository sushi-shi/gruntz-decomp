// WwdGridIter.h - CWwdGridIter, the rect-restricted position cursor over a
// CWwdGrid (the "tomalla-67" cluster). Shared between its owning TU (WwdGrid.cpp,
// where the Start/Init/GetNext bodies live at 0x191ad0/0x191b10/0x191c30 - the
// same obj/.text run as CWwdGrid) and CWwdSpatialMgr (WwdSpatialMgr.cpp), whose
// GetFirst/GetNext API and CountInRect/FlushGrid/ForEachGrid walks drive it.
//
// The cursor walks every grid cell overlapping a query rectangle and visits each
// node truly inside it (optionally unlinking it). Field names are placeholders;
// only offsets + emitted bytes are load-bearing.
#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (iterator's base)
#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h> // DSoundLink - the node IS the link (unbiased)

class CWwdGrid; // the canonical grid (<Gruntz/WwdGrid.h>); the cursor holds a pointer
struct BucketHead; // a grid cell's {head,tail} bucket (<Gruntz/WwdGrid.h>)

// A 16-byte query rectangle passed by value into the cursor + the grid scroll.
struct WwdRect {
    i32 a, b, c, d;
};

// THE grid region node (ex-WwdGridNode - WwdGrid.h and this header modeled this one
// struct field-for-field; merged 2026-07-20): link pair @ +0x00 (from DSoundLink -
// the node IS the link, so the shared InsertHead/Unlink take it directly),
// owning-bucket back-pointer @ +0x0c, pixel position @ +0x10/+0x14, owning-object
// back-pointer @ +0x18 (the engine's CWwdObject region sub-object @ object+0x9c,
// 0x1c bytes). Lives HERE (not WwdGrid.h) so the object headers that embed it by
// value (UserLogic.h/WwdGameObject.h) stay off the full grid header.
struct WwdRegion : DSoundLink { // {m_next,m_prev} @ +0x00/+0x04 from DSoundLink
    char m_pad08[0x0c - 0x08];
    BucketHead* m_bucket; // +0x0c  cached owning bucket
    i32 m_x;              // +0x10
    i32 m_y;              // +0x14
    struct CGameObject* m_object; // +0x18  owning wide-object back-pointer
};

// Position-iterator over a CWwdGrid: a rect-restricted cursor. REAL POLYMORPHIC:
// the retail object's vtable @ 0x5f02a8 is the 5-slot CObject-style interface
// (slot0 sub_1bef01, slot1 the scalar-deleting dtor 0x163a20, slots 2/3/4 the
// shared sub_0028ec/sub_00106e/sub_004034) - the shared engine grand-base CObject.
// Deriving from CObject supplies slots 0/2/3/4 by inheritance; slot 1 is the
// class's own dtor override. cl auto-emits ??_7CWwdGridIter + the implicit
// vptr-FIRST ctor stamp (== the old m_vptr=g_wwdGridIterVtbl first-store shape).
// WwdFile::RebuildPlanes INLINE-stamps this table into its worker's +0x70 embedded
// cursor (genuine inline-construction). This class OWNS the 0x5f02a8 catalog name.
//
// Cursor layout (offsets from 0x191b10/0x191c30): implicit vptr @ +0x00, the grid
// @ +0x04, the current matched node @ +0x08, the saved-next node @ +0x0c, the
// clamped query rect @ +0x10..+0x1c, the cell-range corners @ +0x20..+0x2c, the
// live cell-walk counters @ +0x30..+0x3c, the remove flag @ +0x40.
SIZE(CWwdGridIter, 0x44);
VTBL(CWwdGridIter, 0x001f02a8);
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
    CWwdGrid* m_grid;    // +0x04
    WwdRegion* m_cur;  // +0x08  current matched node
    WwdRegion* m_next; // +0x0c saved-next (cell head cursor)
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

// The iterator ctor: cl auto-emits the implicit vptr-FIRST stamp (??_7CWwdGridIter
// -> reloc-masks the shared 0x5f02a8), then zeros the cursor fields inline.
inline CWwdGridIter::CWwdGridIter() {
    m_grid = 0;
    m_cur = 0;
}
// The dtor body is EMPTY - retail's own out-of-line ??1CWwdGridIter (0x163a10) is
// `mov [ecx], ??_7CObject; ret` (the elided-derived-stamp + inline ~CObject and
// nothing else). [The former `WwdGridIter_Release_191c70(this)` body was a
// fabricated symbol pointing INTO THE INTERIOR of GetNext (0x191c30+0x40) - a hack
// to force users' /GX frames; the user-declared dtor alone already forces the
// frame (a destructible local => fs:0 registration).]
inline CWwdGridIter::~CWwdGridIter() {}

SIZE_UNKNOWN(WwdRect);
SIZE(WwdRegion, 0x1c); // == the +0x9c..+0xb7 embedded m_region span

#endif // GRUNTZ_WWDGRIDITER_H
