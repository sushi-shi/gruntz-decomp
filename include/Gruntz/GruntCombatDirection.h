#ifndef GRUNTZ_GRUNTZ_GRUNTCOMBATDIRECTION_H
#define GRUNTZ_GRUNTZ_GRUNTCOMBATDIRECTION_H

#include <Enums.h>
#include <Gruntz/Grunt.h>
#include <Ints.h>

GZ_ENUM_BEGIN(WingzKnockbackChoice)
    WINGZ_KNOCKBACK_NORTHEAST = 0,
    WINGZ_KNOCKBACK_EAST = 1,
    WINGZ_KNOCKBACK_SOUTHEAST = 2,
    WINGZ_KNOCKBACK_SOUTH = 3,
    WINGZ_KNOCKBACK_SOUTHWEST = 4,
    WINGZ_KNOCKBACK_WEST = 5,
    WINGZ_KNOCKBACK_NORTHWEST = 6
GZ_ENUM_END(WingzKnockbackChoice)

inline i32 OppositeGridIndex(i32 index) {
    switch (index) {
        case GRUNT_DIRECTION_GRID_LOW:
            index = GRUNT_DIRECTION_GRID_HIGH;
            break;
        case GRUNT_DIRECTION_GRID_HIGH:
            index = GRUNT_DIRECTION_GRID_LOW;
            break;
        default:
            break;
    }
    return index;
}

inline CGruntCellRec* GruntCellAt(CGrunt* grunt, i32 row, i32 column) {
    return &grunt->m_cells[GRUNT_DIRECTION_GRID_WIDTH * row + column];
}

#endif // GRUNTZ_GRUNTZ_GRUNTCOMBATDIRECTION_H
