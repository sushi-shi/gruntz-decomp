#ifndef GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>

inline CGrunt* FindGruntByIdentity(CGruntzMgr* reg, const GruntIdentity& identity) {
    return reg->m_triggerMgr->UnitAt(identity.m_playerIndex, identity.m_unitIndex);
}

#endif // GRUNTZ_GRUNTZ_GRUNTCELLINLINE_H
