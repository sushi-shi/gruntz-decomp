#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H
#include <rva.h>
#include <Ints.h>
#include <Gruntz/MapMgr.h>

class CBattlezData;
struct tagRECT;

struct BrickzNode {

    union {
        i32 m_0;
        BrickzNode* m_child;
    };
    union {
        i32 m_4;
        BrickzNode* m_prev;
    };
    union {
        BrickzNode* m_8;
        BrickzNode* m_next;

        i32 m_gCost;
    };
    i32 m_c;
    i32 m_10;
    BrickzNode* m_14;
    BrickzNode* m_18;

    BrickzNode* m_parent;
    BrickzNode* m_20;
};
SIZE_UNKNOWN();

struct BrickzCell {

    union {
        i32 m_0;
        u8 m_flagBytes[4];
    };

    union {
        i32 m_4;
        u8 m_4Bytes[4];
    };

    i32 m_objectId;
    i32 m_c;
    i32 m_10;
    i32 m_count;
    BrickzNode* m_head;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_BRICKZ_H
