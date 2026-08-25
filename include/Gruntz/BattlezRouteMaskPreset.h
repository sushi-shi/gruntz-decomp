#ifndef GRUNTZ_GRUNTZ_BATTLEZROUTEMASKPRESET_H
#define GRUNTZ_GRUNTZ_BATTLEZROUTEMASKPRESET_H

#include <Enums.h>

// Masks used while Battlez AI retries a route to an enemy base.  They are
// presets over the pathfinder's raw cell-mask domain:
// CGrunt::m_routePassableMask is also ORed with dynamic map state, so it is not
// itself a preset enum.
//
// The two base bits are distinguished by HandleUnitContact: 0x40 handles the
// Wingz/Shovel route, while 0x20 handles Bomb, Gauntlet, Brick, Spy, and the
// remaining tools.  The retry sequence progressively broadens those masks and
// finally admits the trigger/ownership bit 0x4000.
GZ_ENUM_CONST_BEGIN(BattlezRouteMaskPreset)
    BATTLEZ_ROUTE_OTHER_TOOLS = 0x20,
    BATTLEZ_ROUTE_WINGZ_SHOVEL = 0x40,
    BATTLEZ_ROUTE_ALL_TOOLS = 0x60,
    BATTLEZ_ROUTE_TOOB_TRAVERSAL = 0x100,
    BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED = 0x228,
    BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED = 0x248,
    BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED = 0x268,
    BATTLEZ_ROUTE_WINGZ_TRAVERSAL = 0x942,
    BATTLEZ_ROUTE_ALL_TOOLS_WINGZ = 0x962,
    BATTLEZ_ROUTE_SPRING_TRAVERSAL = 0x1000,
    BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER = 0x4020,
    BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER = 0x4268,
    BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER_WINGZ = 0x4962
GZ_ENUM_CONST_END(BattlezRouteMaskPreset)

#endif // GRUNTZ_GRUNTZ_BATTLEZROUTEMASKPRESET_H
