#include <rva.h>

#include <Gruntz/DirectionClassify.h>

#include <Gruntz/Grunt.h>
#include <Ints.h>

DATA(0x001e9750)
const double g_slopeNegHalf = -0.5;

DATA(0x001e9758)
const double g_slopePosHalf = 0.5;
DATA(0x001e9760)
const double g_slopePosTwo = 2.0;
DATA(0x001e9768)
const double g_slopeNegTwo = -2.0;

// @early-stop
RVA(0x0004a780, 0x1ec)
GruntDirectionCell* MotionEntity::Classify(MotionEntity* other, char exact) {
    if (other == 0) {
        return &g_gruntMoveDirCenter;
    }
    i32 dy = static_cast<i32>((other->m_positionX - m_positionX));
    i32 dx = static_cast<i32>((m_positionY - other->m_positionY));
    if (dy == 0) {
        if (dx > 0) {
            return &g_gruntMoveDirNorth;
        }
        if (dx < 0) {
            return &g_gruntMoveDirSouth;
        }
        return &g_gruntMoveDirCenter;
    }

    char onCell = exact;
    if (onCell) {
        onCell =
            (static_cast<i32>(m_positionX) == m_gridX && static_cast<i32>(m_positionY) == m_gridY)
                ? 1
                : 0;
    }
    double ratio = static_cast<double>(dx) / static_cast<double>(dy);

    if (dx >= 0 && dy > 0) {
        if (onCell) {
            return &g_gruntMoveDirNorthEast;
        }
        if (ratio <= g_slopePosHalf) {
            return &g_gruntMoveDirEast;
        }
        if (ratio <= g_slopePosTwo) {
            return &g_gruntMoveDirNorthEast;
        }
        return &g_gruntMoveDirNorth;
    }
    if (dx >= 0) {
        if (onCell) {
            return &g_gruntMoveDirNorthWest;
        }
        if (ratio <= g_slopeNegTwo) {
            return &g_gruntMoveDirNorth;
        }
        if (ratio <= g_slopeNegHalf) {
            return &g_gruntMoveDirNorthWest;
        }
        return &g_gruntMoveDirWest;
    }
    if (dy > 0) {
        if (onCell) {
            return &g_gruntMoveDirSouthEast;
        }
        if (ratio <= g_slopeNegTwo) {
            return &g_gruntMoveDirSouth;
        }
        if (ratio <= g_slopeNegHalf) {
            return &g_gruntMoveDirSouthEast;
        }
        return &g_gruntMoveDirEast;
    }

    if (onCell) {
        return &g_gruntMoveDirSouthWest;
    }
    if (ratio <= g_slopePosHalf) {
        return &g_gruntMoveDirWest;
    }
    if (ratio <= g_slopePosTwo) {
        return &g_gruntMoveDirSouthWest;
    }
    return &g_gruntMoveDirSouth;
}
