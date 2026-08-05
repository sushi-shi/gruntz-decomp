#ifndef GRUNTZ_GRUNTZ_RESOLUTION_H
#define GRUNTZ_GRUNTZ_RESOLUTION_H

#include <Enums.h>

// Display-mode selection, as stored in the video config and the registry.
GZ_ENUM_BEGIN(Resolution)
// Initial value before the saved mode is read back; the display modes
// themselves start at 1.
    RES_UNSET = 0,
    RES_640X480 = 1,
    RES_800X600 = 2,
    RES_1024X768 = 3
GZ_ENUM_END(Resolution)

GZ_ENUM_CONST_BEGIN(DisplayResolutionPixels)
    DISPLAY_WIDTH_640 = 640,
    DISPLAY_HEIGHT_480 = 480,
    DISPLAY_WIDTH_800 = 800,
    DISPLAY_HEIGHT_600 = 600,
    DISPLAY_WIDTH_1024 = 1024,
    DISPLAY_HEIGHT_768 = 768
GZ_ENUM_CONST_END(DisplayResolutionPixels)

#endif // GRUNTZ_GRUNTZ_RESOLUTION_H
