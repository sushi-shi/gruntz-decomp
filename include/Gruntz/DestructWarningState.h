#ifndef GRUNTZ_GRUNTZ_DESTRUCTWARNINGSTATE_H
#define GRUNTZ_GRUNTZ_DESTRUCTWARNINGSTATE_H

#include <Enums.h>

// Direction of the destruct-button warning animation. The frame rises from 2
// to 6 in FORWARD, then falls back to 2 in REVERSE.
GZ_ENUM_BEGIN(DestructWarningState)
    DESTRUCT_WARNING_INACTIVE = 0,
    DESTRUCT_WARNING_FORWARD = 1,
    DESTRUCT_WARNING_REVERSE = 2
GZ_ENUM_END(DestructWarningState)

// Frames in GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT. The warning oscillates over
// the inclusive 2..6 band; frame 7 is the disabled multiplayer image.
GZ_ENUM_BEGIN(DestructButtonFrame)
    DESTRUCT_FRAME_IDLE = 1,
    DESTRUCT_FRAME_WARNING_FIRST = 2,
    DESTRUCT_FRAME_WARNING_LAST = 6,
    DESTRUCT_FRAME_DISABLED = 7
GZ_ENUM_END(DestructButtonFrame)
GZ_ENUM_STEPPED(DestructButtonFrame)

#endif // GRUNTZ_GRUNTZ_DESTRUCTWARNINGSTATE_H
