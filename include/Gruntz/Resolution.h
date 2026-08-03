#ifndef GRUNTZ_GRUNTZ_RESOLUTION_H
#define GRUNTZ_GRUNTZ_RESOLUTION_H

#include <Enums.h>

// Display-mode selection, as stored in the video config and the registry.
GZ_ENUM_BEGIN(Resolution)
// Initial value before the saved mode is read back; the display modes
// themselves start at 1.
    RES_UNSET = 0,
    RES_640x480 = 1,
    RES_800x600 = 2,
    RES_1024x768 = 3
GZ_ENUM_END(Resolution)

#endif // GRUNTZ_GRUNTZ_RESOLUTION_H
