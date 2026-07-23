#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (iterator's base)
#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h> // DSoundLink - the node IS the link (unbiased)

class CWwdGrid;    // the canonical grid (<Gruntz/WwdGrid.h>); the cursor holds a pointer
struct BucketHead; // a grid cell's {head,tail} bucket (<Gruntz/WwdGrid.h>)

struct WwdRect {
    i32 a, b, c, d;
};
SIZE_UNKNOWN();

struct WwdRegion : DSoundLink { // {m_next,m_prev} @ +0x00/+0x04 from DSoundLink
    char m_pad08[0x0c - 0x08];
    BucketHead* m_bucket;         // +0x0c  cached owning bucket
    i32 m_x;                      // +0x10
    i32 m_y;                      // +0x14
    struct CGameObject* m_object; // +0x18  owning wide-object back-pointer
};
SIZE(0x1c); // == the +0x9c..+0xb7 embedded m_region span

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
