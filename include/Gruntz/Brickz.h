#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H
#include <rva.h>
#include <Ints.h>
#include <Gruntz/MapMgr.h>

class CBattlezData;
struct tagRECT;

struct BrickzNode {

    union {
        i32 m_col;
        BrickzNode* m_searchNode;
    };
    union {
        i32 m_row;
        BrickzNode* m_cellPrev;
    };
    union {
        BrickzNode* m_cellNext;
        i32 m_gCost;
    };
    i32 m_hCost;
    i32 m_fCost;
    BrickzNode* m_openNext;
    BrickzNode* m_openPrev;

    BrickzNode* m_parent;
    BrickzNode* m_cellLink;
};
SIZE_UNKNOWN();

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
    i32 m_typeCode;
    i32 m_count;
    BrickzNode* m_head;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_BRICKZ_H
