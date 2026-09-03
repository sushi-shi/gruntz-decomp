#ifndef SRC_GRUNTZ_MAPMGR_H
#define SRC_GRUNTZ_MAPMGR_H

#include <rva.h>

#include <MfcWin.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

struct BrickzCell;
struct BrickzCellNode;
struct BrickzNode;
class CDDrawSurfaceMgr;
struct tagRECT;

class CFileMemBase;

class CBrickzNodePool {
public:
    CBrickzNodePool();
    ~CBrickzNodePool();
    i32 Allocate(u32 count);
    void Free();

    BrickzNode* m_freeList;
    BrickzNode* m_storage;
    u32 m_count;
};

class CBrickzCellNodePool {
public:
    CBrickzCellNodePool();
    ~CBrickzCellNodePool();
    i32 Allocate(u32 count);
    void Free();

    BrickzCellNode* m_storage;
    BrickzCellNode* m_freeList;
    u32 m_count;
};

class CGruntzMgr;
class CMapMgr {
public:
    CMapMgr();
    ~CMapMgr();

    virtual void Reset();
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    virtual i32 Save(CFileMemBase*);
    virtual i32 Load(CFileMemBase*);

    virtual i32 FindPath(
        i32 startX,
        i32 startY,
        i32 goalX,
        i32 goalY,
        CPtrList* outPath,
        i32 blockedMask,
        i32 diagonalMask,
        i32 passableMask
    );
    virtual i32 IsCellClear(i32 x, i32 y);

    RVA(0x0002b340, 0xaa)
    inline void Clip(const tagRECT* src) {
        CRect fullBounds(0, 0, m_width, m_height);
        CRect requestedBounds;
        if (src != NULL) {
            requestedBounds = *src;
            requestedBounds.InflateRect(0, 0, 1, 1);
        } else {
            requestedBounds = fullBounds;
        }
        if (!IntersectRect(&m_bounds, &requestedBounds, &fullBounds)) {
            m_bounds = requestedBounds;
        }
        m_gridSize = CRect(m_bounds).Size();
    }
    void ComputeCellFlags(i32 x, i32 y, i32 tileId);
    i32 AllocGrid(i32 width, i32 height, void (*callback)());
    i32 FindPathWithEndpointOverrides(
        i32 startX,
        i32 startY,
        i32 goalX,
        i32 goalY,
        CPtrList* outPath,
        i32 clearEndpointFlags,
        i32 blockedMask,
        i32 passableMask
    );
    i32 UpdateDiagonals(CGruntzMgr* unused);
    i32 LineIsClear(i32 x0, i32 y0, i32 x1, i32 y1);

    i32 ExpandNeighbor(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diagonal);
    i32 InsertOpenNode(BrickzNode* node);
    BrickzNode* PopBestOpenNode();
    void LinkClosedNode(BrickzNode* node);
    BrickzNode* FindOpenNode(i32 col, i32 row);
    BrickzNode* FindClosedNode(i32 col, i32 row);
    void RecycleOpenNodes();
    void UnlinkOpenNode(BrickzNode* node);
    void UnlinkClosedNode(BrickzNode* node, i32 recycleSearchNode);

    void RecycleClosedNodes();

    i32 CellFlagsAt(i32 x, i32 y);

    BrickzCell* m_cellPool;

    BrickzCell** m_rows;
    u32 m_width;
    u32 m_height;
    u32 m_cellCount;
    BrickzNode* m_openList;
    i32 m_reserved1c;
    Coord m_start;
    Coord m_goal;

    CBrickzNodePool m_nodePool;
    CBrickzCellNodePool m_cellNodePool;
    void (*m_stepCb)();
    i32 m_edgeMask;
    i32 m_blockedMask;
    i32 m_passableMask;
    i32 m_diagonalMask;
    b32 m_dirty;

    RECT m_bounds;
    CSize m_gridSize;
    CDDrawSurfaceMgr* m_attrMgr;
};

#endif // SRC_GRUNTZ_MAPMGR_H
