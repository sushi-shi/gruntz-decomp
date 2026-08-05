#ifndef GRUNTZ_GRUNTZ_PLAYERSLOT_H
#define GRUNTZ_GRUNTZ_PLAYERSLOT_H

#include <Enums.h>

// The four player slots shared by dialog rows, map ownership and per-player
// tables. Tile actions additionally accept the explicit all-player sentinel.
GZ_ENUM_BEGIN(PlayerSlot)
    PLAYER_SLOT_0 = 0,
    PLAYER_SLOT_1 = 1,
    PLAYER_SLOT_2 = 2,
    PLAYER_SLOT_3 = 3,
    PLAYER_SLOT_ALL = 5
GZ_ENUM_END(PlayerSlot)

GZ_ENUM_CONST_BEGIN(PlayerSlotConstants)
    PLAYER_SLOT_COUNT = 4
GZ_ENUM_CONST_END(PlayerSlotConstants)

#endif // GRUNTZ_GRUNTZ_PLAYERSLOT_H
