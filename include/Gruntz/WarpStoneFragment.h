#ifndef GRUNTZ_GRUNTZ_WARPSTONEFRAGMENT_H
#define GRUNTZ_GRUNTZ_WARPSTONEFRAGMENT_H

#include <Enums.h>

// The four progressively collected Warp Stone fragments. The HUD checks them
// in order and uses the same value to select the fragment's fly target.
GZ_ENUM_BEGIN(WarpStoneFragment)
    WARPSTONE_FRAGMENT_FIRST = 1,
    WARPSTONE_FRAGMENT_SECOND = 2,
    WARPSTONE_FRAGMENT_THIRD = 3,
    WARPSTONE_FRAGMENT_FOURTH = 4
GZ_ENUM_END(WarpStoneFragment)

#endif // GRUNTZ_GRUNTZ_WARPSTONEFRAGMENT_H
