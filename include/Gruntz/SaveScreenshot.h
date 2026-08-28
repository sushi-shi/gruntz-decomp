#ifndef GRUNTZ_SAVESCREENSHOT_H
#define GRUNTZ_SAVESCREENSHOT_H

#include <Enums.h>
#include <Ints.h>

class CDDSurface;
class CRegMgr;
class CGruntzMgr;

i32 SaveScreenshot(
    CDDSurface* src,
    CRegMgr* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);

#endif // GRUNTZ_SAVESCREENSHOT_H
