#ifndef GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>

inline CGrunt* FindGruntByIdentity(CGruntzMgr* reg, const GruntIdentity& identity) {
    return reg->m_triggerMgr
        ->m_units[identity.m_unitIndex + identity.m_playerIndex * TM_UNITS_PER_PLAYER];
}

#endif // GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H
