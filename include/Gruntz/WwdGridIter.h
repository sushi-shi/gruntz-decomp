#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <rva.h>

#include <Ints.h>
#include <Lith/BaseList.h>
#include <Wap32/Object.h>

#include <stddef.h>

class CWwdGrid;
struct BucketHead;
struct WwdGridNode;

struct WwdRect {
    void Init(i32 minX, i32 minY, i32 maxX, i32 maxY) {
        m_minX = minX;
        m_minY = minY;
        m_maxX = maxX;
        m_maxY = maxY;
    }

    i32 Contains(const WwdGridNode* point) const;

    i32 m_minX;
    i32 m_minY;
    i32 m_maxX;
    i32 m_maxY;
};

struct WwdGridNode : CBaseListItem {
    WwdGridNode();

    enum ENoSeed {
        NO_SEED
    };
    WwdGridNode(ENoSeed) {}
    i32 m_reserved08;
    BucketHead* m_bucket;
    i32 m_x;
    i32 m_y;
};

inline i32 WwdRect::Contains(const WwdGridNode* point) const {
    return point->m_x >= m_minX && point->m_y >= m_minY && point->m_x <= m_maxX
           && point->m_y <= m_maxY;
}

struct WwdRegion : WwdGridNode {
    WwdRegion();

    enum EInlineSeed {
        INLINE_SEED
    };
    WwdRegion(EInlineSeed) : WwdGridNode(WwdGridNode::NO_SEED) {
        SeedFields();
    }

    enum EBaseCall {
        BASE_CALL
    };
    WwdRegion(EBaseCall) : WwdGridNode() {
        m_object = NULL;
    }

    void SeedFields() {
        m_bucket = NULL;
        m_reserved08 = 0;
        m_object = NULL;
    }

    RVA(0x0015b4e0, 0x10)
    ~WwdRegion() {}
    struct CGameObject* m_object;
};

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

inline CWwdGridIter::CWwdGridIter() {
    m_grid = NULL;
    m_cur = NULL;
}
inline CWwdGridIter::~CWwdGridIter() {}

#endif // GRUNTZ_WWDGRIDITER_H
