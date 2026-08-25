#ifndef GRUNTZ_GRUNTZ_GRUNTDIRECTION_H
#define GRUNTZ_GRUNTZ_GRUNTDIRECTION_H

#include <rva.h>

#include <Enums.h>

GZ_ENUM_BEGIN(GruntDirection)
    DIR_CENTER = 0,
    DIR_NORTH = 1,
    DIR_NORTHEAST = 2,
    DIR_EAST = 3,
    DIR_SOUTHEAST = 4,
    DIR_SOUTH = 5,
    DIR_SOUTHWEST = 6,
    DIR_WEST = 7,
    DIR_NORTHWEST = 8,
    DIR_RING_COUNT = 8,
    DIR_COUNT = 9
GZ_ENUM_END(GruntDirection)

class CFileMemBase;
class CGameObject;
GZ_ENUM_FORWARD(SerialMode);
GZ_ENUM_FORWARD(LogicTypeId);

struct CTriRecord {
    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);

    i32 row;
    i32 column;
    GruntDirection direction;
};

struct GruntDirectionCell : public CTriRecord {
    GruntDirectionCell() {}
    GruntDirectionCell(i32 row_, i32 column_, GruntDirection direction_) {
        row = row_;
        column = column_;
        direction = direction_;
    }

    void RotateClockwise(i32 steps);
    void RotateCounterclockwise(i32 steps);
};

#endif // GRUNTZ_GRUNTZ_GRUNTDIRECTION_H
