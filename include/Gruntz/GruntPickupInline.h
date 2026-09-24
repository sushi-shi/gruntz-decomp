#ifndef GRUNTZ_GRUNTPICKUPINLINE_H
#define GRUNTZ_GRUNTPICKUPINLINE_H

#include <Gruntz/BattlezRouteMaskPreset.h>
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

static inline i32 AddBattlezTraversalFlags(CGrunt* unit, i32 flags) {
    PickupType prim = unit->m_entranceReason;
    PickupType t = ArrivalPickupOf(unit, prim);
    if (t == PICKUP_TOOB) {
        flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
    } else {
        t = prim;
        if (prim > PICKUP_EQUIPPABLE_LAST) {
            t = unit->m_toolId;
        }
        if (t == PICKUP_SPRING) {
            flags |= BATTLEZ_ROUTE_SPRING_TRAVERSAL;
        } else {
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                prim = unit->m_toolId;
            }
            if (prim == PICKUP_WINGZ) {
                flags |= BATTLEZ_ROUTE_WINGZ_TRAVERSAL;
            }
        }
    }
    return flags;
}

#endif // GRUNTZ_GRUNTPICKUPINLINE_H
