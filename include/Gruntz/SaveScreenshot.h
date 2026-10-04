#ifndef GRUNTZ_SAVESCREENSHOT_H
#define GRUNTZ_SAVESCREENSHOT_H

#include <Enums.h>
#include <Ints.h>
#include <Io/Bytes.h>

class CDDSurface;
class Settings;
class CGruntzMgr;

i32 SaveScreenshot(
    CDDSurface* src,
    Settings* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);

i32 SaveScreenshot(CDDSurface* src, CGruntzMgr* owner, i32 width, i32 height, io::Output& target);

#endif
