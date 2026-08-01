

#include <Ints.h>
#include <rva.h>
#include <Gruntz/DirectionClassify.h>
#include <Gruntz/Grunt.h>

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
    i32 dy = static_cast<i32>((other->m_78 - m_78));
    i32 dx = static_cast<i32>((m_80 - other->m_80));
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
        onCell = (static_cast<i32>(m_78) == m_140 && static_cast<i32>(m_80) == m_144) ? 1 : 0;
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
