#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h>
#include <Ints.h>
#include <Wap32/Object.h>

class CWwdGrid;
struct BucketHead;

struct WwdRect {
    i32 m_minX;
    i32 m_minY;
    i32 m_maxX;
    i32 m_maxY;
};
SIZE_UNKNOWN();

struct WwdGridNode : DSoundLink {
    WwdGridNode();
    i32 m_reserved08;
    BucketHead* m_bucket;
    i32 m_x;
    i32 m_y;
};
SIZE(0x18);

struct WwdRegion : WwdGridNode {
    WwdRegion();

    ~WwdRegion() {}
    struct CGameObject* m_object;
};
SIZE(0x1c);

// Per-TU guards select inline versus out-of-line constructor emission; see
// docs/patterns/ob1-inline-budget-divergence.md.
#ifndef WWDGRIDNODE_OOL_CTOR
inline WwdGridNode::WwdGridNode() {
    m_bucket = 0;
    m_reserved08 = 0;
}
#endif

#ifndef WWDREGION_OOL_CTOR
inline WwdRegion::WwdRegion() {
    m_object = 0;
}
#endif

class CWwdGridIter : public CObject {
public:
    virtual ~CWwdGridIter() OVERRIDE;

    CWwdGridIter();
    WwdRegion* Start(CWwdGrid* grid, i32 remove);
    WwdRegion* Init(CWwdGrid* grid, WwdRect rect, i32 remove);
    WwdRegion* GetNext();

    CWwdGrid* m_grid;
    WwdRegion* m_cur;
    WwdRegion* m_next;
    WwdRect m_rect;
    i32 m_rowStart;
    i32 m_colStart;
    i32 m_rowEnd;
    i32 m_colEnd;
    i32 m_cell;
    i32 m_row;
    i32 m_col;
    i32 m_rowBase;
    i32 m_remove;
};
SIZE(0x44);

inline CWwdGridIter::CWwdGridIter() {
    m_grid = 0;
    m_cur = 0;
}
inline CWwdGridIter::~CWwdGridIter() {}

#endif // GRUNTZ_WWDGRIDITER_H
