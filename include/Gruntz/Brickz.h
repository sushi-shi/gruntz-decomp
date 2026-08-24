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
// Bit 29 of BrickzCell::m_flags: a grunt is standing on / has reserved this
// cell. Named from its writers, which always move it in lockstep with
// m_occupantId:
//
//   CGrunt::StepArrivalDrop  sets it on the destination cell and writes
//     m_occupantId = (m_playerIndex << 8) | m_unitIndex in the next
//     instruction, having just cleared it (`m_flagBytes[3] &= 0xdf`) and set
//     m_occupantId = -1 on the cell it left.
//   CGrunt::Place            does the same pair when a grunt is spawned.
//   CGrunt::StepRespawn      clears it and sets m_occupantId = -1.
//   CMapMgr::RecomputeCell   carries it across a tile-attribute recompute in
//     the same breath as the 0x1bf40000 keep-mask: the tile kind decides every
//     other bit, occupancy is not the tile's business.
//   CMapMgr::SearchEdge      pulls it out of the caller's route mask
//     (`m_edgeMask = maskA & BRICKZ_CELL_OCCUPIED`), which is how a caller asks
//     the pathfinder to route THROUGH occupied cells.
//
// It is deliberately NOT in BRICKZ_BLOCKED_MASK: occupancy blocks a path only
// when the caller opts in through the route mask.
// Bit 13. Two observed uses, and only two, so the name states the role both
// agree on and claims nothing about the writer (which is not reconstructed):
//
//   CBrickz::SearchRoute       passes it as CMapMgr::Search's maskB.
//   TmDeflectStep              requires it on BOTH orthogonal neighbours of a
//     diagonal candidate before it will let a grunt cut that corner.
GZ_ENUM_CONST_BEGIN(BrickzCellMask)
    BRICKZ_BLOCKED_MASK = 0x939,
    BRICKZ_CELL_ROUTE_MASKB = 0x2000,
    BRICKZ_CELL_OCCUPIED = 0x20000000,
    BRICKZ_CELL_UNOCCUPIED_MASK = ~0x20000000
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

// Bounds-checked cell-flag read; out-of-bounds cells carry the blocking bit 0.
// TmDeflectStep is the only caller of the retail out-of-line COMDAT.
RVA(0x00075a40, 0x34)
inline i32 CMapMgr::CellFlagsAt(i32 x, i32 y) {
    if (static_cast<u32>(x) < m_width && static_cast<u32>(y) < m_height) {
        return m_rows[y][x].m_flags;
    }
    return 1;
}

#endif // GRUNTZ_BRICKZ_H
