#ifndef GRUNTZ_GRUNTDIRECTIONINLINE_H
#define GRUNTZ_GRUNTDIRECTIONINLINE_H

#include <Gruntz/Grunt.h>

inline GruntDirectionCell MovementDirection(const Coord& from, const Coord& to) {
    if (to.m_x > from.m_x) {
        if (to.m_y > from.m_y) {
            return g_gruntMoveDirSouthEast;
        }
        if (to.m_y == from.m_y) {
            return g_gruntMoveDirEast;
        }
        return g_gruntMoveDirNorthEast;
    }
    if (to.m_x < from.m_x) {
        if (to.m_y > from.m_y) {
            return g_gruntMoveDirSouthWest;
        }
        if (to.m_y == from.m_y) {
            return g_gruntMoveDirWest;
        }
        return g_gruntMoveDirNorthWest;
    }
    if (to.m_y < from.m_y) {
        return g_gruntMoveDirNorth;
    }
    return g_gruntMoveDirSouth;
}

#endif // GRUNTZ_GRUNTDIRECTIONINLINE_H
