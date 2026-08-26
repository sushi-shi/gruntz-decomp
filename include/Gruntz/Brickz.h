#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>

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

RVA(0x00075910, 0x34)
inline i32 CMapMgr::CellFlagsAt(i32 x, i32 y) {
    if (static_cast<u32>(x) < m_width && static_cast<u32>(y) < m_height) {
        return m_rows[y][x].m_flags;
    }
    return 1;
}

#endif // GRUNTZ_BRICKZ_H
