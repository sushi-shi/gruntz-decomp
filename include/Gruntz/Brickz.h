#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>

class CBattlezData;
struct tagRECT;
struct BrickzNode;

struct BrickzCellNode {
    BrickzNode* m_searchNode;
    BrickzCellNode* m_cellPrev;
    BrickzCellNode* m_cellNext;
};
SIZE(0xc);

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
SIZE(0x24);

// Every bit in BrickzCell::m_flags that makes a cell not free to move through.
//
// Named from usage rather than from a bit table, because the usage is
// unanimous: all 24 sites across 14 files spell it
// `!(cell->m_flags & BRICKZ_BLOCKED_MASK)` - always negated, always this exact
// composite, never a positive test and never a different mask. The mask exists
// to ask one question, "is nothing set here that would block", and CBrickz asks
// it of opposing neighbour pairs (up/down, left/right, and both diagonals) to
// decide whether a piece can pass between them.
//
// The individual bits (0, 3, 4, 5, 8, 11) are NOT named: that needs the WWD
// tile-attribute table recovered from the binary, and nothing in the tree tests
// them one at a time.
GZ_ENUM_CONST_BEGIN(BrickzCellMask)
    BRICKZ_BLOCKED_MASK = 0x939
GZ_ENUM_CONST_END(BrickzCellMask)

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
SIZE_UNKNOWN();

#endif // GRUNTZ_BRICKZ_H
