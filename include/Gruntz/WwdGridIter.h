#ifndef GRUNTZ_WWDGRIDITER_H
#define GRUNTZ_WWDGRIDITER_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <stddef.h>

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

    // Retail's 0x15b2b0 seeds this base's two fields INLINE, so WwdRegion cannot
    // reach the base through the called ctor above. Same no-seed tag idiom as
    // WwdDirtyRect::WwdDirtyRect(ENoSeed).
    enum ENoSeed {
        NO_SEED
    };
    WwdGridNode(ENoSeed) {}
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

// Both ctors are out-of-line in retail: CDDrawChildGroup's factories CALL them
// (0x15b2a0 from 0x1592b5/0x1594a5, 0x15b2b0 from 0x159663) while the derived
// object's own ctor is inlined into the factory. The bodies live in
// src/Wwd/WwdObjMgr.cpp and src/Wwd/WwdFactoryObject.cpp.

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
    m_grid = NULL;
    m_cur = NULL;
}
inline CWwdGridIter::~CWwdGridIter() {}

#endif // GRUNTZ_WWDGRIDITER_H
