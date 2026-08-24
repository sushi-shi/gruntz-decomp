#ifndef GRUNTZ_WWDGRID_H
#define GRUNTZ_WWDGRID_H

#include <rva.h>

#include <Dsndmgr/IntrusiveList.h>
#include <Gruntz/WwdGridIter.h>
#include <Ints.h>
#include <Wap32/Object.h>

struct BucketHead;

struct BucketHead : IntrusiveList {

    BucketHead() {}

    ~BucketHead();
};

class CWwdGrid : public CObject {
public:
    CWwdGrid() {
        m_allocated = 0;
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

    i32 m_allocated;
    i32 m_count;
    i32 m_cols;
    i32 m_rows;
    i32 m_shiftY;
    i32 m_shiftX;
    i32 m_cellCount;
    i32 m_width;
    i32 m_height;

    WwdRect m_bounds;
    i32 m_cellH;
    i32 m_cellW;
    BucketHead* m_buckets;
};

#endif // GRUNTZ_WWDGRID_H
