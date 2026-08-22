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

#define ARRIVAL_PICKUP_TERNARY_LE(grunt)                                                           \
    ((grunt->m_entranceReason <= PICKUP_EQUIPPABLE_LAST) ? grunt->m_entranceReason                 \
                                                         : grunt->m_toolId)

#define ARRIVAL_PICKUP_TERNARY_GT(grunt)                                                           \
    ((grunt->m_entranceReason > PICKUP_EQUIPPABLE_LAST) ? grunt->m_toolId : grunt->m_entranceReason)

#define ARRIVAL_PICKUP_OF_TERNARY_LE(grunt, entranceReason)                                        \
    ((entranceReason <= PICKUP_EQUIPPABLE_LAST) ? entranceReason : grunt->m_toolId)

#endif // GRUNTZ_GRUNTPICKUPINLINE_H
