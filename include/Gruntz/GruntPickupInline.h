#ifndef GRUNTZ_GRUNTPICKUPINLINE_H
#define GRUNTZ_GRUNTPICKUPINLINE_H

#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/Grunt.h>

inline PickupType CGrunt::GetEquippedToolType() const {
    PickupType pickup = m_activePickupType;
    if (pickup > PICKUP_EQUIPPABLE_LAST) {
        pickup = m_savedToolType;
    }
    return pickup;
}

inline PickupType CGrunt::ResolveEquippedToolType(PickupType activePickupType) const {
    PickupType pickup = activePickupType;
    if (activePickupType > PICKUP_EQUIPPABLE_LAST) {
        pickup = m_savedToolType;
    }
    return pickup;
}

#define EQUIPPED_TOOL_TERNARY_LE(grunt)                                                            \
    ((grunt->GetActivePickupType() <= PICKUP_EQUIPPABLE_LAST) ? grunt->GetActivePickupType()       \
                                                              : grunt->m_savedToolType)

#define EQUIPPED_TOOL_TERNARY_GT(grunt)                                                            \
    ((grunt->GetActivePickupType() > PICKUP_EQUIPPABLE_LAST) ? grunt->m_savedToolType              \
                                                             : grunt->GetActivePickupType())

#define EQUIPPED_TOOL_OF_TERNARY_LE(grunt, activePickupType)                                       \
    ((activePickupType <= PICKUP_EQUIPPABLE_LAST) ? activePickupType : grunt->m_savedToolType)

#define ADD_BATTLEZ_TRAVERSAL_FLAGS(grunt, flags)                                                  \
    {                                                                                              \
        PickupType prim = (grunt)->GetActivePickupType();                                          \
        if ((grunt)->ResolveEquippedToolType(prim) == PICKUP_TOOB) {                               \
            (flags) |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;                                               \
        } else if ((grunt)->ResolveEquippedToolType(prim) == PICKUP_SPRING) {                      \
            (flags) |= BATTLEZ_ROUTE_SPRING_TRAVERSAL;                                             \
        } else if ((grunt)->ResolveEquippedToolType(prim) == PICKUP_WINGZ) {                       \
            (flags) |= BATTLEZ_ROUTE_WINGZ_TRAVERSAL;                                              \
        }                                                                                          \
    }

#endif // GRUNTZ_GRUNTPICKUPINLINE_H
