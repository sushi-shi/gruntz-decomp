#ifndef GRUNTZ_GRUNTZ_CHAINFORWARD_H
#define GRUNTZ_GRUNTZ_CHAINFORWARD_H

#include <Ints.h>

class CGruntzMgr;
namespace Utils {
    class RegistryHelper;
}

i32 ChainForward14(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);
i32 ChainForward(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
);

#endif // GRUNTZ_GRUNTZ_CHAINFORWARD_H
