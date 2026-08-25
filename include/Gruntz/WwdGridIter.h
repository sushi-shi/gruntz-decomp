#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <rva.h>

#include <Dsndmgr/IntrusiveList.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <stddef.h>

class CWwdGrid;
struct BucketHead;

struct WwdRect {
    union {
        struct {
            i32 m_minX;
            i32 m_minY;
            i32 m_maxX;
            i32 m_maxY;
        };
        RECT m_rect;
    };
};

struct WwdGridNode : IntrusiveLink {
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
