#ifndef INCLUDE_GRUNTZ_SCANGRIDMACROS_H
#define INCLUDE_GRUNTZ_SCANGRIDMACROS_H

#include <MfcWin.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/PickupType.h>

inline i32 PickupPriority(PickupType pickup) {
    switch (pickup) {
        case PICKUP_BOMB:
            return 2;
        case PICKUP_WELDER:
            return 3;
        case PICKUP_SWORD:
            return 4;
        case PICKUP_GUNHAT:
            return 5;
        case PICKUP_CLUB:
            return 6;
        case PICKUP_ROCK:
            return 7;
        case PICKUP_SHOVEL:
            return 8;
        case PICKUP_BOOMERANG:
            return 9;
        case PICKUP_SPRING:
            return 10;
        case PICKUP_GAUNTLETZ:
            return 11;
        case PICKUP_WINGZ:
            return 12;
        case PICKUP_SPY:
            return 13;
        case PICKUP_BRICK:
            return 14;
        case PICKUP_GRAVITYBOOTZ:
            return 15;
        case PICKUP_SHIELD:
            return 16;
        case PICKUP_GOOBER:
            return 17;
        case PICKUP_TOOB:
            return 18;
        case PICKUP_GLOVEZ:
            return 19;
        case PICKUP_TIMEBOMB:
            return 20;
        case PICKUP_NERFGUN:
            return 21;
        case PICKUP_WAND:
            return 22;
        default:
            return 23;
    }
}

#endif // INCLUDE_GRUNTZ_SCANGRIDMACROS_H
