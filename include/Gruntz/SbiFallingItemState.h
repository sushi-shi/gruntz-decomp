#ifndef GRUNTZ_GRUNTZ_SBIFALLINGITEMSTATE_H
#define GRUNTZ_GRUNTZ_SBIFALLINGITEMSTATE_H

#include <Enums.h>

// Phase of the resource icon dropped through the status-bar chip grinder.
GZ_ENUM_BEGIN(SbiFallingItemState)
    FALLING_ITEM_INACTIVE = 0,
    FALLING_ITEM_DESCENDING = 1,
    FALLING_ITEM_GRINDING = 2
GZ_ENUM_END(SbiFallingItemState)

#endif // GRUNTZ_GRUNTZ_SBIFALLINGITEMSTATE_H
