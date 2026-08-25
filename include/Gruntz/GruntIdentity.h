#ifndef GRUNTZ_GRUNTIDENTITY_H
#define GRUNTZ_GRUNTIDENTITY_H

#include <rva.h>

// The two-dimensional identity used to address CTriggerMgr::m_units. This is
// not a map coordinate: the first component selects a player and the second a
// unit within that player's fixed-size roster.
struct GruntIdentity {
    i32 m_playerIndex;
    i32 m_unitIndex;
};

#endif // GRUNTZ_GRUNTIDENTITY_H
