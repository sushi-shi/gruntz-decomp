#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>

#include <string.h>

class CGameStats;
struct tagRECT;
struct BrickzNode;

struct BrickzCellNode {
    BrickzNode* m_searchNode;
    BrickzCellNode* m_cellPrev;
    BrickzCellNode* m_cellNext;
};

struct BrickzNode {
    i32 m_col;
    i32 m_row;
    i32 m_gCost;
    i32 m_hCost;
    i32 m_fCost;
    BrickzNode* m_openNext;
    BrickzNode* m_openPrev;

    BrickzNode* m_parent;
    BrickzCellNode* m_cellLink;
};

inline BrickzNode* CBrickzNodePool::Pop() {
    BrickzNode* node = m_freeList;
    BrickzNode* next = node->m_openNext;
    if (next == NULL) {
        node = NULL;
    } else {
        m_freeList = next;
        next->m_openPrev = NULL;
    }
    return node;
}

inline void CBrickzNodePool::Push(BrickzNode* node) {
    node->m_openNext = m_freeList;
    node->m_openPrev = NULL;
    m_freeList->m_openPrev = node;
    m_freeList = node;
}

inline BrickzCellNode* CBrickzCellNodePool::Pop() {
    BrickzCellNode* node = m_freeList;
    BrickzCellNode* next = node->m_cellNext;
    if (next == NULL) {
        return NULL;
    }
    m_freeList = next;
    next->m_cellPrev = NULL;
    return node;
}

inline void CBrickzCellNodePool::Push(BrickzCellNode* node) {
    node->m_cellNext = m_freeList;
    node->m_cellPrev = NULL;
    m_freeList->m_cellPrev = node;
    m_freeList = node;
}

GZ_ENUM_CONST_BEGIN(BrickzCellMask)
    BRICKZ_BLOCKED_MASK = 0x939,
    BRICKZ_CELL_ROUTE_MASKB = 0x2000,
    BRICKZ_CELL_OCCUPIED = 0x20000000,
    BRICKZ_CELL_UNOCCUPIED_MASK = ~0x20000000
GZ_ENUM_CONST_END(BrickzCellMask)

GZ_ENUM_CONST_BEGIN(BrickStackRandomization)
    BRICK_COLOR_ROLL_PERCENT_MAX = 100,
    BRICK_TWO_STACK_TOP_PERCENT = 50,
    BRICK_THREE_STACK_LAYER_ROLL_MAX = 600,
    BRICK_THREE_STACK_LOW_ROLL_MAX = 200,
    BRICK_THREE_STACK_MIDDLE_ROLL_MAX = 400
GZ_ENUM_CONST_END(BrickStackRandomization)

struct BrickzCell {

    union {
        i32 m_flags;
        u8 m_flagBytes[4];
    };

    union {
        i32 m_occupantId;
        u8 m_occupantIdBytes[4];
    };

    i32 m_objectId;
    i32 m_tileId;
    TileCollisionKind m_typeCode;
    i32 m_count;
    BrickzCellNode* m_head;
};

inline TileCollisionKind CMapMgr::CellTypeAt(i32 x, i32 y) const {
    return m_rows[y][x].m_typeCode;
}

inline BrickzCell CMapMgr::CellAt(i32 x, i32 y) {
    BrickzCell cell;
    const BrickzCell* source;
    if (static_cast<u32>(x) < m_width && static_cast<u32>(y) < m_height) {
        source = &m_rows[y][x];
    } else {
        memset(&cell, 1, sizeof(cell));
        source = &cell;
    }
    return *source;
}

// Preserve indexed-name expansion at the two AdvanceMotion read sites.
#define MAP_CELL_FLAGS_AT_UNCHECKED(map, x, y) ((map)->m_rows[(y)][(x)].m_flags)

inline i32& CMapMgr::CellFlagsAtUnchecked(i32 x, i32 y) {
    return m_rows[y][x].m_flags;
}

RVA(0x00075a40, 0x34)
inline i32 CMapMgr::CellFlagsAt(i32 x, i32 y) {
    if (static_cast<u32>(x) < m_width && static_cast<u32>(y) < m_height) {
        return CellFlagsAtUnchecked(x, y);
    }
    return 1;
}

inline i32 CMapMgr::ObjectIdAt(u32 x, u32 y) const {
    if (x < m_width && y < m_height) {
        return m_rows[y][x].m_objectId;
    }
    return 0;
}

inline void CMapMgr::SetObjectIdAt(u32 x, u32 y, i32 objectId) {
    if (x < m_width && y < m_height) {
        m_rows[y][x].m_objectId = objectId;
        if (objectId != 0) {
            CellFlagsAtUnchecked(x, y) |= 0x40000;
        } else {
            CellFlagsAtUnchecked(x, y) &= ~0x40000;
        }
    }
}

RVA(0x000853f0, 0x46)
inline i32 CMapMgr::IsCellClear(i32 x, i32 y) {
    return CellFlagsAt(x, y) == 0;
}

#endif // GRUNTZ_BRICKZ_H
