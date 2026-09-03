#ifndef GRUNTZ_GRUNTPICKUPINLINE_H
#define GRUNTZ_GRUNTPICKUPINLINE_H

#include <Gruntz/Grunt.h>

inline PickupType ArrivalPickup(CGrunt* grunt) {
    PickupType pickup = grunt->m_entranceReason;
    if (pickup > PICKUP_EQUIPPABLE_LAST) {
        pickup = grunt->m_toolId;
    }
    return pickup;
}

inline PickupType ArrivalPickupOf(CGrunt* grunt, PickupType entranceReason) {
    PickupType pickup = entranceReason;
    if (entranceReason > PICKUP_EQUIPPABLE_LAST) {
        pickup = grunt->m_toolId;
    }
    return pickup;
}

#endif // GRUNTZ_GRUNTPICKUPINLINE_H
