#ifndef GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H

#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>

inline CGrunt* GruntAtCell(CGruntzMgr* reg, const Coord& cell) {
    return reg->m_cmdGrid->m_units[cell.m_y + cell.m_x * TM_UNITS_PER_PLAYER];
}

#endif // GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H
