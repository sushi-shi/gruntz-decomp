#ifndef GRUNTZ_SAVESCREENSHOT_H
#define GRUNTZ_SAVESCREENSHOT_H

#include <Enums.h>
#include <Ints.h>

class CDDSurface;
namespace Utils {
    class RegistryHelper;
}
class CGruntzMgr;

i32 SaveScreenshot(
    CDDSurface* src,
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);

#endif // GRUNTZ_SAVESCREENSHOT_H
