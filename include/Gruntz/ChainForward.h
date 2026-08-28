#ifndef GRUNTZ_GRUNTZ_CHAINFORWARD_H
#define GRUNTZ_GRUNTZ_CHAINFORWARD_H

#include <Enums.h>
#include <Ints.h>

class CGruntzMgr;
class CRegMgr;

i32 SaveBackBufferShot(
    CRegMgr* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);
i32 SaveOverlayBufferShot(
    CRegMgr* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);

#endif // GRUNTZ_GRUNTZ_CHAINFORWARD_H
