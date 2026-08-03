#ifndef GRUNTZ_GRUNTZ_GRUNTDEATHTYPE_H
#define GRUNTZ_GRUNTZ_GRUNTDEATHTYPE_H

#include <Enums.h>

// How a Grunt dies - the slot in CGrunt::m_deathType that picks the death
// animation. Names are the animation the arm loads (`GRUNTZ_EXITZ_*`,
// `GRUNTZ_NORMALGRUNT_DEATH`, ...) in CGrunt's death-asset loader.
// 13 is unrecovered; nothing is invented for it.
GZ_ENUM_BEGIN(GruntDeathType)
// Written by CGrunt's reset path: not dead.
    DEATH_NONE = -1,
    DEATH_DROP = 0,
    DEATH_NORMAL = 1,
    DEATH_SQUASH = 2,
    DEATH_HOLE = 3,
    DEATH_SINK = 4,
    DEATH_MELT = 5,
    DEATH_SHATTER = 6,
    DEATH_BURN = 7,
    DEATH_FALL = 8,
    DEATH_ELECTROCUTE = 9,
    DEATH_KAROKE = 10,
    DEATH_EXPLODE = 11,
    DEATH_DRAIN = 12,
    // The only slot CTriggerMgr::CellDispatch routes to
    // BuildGruntExitAnimation() instead of the death-animation loader.
    DEATH_EXIT = 13,
    DEATH_FALL2 = 14,
    DEATH_QUICKFALL = 15,

    // Slot 12 doubles as the level-exit trigger: its animation is the EXITZ
    // "drain", and CGrunt treats reaching it as warping out (the arrival path
    // posts CMD_LOAD_WORLD when m_deathType is this). Same slot, two readings.
    GRUNT_DEATH_WARPOUT = DEATH_DRAIN
GZ_ENUM_END(GruntDeathType)

#endif // GRUNTZ_GRUNTZ_GRUNTDEATHTYPE_H
