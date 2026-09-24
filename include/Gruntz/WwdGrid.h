#ifndef GRUNTZ_WWDGRID_H
#define GRUNTZ_WWDGRID_H

#include <rva.h>

#include <MfcWin.h>

#include <Gruntz/WwdGridIter.h>
#include <Ints.h>
#include <Lith/BaseList.h>
#include <Wap32/Object.h>

struct BucketHead;

struct BucketHead : CLTBaseList {

    BucketHead() {}

    ~BucketHead();
};

class CWwdGrid : public CObject {
public:
    CWwdGrid() {
        m_allocated = false;
    }
    virtual ~CWwdGrid() OVERRIDE {
        FreeBuckets();
    }
    virtual void OnFound(WwdRegion* r) = 0;

    i32 Setup(RECT rect, i32 cellW, i32 cellH);
    i32 Setup(RECT rect);

    void FreeBuckets();
    i32 Add(WwdRegion* r);
    void Remove(WwdRegion* r);
    i32 Query(WwdRect q, i32 doRemove);
    i32 Clear();

    b32 m_allocated;
    i32 m_count;
    CSize m_gridSize;
    CSize m_cellShift;
    i32 m_cellCount;
    CSize m_extent;

    WwdRect m_bounds;
    CSize m_cellSize;
    BucketHead* m_buckets;
};

#endif // GRUNTZ_WWDGRID_H
