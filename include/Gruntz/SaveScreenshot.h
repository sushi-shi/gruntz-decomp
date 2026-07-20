#ifndef GRUNTZ_SAVESCREENSHOT_H
#define GRUNTZ_SAVESCREENSHOT_H

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
    i32 arg4,
    i32 arg5,
    char* name,
    void* arg7
); // 0x114ff0

#endif // GRUNTZ_SAVESCREENSHOT_H
