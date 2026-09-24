#ifndef GRUNTZ_GRUNTPICKUPINLINE_H
#define GRUNTZ_GRUNTPICKUPINLINE_H

#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/Grunt.h>

inline PickupType CGrunt::ArrivalPickup() const {
    PickupType pickup = m_entranceReason;
    if (pickup > PICKUP_EQUIPPABLE_LAST) {
        pickup = m_toolId;
    }
    return pickup;
}

inline PickupType CGrunt::ArrivalPickupOf(PickupType entranceReason) const {
    PickupType pickup = entranceReason;
    if (entranceReason > PICKUP_EQUIPPABLE_LAST) {
        pickup = m_toolId;
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

inline i32 CGrunt::AddBattlezTraversalFlags(i32 flags) const {
    PickupType prim = m_entranceReason;
    PickupType t = ArrivalPickupOf(prim);
    if (t == PICKUP_TOOB) {
        flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
    } else {
        t = prim;
        if (prim > PICKUP_EQUIPPABLE_LAST) {
            t = m_toolId;
        }
        if (t == PICKUP_SPRING) {
            flags |= BATTLEZ_ROUTE_SPRING_TRAVERSAL;
        } else {
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                prim = m_toolId;
            }
            if (prim == PICKUP_WINGZ) {
                flags |= BATTLEZ_ROUTE_WINGZ_TRAVERSAL;
            }
        }
    }
    return flags;
}

#endif // GRUNTZ_GRUNTPICKUPINLINE_H
