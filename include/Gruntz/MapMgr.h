#ifndef SRC_GRUNTZ_MAPMGR_H
#define SRC_GRUNTZ_MAPMGR_H

#include <rva.h>

#include <Mfc.h>

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

// The retail destructor is the 5-byte `jmp` at 0x9e720 that CMapMgr's unwind
// funclets call; the body at 0x9e7e0 is a separate member CMapMgr::Reset calls
// DIRECTLY, so it is not the destructor. Its retail name is unrecoverable - it
// releases the storage and re-zeroes the head, hence Free.
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
    virtual i32 Visit(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    virtual i32 Save(CFileMemBase*);
    virtual i32 Load(CFileMemBase*);

    virtual i32
    Search(i32 x1, i32 y1, i32 x2, i32 y2, CPtrList* list, i32 maskA, i32 maskB, i32 maskC);
    virtual i32 IsCellClear(i32 x, i32 y);

    void Clip(const tagRECT* r);
    void ComputeCellFlags(i32 x, i32 y, i32 id3);
    i32 AllocGrid(i32 width, i32 height, void (*callback)());
    i32
    SearchEdge(i32 xA, i32 yA, i32 xB, i32 yB, CPtrList* list, i32 clearFlag, i32 maskA, i32 maskC);
    i32 UpdateDiagonals(CGruntzMgr* unused);
    i32 LineIsClear(i32 x0, i32 y0, i32 x1, i32 y1);

    i32 Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag);
    i32 Insert(BrickzNode* node);
    BrickzNode* PopFront();
    void CellPush(BrickzNode* node);
    BrickzNode* Find(i32 key1, i32 key2);
    BrickzNode* FindCellNode(i32 col, i32 row);
    void Drain();
    void Unlink(BrickzNode* node);
    void CellPop(BrickzNode* node, i32 flag);

    void ResetCells();

    i32 CellFlagsAt(i32 x, i32 y);

    BrickzCell* m_cellPool;

    union {
        BrickzCell** m_rows;
        i32** m_rowInts;
        char** m_rowBytes;
    };
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
    i32 m_maskA;
    i32 m_maskC;
    i32 m_maskB;
    i32 m_dirty;

    RECT m_bounds;
    i32 m_gridW;
    i32 m_gridH;
    CDDrawSurfaceMgr* m_attrMgr;
};

#endif // SRC_GRUNTZ_MAPMGR_H
