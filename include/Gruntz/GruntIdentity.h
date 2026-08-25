#ifndef GRUNTZ_GRUNTIDENTITY_H
#define GRUNTZ_GRUNTIDENTITY_H

#include <rva.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(GruntIdentityPacking)
    GRUNT_IDENTITY_COMPONENT_MASK = 0xff,
    GRUNT_IDENTITY_PLAYER_SHIFT = 8
GZ_ENUM_CONST_END(GruntIdentityPacking)

struct GruntIdentity {
    i32 m_playerIndex;
    i32 m_unitIndex;
};

#endif // GRUNTZ_GRUNTIDENTITY_H
