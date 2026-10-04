#ifndef GRUNTZ_GRUNTZ_CHAINFORWARD_H
#define GRUNTZ_GRUNTZ_CHAINFORWARD_H

#include <Enums.h>
#include <Ints.h>

class CGruntzMgr;
class Settings;

i32 SaveBackBufferShot(
    Settings* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);
i32 SaveOverlayBufferShot(
    Settings* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);

#endif
